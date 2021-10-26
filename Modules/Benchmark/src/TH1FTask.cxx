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

namespace o2::quality_control_modules::benchmark
{

TH1FTask::~TH1FTask()
{
}

void TH1FTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info) << "initialize TH1FTask" << ENDM;

  int histogramsNumber = 1;
  if (auto param = mCustomParameters.find("histoNumber"); param != mCustomParameters.end()) {
    histogramsNumber = stoi(param->second);
  }

  int binsNumber = 20;
  if (auto param = mCustomParameters.find("binsNumber"); param != mCustomParameters.end()) {
    binsNumber = stoi(param->second);
  }

  ILOG(Info) << "Will create " << histogramsNumber << " histograms." << ENDM;
  ILOG(Info) << "They will have " << binsNumber << " bins each." << ENDM;
  ILOG(Info) << "In-memory size of one histogram will be around " << binsNumber * 4 << " bytes." << ENDM;

  for (int i = 0; i < histogramsNumber; i++) {
    std::string name = "histo-" + std::to_string(i);
    mHistograms.push_back(std::make_shared<TH1F>(name.c_str(), name.c_str(), binsNumber, 0, 30000));
    getObjectsManager()->startPublishing(mHistograms.back().get());
  }
}

void TH1FTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "startOfActivity" << ENDM;
  for (auto& histo : mHistograms) {
    histo->Reset();
  }
}

void TH1FTask::startOfCycle()
{
  ILOG(Info) << "startOfCycle" << ENDM;
}

void TH1FTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // the minimum that we can do, which includes accessing data and creating non-empty histograms
  size_t dummySum = 0;
  for (auto&& input : ctx.inputs()) {
    if (input.header != nullptr && input.payload != nullptr) {
      const auto* header = header::get<header::DataHeader*>(input.header);
      dummySum += header->payloadSize + input.payload[0];
    }
  }

  dummySum %= 30000;
  for (auto& histo : mHistograms) {
    histo->Fill(dummySum++);
  }
}

void TH1FTask::endOfCycle()
{
  ILOG(Info) << "endOfCycle" << ENDM;
}

void TH1FTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "endOfActivity" << ENDM;
}

void TH1FTask::reset()
{
  ILOG(Info) << "Resetting the histograms" << ENDM;
  for (auto& histo : mHistograms) {
    histo->Reset();
  }
}

} // namespace o2::quality_control_modules::benchmark
