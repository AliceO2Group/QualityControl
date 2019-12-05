// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   BenchmarkTask.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#include "Benchmark/TH2FTask.h"

#include <TH2F.h>
#include <Headers/DataHeader.h>
#include "QualityControl/QcInfoLogger.h"

namespace o2::quality_control_modules::benchmark
{

TH2FTask::~TH2FTask()
{
}

void TH2FTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  QcInfoLogger::GetInstance() << "initialize TH2FTask" << AliceO2::InfoLogger::InfoLogger::endm;

  int histogramsNumber = 1;
  if (auto param = mCustomParameters.find("histoNumber"); param != mCustomParameters.end()) {
    histogramsNumber = stoi(param->second);
  }

  int binsNumber = 20;
  if (auto param = mCustomParameters.find("binsNumber"); param != mCustomParameters.end()) {
    binsNumber = stoi(param->second);
  }

  int checksPerHisto = 1;
  if (auto param = mCustomParameters.find("checksPerHisto"); param != mCustomParameters.end()) {
    checksPerHisto = stoi(param->second);
  }

  QcInfoLogger::GetInstance() << "Will create " << histogramsNumber << " histograms." << AliceO2::InfoLogger::InfoLogger::endm;
  QcInfoLogger::GetInstance() << "They will have 2 dimensions with " << binsNumber << " x " << binsNumber << " bins each." << AliceO2::InfoLogger::InfoLogger::endm;
  QcInfoLogger::GetInstance() << "In-memory size of one histogram will be around " << binsNumber * binsNumber * 4 << " bytes." << AliceO2::InfoLogger::InfoLogger::endm;

  for (int i = 0; i < histogramsNumber; i++) {
    std::string name = "histo-" + std::to_string(i);
    mHistograms.push_back(std::make_shared<TH2F>(name.c_str(), name.c_str(), binsNumber, 0, 30000, binsNumber, 0, 30000));
    getObjectsManager()->startPublishing(mHistograms.back().get());
    for (int i = 0; i < checksPerHisto; i++) {
      getObjectsManager()->addCheck(mHistograms.back().get(), "bmCheck-" + std::to_string(i),
                                    "o2::quality_control_modules::benchmark::AlwaysGoodTH2Check", "QcBenchmark");
    }
  }
}

void TH2FTask::startOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
  for (auto& histo : mHistograms) {
    histo->Reset();
  }
}

void TH2FTask::startOfCycle()
{
  QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void TH2FTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // the minimum that we can do, which includes accessing data and creating non-empty histograms
  size_t dummySum = 0;
  for (auto&& input : ctx.inputs()) {
    if (input.header != nullptr && input.payload != nullptr) {
      const auto* header = header::get<header::DataHeader*>(input.header);
      dummySum += header->payloadSize + input.payload[0];
    }
  }

  for (auto& histo : mHistograms) {
    histo->Fill(dummySum, dummySum + 1);
    dummySum += 2;
  }
}

void TH2FTask::endOfCycle()
{
  QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void TH2FTask::endOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void TH2FTask::reset()
{
  QcInfoLogger::GetInstance() << "Resetting the histograms" << AliceO2::InfoLogger::InfoLogger::endm;
  for (auto& histo : mHistograms) {
    histo->Reset();
  }
}

} // namespace o2::quality_control_modules::benchmark
