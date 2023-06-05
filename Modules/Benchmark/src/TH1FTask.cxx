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
/// \file   TH1FTask.cxx
/// \author Piotr Konopka
///

#include "Benchmark/TH1FTask.h"

#include <TH1F.h>
#include <Headers/DataHeader.h>
#include "QualityControl/QcInfoLogger.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/DataRefUtils.h>

namespace o2::quality_control_modules::benchmark
{

TH1FTask::~TH1FTask()
{
}

void TH1FTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize TH1FTask" << ENDM;

  int histogramsNumber = 1;
  if (mCustomParameters.count("histoNumber") > 0) {
    histogramsNumber = stoi(mCustomParameters.at("histoNumber"));
  }

  int binsNumber = 20;
  if (mCustomParameters.count("binsNumber") > 0) {
    binsNumber = stoi(mCustomParameters.at("binsNumber"));
  }

  ILOG(Info, Support) << "Will create " << histogramsNumber << " histograms." << ENDM;
  ILOG(Info, Support) << "They will have " << binsNumber << " bins each." << ENDM;
  ILOG(Info, Support) << "In-memory size of one histogram will be around " << binsNumber * 4 << " bytes." << ENDM;

  for (int i = 0; i < histogramsNumber; i++) {
    std::string name = "histo-" + std::to_string(i);
    mHistograms.push_back(std::make_shared<TH1F>(name.c_str(), name.c_str(), binsNumber, 0, 30000));
    getObjectsManager()->startPublishing(mHistograms.back().get());
  }
}

void TH1FTask::startOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;
  for (auto& histo : mHistograms) {
    histo->Reset();
  }
}

void TH1FTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void TH1FTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // the minimum that we can do, which includes accessing data and creating non-empty histograms
  size_t dummySum = 0;
  for (auto&& input : o2::framework::InputRecordWalker(ctx.inputs())) {
    if (input.header != nullptr && input.payload != nullptr) {
      // TODO: better use InputRecord::get<std::byte> to get a span of the raw input data
      // and use its size and first byte
      const auto* header = o2::framework::DataRefUtils::getHeader<header::DataHeader*>(input);
      auto payloadSize = o2::framework::DataRefUtils::getPayloadSize(input);
      dummySum += payloadSize + input.payload[0];
    }
  }

  dummySum %= 30000;
  for (auto& histo : mHistograms) {
    histo->Fill(dummySum++);
  }
}

void TH1FTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void TH1FTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void TH1FTask::reset()
{
  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  for (auto& histo : mHistograms) {
    histo->Reset();
  }
}

} // namespace o2::quality_control_modules::benchmark
