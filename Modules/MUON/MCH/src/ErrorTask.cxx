// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   ErrorTask.cxx
/// \author Philippe Pillot
///

#include <array>

#include <TProfile.h>

#include "QualityControl/QcInfoLogger.h"
#include "MCH/ErrorTask.h"
#include "MCHBase/Error.h"
#include "MCHBase/ErrorMap.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/DataRefUtils.h>

namespace o2::quality_control_modules::muonchambers
{

using namespace o2::mch;

constexpr uint32_t getNDE()
{
  return 156;
}

constexpr uint32_t getDEIdxOffset(uint32_t chamber)
{
  constexpr int offset[10] = { 0, 4, 8, 12, 16, 34, 52, 78, 104, 130 };
  return offset[chamber];
}

constexpr uint32_t getDEIdx(uint32_t de)
{
  if (de < 100 || de > 1025) {
    return getNDE();
  }
  return getDEIdxOffset(de / 100 - 1) + de % 100;
}

constexpr uint32_t getDE(uint32_t idx)
{
  uint32_t chamber = 9;
  while (idx < getDEIdxOffset(chamber)) {
    --chamber;
  }
  return (chamber + 1) * 100 + idx - getDEIdxOffset(chamber);
}

auto ErrorTask::createProfile(const char* name, const char* title, int nbins, double xmin, double xmax)
{
  // set the bin error option to "i" to make sure bins filled with zeros also get an error,
  // otherwise the merging of TProfile with labels set on the x-axis ignores these bins,
  // which biases the counting of the number of entries in these bins and thus the
  // calculation of the average if their content is not always zero.
  auto h = std::make_unique<TProfile>(name, title, nbins, xmin, xmax, "i");
  h->SetStats(0);
  getObjectsManager()->startPublishing(h.get());
  return h;
}

void ErrorTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize ErrorTask" << ENDM;

  mSummary = createProfile("Summary", "summary of all processing errors;;# per TF",
                           Error::typeNames.size(), 0., Error::typeNames.size());
  int i = 0;
  for (const auto& typeName : Error::typeNames) {
    mSummary->GetXaxis()->SetBinLabel(++i, typeName.second.c_str());
  }

  auto type = ErrorType::PreClustering_MultipleDigitsInSamePad;
  mMultipleDigitsInSamePad = createProfile(Error::typeNames.at(type).c_str(),
                                           (Error::typeDescriptions.at(type) + ";DE;# per TF").c_str(),
                                           getNDE(), 0., getNDE());

  type = ErrorType::Clustering_TooManyLocalMaxima;
  mTooManyLocalMaxima = createProfile(Error::typeNames.at(type).c_str(),
                                      (Error::typeDescriptions.at(type) + ";DE;# per TF").c_str(),
                                      getNDE(), 0., getNDE());

  for (auto i = 0; i < getNDE(); ++i) {
    mMultipleDigitsInSamePad->GetXaxis()->SetBinLabel(i + 1, std::to_string(getDE(i)).c_str());
    mTooManyLocalMaxima->GetXaxis()->SetBinLabel(i + 1, std::to_string(getDE(i)).c_str());
  }
}

void ErrorTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
  reset();
}

void ErrorTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void ErrorTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  auto errors = ctx.inputs().get<gsl::span<Error>>("errors");

  ErrorMap errorMap{};
  errorMap.add(errors);

  // summary of all processing errors
  for (const auto& [type, name] : Error::typeNames) {
    mSummary->Fill(name.c_str(), errorMap.getNumberOfErrors(type));
  }

  // preclustering error "multiple digits on the same pad" per DE
  std::array<uint64_t, getNDE() + 1> errorsPerDE{};
  errorMap.forEach(ErrorType::PreClustering_MultipleDigitsInSamePad, [&errorsPerDE](Error error) {
    errorsPerDE[getDEIdx(error.id0)] += error.count;
  });
  for (int i = 0; i <= getNDE(); ++i) {
    mMultipleDigitsInSamePad->Fill(i, errorsPerDE[i]);
  }

  // clustering error "too many local maxima" per DE
  errorsPerDE.fill(0);
  errorMap.forEach(ErrorType::Clustering_TooManyLocalMaxima, [&errorsPerDE](Error error) {
    errorsPerDE[getDEIdx(error.id0)] += error.count;
  });
  for (int i = 0; i <= getNDE(); ++i) {
    mTooManyLocalMaxima->Fill(i, errorsPerDE[i]);
  }
}

void ErrorTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void ErrorTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void ErrorTask::reset()
{
  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  mSummary->Reset();
  mMultipleDigitsInSamePad->Reset();
  mTooManyLocalMaxima->Reset();
}

} // namespace o2::quality_control_modules::muonchambers
