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
/// \file   ExampleTask.cxx
/// \author Barthelemy von Haller
///

#include "Example/ExampleTask.h"
#include <Framework/InputRecord.h>
#include "QualityControl/QcInfoLogger.h"
#include <TH1.h>

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
  ILOG(Info, Support) << "initialize ExampleTask" << ENDM;

  for (int i = 0; i < 24; i++) {
    publishHisto(i);
  }

  // Extendable axis
  mHistos[0]->SetCanExtend(TH1::kXaxis);
}

void ExampleTask::publishHisto(int i)
{
  stringstream name;
  name << "array-" << i;
  mHistos[i] = new TH1F(name.str().c_str(), name.str().c_str(), 100, 0, 99);
  getObjectsManager()->startPublishing(mHistos[i]);
}

void ExampleTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity : " << activity.mId << ENDM;
  for (auto& mHisto : mHistos) {
    if (mHisto) {
      mHisto->Reset();
    }
  }
}

void ExampleTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void ExampleTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  for (auto& input : ctx.inputs()) {
    if (input.header != nullptr) {
      const auto* header = o2::header::get<header::DataHeader*>(input.header); // header of first valid input
      mHistos[0]->Fill(header->payloadSize);
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
  ILOG(Info, Support) << "endOfCycle" << ENDM;
  mNumberCycles++;

  // Add one more object just to show that we can do it
  if (mNumberCycles == 3) {
    publishHisto(24);
  }
}

void ExampleTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void ExampleTask::reset() { ILOG(Info, Support) << "Reset" << ENDM; }

} // namespace o2::quality_control_modules::example
