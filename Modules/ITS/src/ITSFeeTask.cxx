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
/// \file   ITSFeeTask.cxx
/// \author Jian Liu
/// \author Liang Zhang
///

#include "ITS/ITSFeeTask.h"
#include "QualityControl/QcInfoLogger.h"

#include <DPLUtils/RawParser.h>
#include <DPLUtils/DPLRawParser.h>

using namespace o2::framework;
using namespace o2::header;

namespace o2::quality_control_modules::its
{

ITSFeeTask::ITSFeeTask()
  : TaskInterface()
{
}

ITSFeeTask::~ITSFeeTask()
{
  delete mTFInfo;
  delete mTrigger;
  delete mTriggerVsFeeId;
  delete mLaneInfo;
  delete mFlag1Check;
  delete mIndexCheck;
  delete mIdCheck;
  delete mProcessingTime;
  for (int i = 0; i < NFlags; i++) {
    delete mLaneStatus[i];
  }

  //delete mInfoCanvas;
}

void ITSFeeTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info) << "Initializing the ITSFeeTask" << ENDM;
  createFeePlots();
  setPlotsFormat();
}

void ITSFeeTask::createFeePlots()
{
  mTrigger = new TH1I("TriggerFlag", "Trigger vs counts", mNTrigger, 0.5, mNTrigger + 0.5);
  getObjectsManager()->startPublishing(mTrigger); //mTrigger

  mTFInfo = new TH1I("STFInfo", "STF vs count", 15000, 0, 15000);
  getObjectsManager()->startPublishing(mTFInfo); //mTFInfo

  mLaneInfo = new TH2I("LaneInfo", "Lane Information", NLanes, -.5, NLanes - 0.5, NFlags, -.5, NFlags - 0.5);
  getObjectsManager()->startPublishing(mLaneInfo); //mLaneInfo

  mProcessingTime = new TH1I("ProcessingTime", "Processing Time", 10000, 0, 10000);
  getObjectsManager()->startPublishing(mProcessingTime); //mProcessingTime

  mTriggerVsFeeId = new TH2I("TriggerVsFeeid", "Trigger count vs Trigger ID and Fee ID", NFees, 0, NFees, mNTrigger, 0.5, mNTrigger + 0.5);
  getObjectsManager()->startPublishing(mTriggerVsFeeId); //mTriggervsFeeId

  for (int i = 0; i < NFlags; i++) {
    mLaneStatus[i] = new TH2I(Form("LaneStatus/laneStatusFlag%s", mLaneStatusFlag[i].c_str()), Form("Lane Status Flag : %s", mLaneStatusFlag[i].c_str()), NFees, 0, NFees, NLanes, 0, NLanes);
    getObjectsManager()->startPublishing(mLaneStatus[i]); //mlaneStatus
  }

  mFlag1Check = new TH2I("Flag1Check", "Flag 1 Check", NFees, 0, NFees, 3, 0, 3); // Row 1 : transmission_timeout, Row 2 : packet_overflow, Row 3 : lane_starts_violation
  getObjectsManager()->startPublishing(mFlag1Check);                              //mFlag1Check

  mIndexCheck = new TH2I("IndexCheck", "Index Check", NFees, 0, NFees, 4, 0, 4);
  getObjectsManager()->startPublishing(mIndexCheck); //mIndexCheck

  mIdCheck = new TH2I("IdCheck", "Id Check", NFees, 0, NFees, 8, 0, 8);
  getObjectsManager()->startPublishing(mIdCheck); //mIdCheck
}

void ITSFeeTask::setAxisTitle(TH1* object, const char* xTitle, const char* yTitle)
{
  object->GetXaxis()->SetTitle(xTitle);
  object->GetYaxis()->SetTitle(yTitle);
}

void ITSFeeTask::setPlotsFormat()
{
  if (mTrigger) {
    setAxisTitle(mTrigger, "Trigger ID", "Counts");
    mTrigger->SetMinimum(0);
    mTrigger->SetFillColor(kBlue);
    for (int i = 0; i < mNTrigger; i++) {
      mTrigger->GetXaxis()->SetBinLabel(i + 1, mTriggerType[i]);
    }
  }

  if (mTFInfo) {
    setAxisTitle(mTFInfo, "STF ID", "Counts");
  }

  if (mTriggerVsFeeId) {
    setAxisTitle(mTriggerVsFeeId, "FeeID", "Trigger ID");
    mTriggerVsFeeId->SetMinimum(0);
    mTriggerVsFeeId->SetStats(0);
    for (int i = 0; i < mNTrigger; i++) {
      mTriggerVsFeeId->GetYaxis()->SetBinLabel(i + 1, mTriggerType[i]);
    }
  }

  if (mLaneInfo) {
    setAxisTitle(mLaneInfo, "Lane", "Flag");
  }

  if (mProcessingTime) {
    setAxisTitle(mProcessingTime, "STF", "Time (us)");
  }

  for (int i = 0; i < NFlags; i++) {
    if (mLaneStatus[i]) {
      setAxisTitle(mLaneStatus[i], "FEEID", "Lane");
    }
  }

  if (mFlag1Check) {
    setAxisTitle(mFlag1Check, "FEEID", "Flag");
  }

  if (mIndexCheck) {
    setAxisTitle(mIndexCheck, "FEEID", "Flag");
  }

  if (mIdCheck) {
    setAxisTitle(mIdCheck, "FEEID", "Flag");
  }
}

void ITSFeeTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity : " << activity.mId << ENDM;
  mRunNumber = activity.mId;
}

void ITSFeeTask::startOfCycle() { ILOG(Info) << "startOfCycle" << ENDM; }

void ITSFeeTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  std::chrono::time_point<std::chrono::high_resolution_clock> end;
  int difference;
  start = std::chrono::high_resolution_clock::now();

  std::vector<InputSpec> rawDataFilter{ InputSpec{ "", ConcreteDataTypeMatcher{ "DS", "RAWDATA0" }, Lifetime::Timeframe } };
  DPLRawParser parser(ctx.inputs(), rawDataFilter);
  for (auto it = parser.begin(), end = parser.end(); it != end; ++it) {
    auto const* rdh = it.get_if<o2::header::RAWDataHeaderV6>();
    int istave = (int)(rdh->feeId & 0x00ff);
    int ilink = (int)((rdh->feeId & 0x0f00) >> 8);
    int ilayer = (int)((rdh->feeId & 0xf000) >> 12);
    int ifee = 3 * StaveBoundary[ilayer] - (StaveBoundary[ilayer] - StaveBoundary[NLayerIB]) * (ilayer >= NLayerIB) + istave * (3 - (ilayer >= NLayerIB)) + ilink;

    if ((int)(rdh->stop) && it.size()) { //looking into the DDW0 from the closing packet
      auto const* ddw = reinterpret_cast<const GBTDiagnosticWord*>(it.data());
      uint64_t laneInfo = ddw->laneWord.laneBits.laneStatus;
      uint8_t flag1 = ddw->indexWord.indexBits.flag1;

      for (int i = 0; i < 3; i++) {
        if (flag1 >> i & 0x1) {
          mFlag1Check->Fill(ifee, i);
        }
      }

      uint8_t index = ddw->indexWord.indexBits.index;
      if (index != 0) {
        for (int i = 0; i < 4; i++) {
          if (index >> i & 0x1) {
            mIndexCheck->Fill(ifee, i);
          }
        }
      }

      uint8_t id = ddw->indexWord.indexBits.id;
      if (id != 0xe4) {
        for (int i = 0; i < 8; i++) {
          if (id >> i & 0x1) {
            mIdCheck->Fill(ifee, i);
          }
        }
      }

      for (int i = 0; i < NLanes; i++) {
        int laneValue = laneInfo >> (2 * i) & 0x3;
        if (laneValue) {
          mLaneStatus[laneValue]->Fill(ifee, i);
        }
      }
    }

    for (int i = 0; i < 13; i++) {
      if (((uint32_t)(rdh->triggerType) >> i & 1) == 1) {
        mTrigger->Fill(i + 1);
        mTriggerVsFeeId->Fill(ifee, i + 1);
      }
    }
  }

  mTimeFrameId = ctx.inputs().get<int>("G");

  mTFInfo->Fill(mTimeFrameId);
  end = std::chrono::high_resolution_clock::now();
  difference = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  ILOG(Debug) << "Processing time: " << difference << ", and TF ID == " << mTimeFrameId << ENDM;
  mProcessingTime->SetBinContent(mTimeFrameId, difference);
}

void ITSFeeTask::endOfCycle()
{
  getObjectsManager()->addMetadata(mTFInfo->GetName(), "Run", mRunNumber);
  getObjectsManager()->addMetadata(mTriggerVsFeeId->GetName(), "Run", mRunNumber);
  getObjectsManager()->addMetadata(mTrigger->GetName(), "Run", mRunNumber);
  ILOG(Info) << "endOfCycle" << ENDM;
}

void ITSFeeTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "endOfActivity" << ENDM;
}

void ITSFeeTask::resetGeneralPlots()
{
  mTFInfo->Reset();
  mTriggerVsFeeId->Reset();
  mTrigger->Reset();
}
void ITSFeeTask::reset()
{
  resetGeneralPlots();
  ILOG(Info) << "Reset" << ENDM;
}

} // namespace o2::quality_control_modules::its
