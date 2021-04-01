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
/// \file   BasicReadoutHeaderQcTask.cxx
/// \author Tomas Herman
/// \author Guillermo Contreras
///

// ROOT
#include <TH1.h>
// O2
#include <DPLUtils/RawParser.h>
#include <DPLUtils/DPLRawParser.h>
// #include <Framework/InputRecord.h> // needed??
// Quality Control
#include "QualityControl/QcInfoLogger.h"
#include "MFT/BasicReadoutHeaderQcTask.h"

using namespace o2::framework;
using namespace o2::header;

namespace o2::quality_control_modules::mft
{

BasicReadoutHeaderQcTask::~BasicReadoutHeaderQcTask()
{
  /*
    not needed for unique pointers
  */
}

void BasicReadoutHeaderQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize BasicReadoutHeaderQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - myOwnKey: " << param->second << ENDM;
  }

  // Defining histograms
  //==============================================
  mMFT_LaneStatus_H = std::make_unique<TH1F>("mMFT_LaneStatus_H", "mMFT_LaneStatus_H", 5, -0.5, 4.5);
  getObjectsManager()->startPublishing(mMFT_LaneStatus_H.get());
  mMFT_LaneStatus_H->GetXaxis()->SetBinLabel(1, "Missing data");
  mMFT_LaneStatus_H->GetXaxis()->SetBinLabel(2, "Warning");
  mMFT_LaneStatus_H->GetXaxis()->SetBinLabel(3, "Error");
  mMFT_LaneStatus_H->GetXaxis()->SetBinLabel(4, "Fault");
  mMFT_LaneStatus_H->GetXaxis()->SetBinLabel(5, "#Entries");
}

void BasicReadoutHeaderQcTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;
  mMFT_LaneStatus_H->Reset();
}

void BasicReadoutHeaderQcTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void BasicReadoutHeaderQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // get the input
  DPLRawParser parser(ctx.inputs());

  // loop over input
  for (auto it = parser.begin(), end = parser.end(); it != end; ++it) {
    // get the header
    auto const* rdh = it.get_if<o2::header::RAWDataHeaderV6>();
    // get detector field
    uint64_t SummaryLaneStatus = rdh->detectorField;
    // fill histogram bin with #entries
    mMFT_LaneStatus_H->Fill(4);
    // fill status if set
    if (SummaryLaneStatus & (1 << 0))
      mMFT_LaneStatus_H->Fill(0); // missing data
    if (SummaryLaneStatus & (1 << 1))
      mMFT_LaneStatus_H->Fill(1); // warning
    if (SummaryLaneStatus & (1 << 2))
      mMFT_LaneStatus_H->Fill(2); // error
    if (SummaryLaneStatus & (1 << 3))
      mMFT_LaneStatus_H->Fill(3); // fault
  }                               // end loop over input
}

void BasicReadoutHeaderQcTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void BasicReadoutHeaderQcTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void BasicReadoutHeaderQcTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  mMFT_LaneStatus_H->Reset();
}

} // namespace o2::quality_control_modules::mft
