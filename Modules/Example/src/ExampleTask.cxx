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
/// \file   ExampleTask.cxx
/// \author Barthelemy von Haller
///

#include "Example/ExampleTask.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/DataRefUtils.h>
#include "QualityControl/QcInfoLogger.h"
#include <TH1.h>
#include "Example/CustomTH2F.h"

using namespace std;

namespace o2::quality_control_modules::example
{

ExampleTask::ExampleTask() : TaskInterface()
{
  for (auto& mHisto : mHistos) {
    mHisto = nullptr;
  }
  mNumberCycles = 0;
}

ExampleTask::~ExampleTask()
{
  for (auto& mHisto : mHistos) {
    delete mHisto;
  }
}

void ExampleTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize ExampleTask" << ENDM;

  for (int i = 0; i < 24; i++) {
    publishHisto(i);
  }

  // Extendable axis
  mHistos[0]->SetCanExtend(TH1::kXaxis);

  mCustomTH2F = new CustomTH2F("customTH2F");
  getObjectsManager()->startPublishing(mCustomTH2F);
}

void ExampleTask::publishHisto(int i)
{
  stringstream name;
  name << "array-" << i;
  mHistos[i] = new TH1F(name.str().c_str(), name.str().c_str(), 100, 0, 99);
  getObjectsManager()->startPublishing(mHistos[i]);
}

void ExampleTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity : " << activity.mId << ENDM;
  for (auto& mHisto : mHistos) {
    if (mHisto) {
      mHisto->Reset();
    }
  }
}

void ExampleTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void ExampleTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  for (const auto& input : o2::framework::InputRecordWalker(ctx.inputs())) {
    if (input.header != nullptr) {
      const auto payloadSize = o2::framework::DataRefUtils::getPayloadSize(input);
      mHistos[0]->Fill(payloadSize);
      mCustomTH2F->Fill(payloadSize % 100, payloadSize % 100);
      for (auto& mHisto : mHistos) {
        if (mHisto) {
          mHisto->FillRandom("gaus", 1);
        }
      }
      break;
    }
  }
}

void ExampleTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
  mNumberCycles++;

  // Add one more object just to show that we can do it
  if (mNumberCycles == 3) {
    publishHisto(24);
  }
}

void ExampleTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void ExampleTask::reset() { ILOG(Info, Support) << "Reset" << ENDM; }

} // namespace o2::quality_control_modules::example
