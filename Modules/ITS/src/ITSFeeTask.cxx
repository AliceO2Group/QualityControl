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
/// \author Pietro Fecchio
/// \author Antonio Palasciano
///

#include "ITS/ITSFeeTask.h"
#include "QualityControl/QcInfoLogger.h"
#include "Common/Utils.h"

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
  delete mTriggerVsFeeId_reset;
  delete mFlag1Check;
  delete mDecodingCheck;
  delete mProcessingTime;
  delete mPayloadSize;
  delete mLaneStatusSummaryIB;
  delete mLaneStatusSummaryML;
  delete mLaneStatusSummaryOL;
  delete mLaneStatusSummaryGlobal;
  delete mRDHSummary;
  delete mRDHSummaryCumulative;
  delete mTrailerCount;
  delete mTrailerCount_reset;
  delete mCalibrationWordCount;
  delete mCalibStage;
  delete mCalibLoop;
  delete mActiveLanes;
  for (int i = 0; i < NFlags; i++) {
    delete mLaneStatus[i];
    delete mLaneStatusCumulative[i];
  }
  for (int i = 0; i < 2; i++) {
    delete mLaneStatusOverview[i];
  }
  for (int i = 0; i < NLayer; i++) {
    delete mLaneStatusSummary[i];
  }

  // delete mInfoCanvas;
}

void ITSFeeTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initializing the ITSFeeTask" << ENDM;
  getParameters();
  createFeePlots();
  setPlotsFormat();
}

void ITSFeeTask::createFeePlots()
{

  mEmptyPayload = new TH1I("EmptyPayload", "Numer of orbits with empty payload", NFees, 0, NFees);
  getObjectsManager()->startPublishing(mEmptyPayload);

  mTrigger = new TH1I("TriggerFlag", "Trigger vs counts", mTriggerType.size(), 0.5, mTriggerType.size() + 0.5);
  getObjectsManager()->startPublishing(mTrigger); // mTrigger

  mTFInfo = new TH1I("STFInfo", "STF vs count", 10000, 0, 10000);
  getObjectsManager()->startPublishing(mTFInfo); // mTFInfo

  mProcessingTime = new TH1I("ProcessingTime", "Processing Time", 10000, 0, 10000); // last bin: overflow
  getObjectsManager()->startPublishing(mProcessingTime);                            // mProcessingTime

  mProcessingTime2 = new TH1D("ProcessingTime2", "Processing Time (last bin: overflow)", 30001, 0, 120004); // last bin: overflow
  getObjectsManager()->startPublishing(mProcessingTime2);                                                   // mProcessingTime

  mTriggerVsFeeId = new TH2I("TriggerVsFeeid", "Trigger count vs Trigger ID and Fee ID", NFees, 0, NFees, mTriggerType.size(), 0.5, mTriggerType.size() + 0.5);
  getObjectsManager()->startPublishing(mTriggerVsFeeId); // mTriggervsFeeId

  mTriggerVsFeeId_reset = new TH2I("TriggerVsFeeid_reset", "Trigger count vs Trigger ID and Fee ID", NFees, 0, NFees, mTriggerType.size(), 0.5, mTriggerType.size() + 0.5);
  getObjectsManager()->startPublishing(mTriggerVsFeeId_reset); // mTriggervsFeeId

  for (int i = 0; i < NFlags; i++) {
    mLaneStatus[i] = new TH2I(Form("LaneStatus/laneStatusFlag%s", mLaneStatusFlag[i].c_str()), Form("Lane Status Flag: %s", mLaneStatusFlag[i].c_str()), NFees, 0, NFees, NLanesMax, 0, NLanesMax);
    mLaneStatusCumulative[i] = new TH2I(Form("LaneStatus/laneStatusFlagCumulative%s", mLaneStatusFlag[i].c_str()), Form("Lane Status Flags since SOX: %s", mLaneStatusFlag[i].c_str()), NFees, 0, NFees, NLanesMax, 0, NLanesMax);
    getObjectsManager()->startPublishing(mLaneStatus[i]);           // mlaneStatus
    getObjectsManager()->startPublishing(mLaneStatusCumulative[i]); // mlaneStatus
  }

  mLaneStatusOverview[0] = new TH2Poly();
  mLaneStatusOverview[0]->SetName("LaneStatus/laneStatusOverviewFlagWARNING");

  mLaneStatusOverview[1] = new TH2Poly();
  mLaneStatusOverview[1]->SetName("LaneStatus/laneStatusOverviewFlagERROR");

  for (int i = 0; i < NLayer; i++) {
    mLaneStatusSummary[i] = new TH1I(Form("LaneStatusSummary/LaneStatusSummaryL%i", i), Form("Lane Status Summary L%i", i), 3, 0, 3);
    getObjectsManager()->startPublishing(mLaneStatusSummary[i]); // mLaneStatusSummary
  }

  mLaneStatusSummaryIB = new TH1D("LaneStatusSummary/LaneStatusSummaryIB", "Lane Status Summary IB", 3, 0, 3);
  getObjectsManager()->startPublishing(mLaneStatusSummaryIB); // mLaneStatusSummaryIB
  mLaneStatusSummaryML = new TH1D("LaneStatusSummary/LaneStatusSummaryML", "Lane Status Summary ML", 3, 0, 3);
  getObjectsManager()->startPublishing(mLaneStatusSummaryML); // mLaneStatusSummaryML
  mLaneStatusSummaryOL = new TH1D("LaneStatusSummary/LaneStatusSummaryOL", "Lane Status Summary OL", 3, 0, 3);
  getObjectsManager()->startPublishing(mLaneStatusSummaryOL); // mLaneStatusSummaryOL

  mLaneStatusSummaryGlobal = new TH1D("LaneStatusSummary/LaneStatusSummaryGlobal", "Lane Status Summary Global", 4, 0, 4);
  mLaneStatusSummaryGlobal->SetMaximum(1);
  TLine* mLaneStatusSummaryLine = new TLine(0, 0.1, 4, 0.1);
  mLaneStatusSummaryLine->SetLineStyle(9);
  mLaneStatusSummaryLine->SetLineColor(kRed);

  TLatex* mLaneStatusSummaryInfo = new TLatex(0.1, 0.11, Form("#bf{%s}", "Threshold value"));
  mLaneStatusSummaryInfo->SetTextSize(0.05);
  mLaneStatusSummaryInfo->SetTextFont(43);
  mLaneStatusSummaryInfo->SetTextColor(kRed);

  mLaneStatusSummaryGlobal->GetListOfFunctions()->Add(mLaneStatusSummaryLine);
  mLaneStatusSummaryGlobal->GetListOfFunctions()->Add(mLaneStatusSummaryInfo);
  getObjectsManager()->startPublishing(mLaneStatusSummaryGlobal); // mLaneStatusSummaryGlobal

  mFlag1Check = new TH2I("Flag1Check", "Flag 1 Check", NFees, 0, NFees, 3, 0, 3); // Row 1 : transmission_timeout, Row 2 : packet_overflow, Row 3 : lane_starts_violation
  getObjectsManager()->startPublishing(mFlag1Check);                              // mFlag1Check

  mDecodingCheck = new TH2I("DecodingCheck", "Error in parsing data", NFees, 0, NFees, 5, 0, 5); // 0: DataFormat not recognized, 1: DDW index != 0, 2: DDW wrong identifier, 3: IHW wrong identifier, 4: CDW wrong version -- adapt y range!
  getObjectsManager()->startPublishing(mDecodingCheck);

  mPayloadSize = new TH2F("PayloadSize", "Payload Size", NFees, 0, NFees, mNPayloadSizeBins, 0, 4.096e5);
  getObjectsManager()->startPublishing(mPayloadSize); // mPayloadSize

  mRDHSummary = new TH2I("RDHSummary", "Detector field in first and last page", NFees, 0, NFees, mRDHDetField.size(), 0, mRDHDetField.size());
  getObjectsManager()->startPublishing(mRDHSummary);

  mRDHSummaryCumulative = new TH2I("RDHSummaryCumulative", "Detector field in first and last page, since SOX", NFees, 0, NFees, mRDHDetField.size(), 0, mRDHDetField.size());
  getObjectsManager()->startPublishing(mRDHSummaryCumulative);

  mTrailerCount = new TH2I("TrailerCount", "Internal triggers per Orbit", NFees, 0, NFees, 21, -1, 20); // negative value if #ROF exceeds 20
  getObjectsManager()->startPublishing(mTrailerCount);

  mTrailerCount_reset = new TH2I("TrailerCount_reset", "Internal triggers per Orbit for last TF", NFees, 0, NFees, 21, -1, 20); // negative value if #ROF exceeds 20
  getObjectsManager()->startPublishing(mTrailerCount_reset);

  mActiveLanes = new TH2I("ActiveLanes", "Number of lanes enabled in IHW", NFees, 0, NFees, NLanesMax, 0, NLanesMax);
  getObjectsManager()->startPublishing(mActiveLanes);

  mCalibrationWordCount = new TH1I("CalibrationWordCount", "Calibration Data Word count", NFees, 0, NFees);

  mCalibStage = new TH2I("CalibStage", "Stage in calib scan (chip row number)", NFees, 0, NFees, 512, 0, 512);

  mCalibLoop = new TH2I("CalibLoop", "Calib loop (register value)", NFees, 0, NFees, 256, 0, 256);

  if (mDecodeCDW) {
    getObjectsManager()->startPublishing(mCalibrationWordCount);
    getObjectsManager()->startPublishing(mCalibLoop);
    getObjectsManager()->startPublishing(mCalibStage);
  }
}

void ITSFeeTask::setAxisTitle(TH1* object, const char* xTitle, const char* yTitle)
{
  object->GetXaxis()->SetTitle(xTitle);
  object->GetYaxis()->SetTitle(yTitle);
}

void ITSFeeTask::drawLayerName(TH2* histo2D)
{
  TLatex* t[NLayer];
  double minTextPosX[NLayer] = { 1, 42, 92, 150, 205, 275, 370 };
  for (int ilayer = 0; ilayer < NLayer; ilayer++) {
    t[ilayer] = new TLatex(minTextPosX[ilayer], 28.3, Form("Layer %d", ilayer));
    histo2D->GetListOfFunctions()->Add(t[ilayer]);
  }
  for (const int& lay : LayerBoundaryFEE) {
    auto l = new TLine(lay, 0, lay, histo2D->GetNbinsY());
    histo2D->GetListOfFunctions()->Add(l);
  }
}

void ITSFeeTask::setPlotsFormat()
{
  if (mTrigger) {
    setAxisTitle(mTrigger, "Trigger ID", "Counts");
    mTrigger->SetMinimum(0);
    mTrigger->SetFillColor(kBlue);
    for (int i = 0; i < mTriggerType.size(); i++) {
      mTrigger->GetXaxis()->SetBinLabel(i + 1, mTriggerType.at(i).second);
    }
  }

  if (mTFInfo) {
    setAxisTitle(mTFInfo, "STF ID", "Counts");
  }

  if (mTriggerVsFeeId) {
    setAxisTitle(mTriggerVsFeeId, "FeeID", "Trigger ID");
    mTriggerVsFeeId->SetMinimum(0);
    mTriggerVsFeeId->SetStats(0);
    for (int i = 0; i < mTriggerType.size(); i++) {
      mTriggerVsFeeId->GetYaxis()->SetBinLabel(i + 1, mTriggerType.at(i).second);
    }
    setAxisTitle(mTriggerVsFeeId_reset, "FeeID", "Trigger ID");
    mTriggerVsFeeId_reset->SetMinimum(0);
    mTriggerVsFeeId_reset->SetStats(0);
    for (int i = 0; i < mTriggerType.size(); i++) {
      mTriggerVsFeeId_reset->GetYaxis()->SetBinLabel(i + 1, mTriggerType.at(i).second);
    }
  }

  if (mProcessingTime) {
    setAxisTitle(mProcessingTime, "STF", "Time (us)");
  }

  if (mProcessingTime2) {
    setAxisTitle(mProcessingTime2, "Time (us)", "STF count");
  }

  if (mRDHSummary) {
    setAxisTitle(mRDHSummary, "QC FEEId", "");
    mRDHSummary->SetStats(0);
    for (int idf = 0; idf < mRDHDetField.size(); idf++) {
      mRDHSummary->GetYaxis()->SetBinLabel(idf + 1, mRDHDetField.at(idf).second);
    }
    drawLayerName(mRDHSummary);
  }

  if (mRDHSummaryCumulative) {
    setAxisTitle(mRDHSummaryCumulative, "QC FEEId", "");
    mRDHSummaryCumulative->SetStats(0);
    for (int idf = 0; idf < mRDHDetField.size(); idf++) {
      mRDHSummaryCumulative->GetYaxis()->SetBinLabel(idf + 1, mRDHDetField.at(idf).second);
    }
    drawLayerName(mRDHSummaryCumulative);
  }

  if (mTrailerCount) {
    setAxisTitle(mTrailerCount, "QC FEEId", "Estimated ROF frequenccy");
    mTrailerCount->SetStats(0);
    mTrailerCount->GetYaxis()->SetBinLabel(3, "11 kHz");
    mTrailerCount->GetYaxis()->SetBinLabel(6, "45 kHz");
    mTrailerCount->GetYaxis()->SetBinLabel(8, "67 kHz");
    mTrailerCount->GetYaxis()->SetBinLabel(11, "101 kHz");
    mTrailerCount->GetYaxis()->SetBinLabel(15, "135 kHz");
    mTrailerCount->GetYaxis()->SetBinLabel(20, "202 kHz");

    setAxisTitle(mTrailerCount_reset, "QC FEEId", "Estimated ROF frequenccy");
    mTrailerCount_reset->SetStats(0);
    mTrailerCount_reset->GetYaxis()->SetBinLabel(3, "11 kHz");
    mTrailerCount_reset->GetYaxis()->SetBinLabel(6, "45 kHz");
    mTrailerCount_reset->GetYaxis()->SetBinLabel(8, "67 kHz");
    mTrailerCount_reset->GetYaxis()->SetBinLabel(11, "101 kHz");
    mTrailerCount_reset->GetYaxis()->SetBinLabel(15, "135 kHz");
    mTrailerCount_reset->GetYaxis()->SetBinLabel(20, "202 kHz");
  }

  if (mCalibrationWordCount) {
    setAxisTitle(mCalibrationWordCount, "QC FEEId", "Number of decoded CDWs");
  }

  if (mCalibStage) {
    setAxisTitle(mCalibStage, "QC FEEId", "Chip row number");
  }

  if (mCalibLoop) {
    setAxisTitle(mCalibLoop, "QC FEEId", "DAC register value");
  }

  if (mActiveLanes) {
    setAxisTitle(mActiveLanes, "QC FEEId", "Number of enabled lanes");
  }

  for (int i = 0; i < NFlags; i++) {
    if (mLaneStatus[i]) {
      setAxisTitle(mLaneStatus[i], "QC FEEId", "Lane");
      mLaneStatus[i]->SetStats(0);
      drawLayerName(mLaneStatus[i]);
      setAxisTitle(mLaneStatusCumulative[i], "QC FEEId", "Lane");
      mLaneStatusCumulative[i]->SetStats(0);
      drawLayerName(mLaneStatusCumulative[i]);
    }
  }

  for (int i = 0; i < 2; i++) {
    TString title = (i == 0) ? "Fraction of lanes into WARNING" : "Fraction of lanes in Not OK status";
    title += ";mm (IB 3x);mm (IB 3x)";
    mLaneStatusOverview[i]->SetTitle(title);
    mLaneStatusOverview[i]->SetStats(0);
    mLaneStatusOverview[i]->SetOption("lcolz");
    mLaneStatusOverview[i]->SetMinimum(0);
    mLaneStatusOverview[i]->SetMaximum(1);
    mLaneStatusOverview[i]->SetBit(TH1::kIsAverage);
    for (int ilayer = 0; ilayer < 7; ilayer++) {
      for (int istave = 0; istave < NStaves[ilayer]; istave++) {
        double* px = new double[4];
        double* py = new double[4];
        getStavePoint(ilayer, istave, px, py);
        if (ilayer < 3) {
          for (int icoo = 0; icoo < 4; icoo++) {
            px[icoo] *= 3.;
            py[icoo] *= 3.;
          }
        }
        mLaneStatusOverview[i]->AddBin(4, px, py);
      }
    }
    getObjectsManager()->startPublishing(mLaneStatusOverview[i]); // mLaneStatusOverview
  }

  for (int i = 0; i < NLayer; i++) {
    if (mLaneStatusSummary[i]) {
      setAxisTitle(mLaneStatusSummary[i], "", "#Lanes");
      for (int j = 0; j < NFlags; j++) {
        mLaneStatusSummary[i]->GetXaxis()->SetBinLabel(j + 1, mLaneStatusFlag[j].c_str());
      }
      mLaneStatusSummary[i]->GetXaxis()->CenterLabels();
      mLaneStatusSummary[i]->SetStats(0);
    }
  }

  if (mLaneStatusSummaryIB) {
    setAxisTitle(mLaneStatusSummaryIB, "", "Fraction of Lanes");
    for (int i = 0; i < NFlags; i++) {
      mLaneStatusSummaryIB->GetXaxis()->SetBinLabel(i + 1, mLaneStatusFlag[i].c_str());
    }
    mLaneStatusSummaryIB->GetXaxis()->CenterLabels();
    mLaneStatusSummaryIB->SetStats(0);
  }

  if (mLaneStatusSummaryML) {
    setAxisTitle(mLaneStatusSummaryML, "", "Fraction of Lanes");
    for (int i = 0; i < NFlags; i++) {
      mLaneStatusSummaryML->GetXaxis()->SetBinLabel(i + 1, mLaneStatusFlag[i].c_str());
    }
    mLaneStatusSummaryML->GetXaxis()->CenterLabels();
    mLaneStatusSummaryML->SetStats(0);
  }

  if (mLaneStatusSummaryOL) {
    setAxisTitle(mLaneStatusSummaryOL, "", "Fraction of Lanes");
    for (int i = 0; i < NFlags; i++) {
      mLaneStatusSummaryOL->GetXaxis()->SetBinLabel(i + 1, mLaneStatusFlag[i].c_str());
    }
    mLaneStatusSummaryOL->GetXaxis()->CenterLabels();
    mLaneStatusSummaryOL->SetStats(0);
  }

  if (mLaneStatusSummaryGlobal) {
    setAxisTitle(mLaneStatusSummaryGlobal, "", "Fraction Lanes");
    for (int i = 0; i < NFlags; i++) {
      mLaneStatusSummaryGlobal->GetXaxis()->SetBinLabel(i + 1, mLaneStatusFlag[i].c_str());
    }
    mLaneStatusSummaryGlobal->GetXaxis()->SetBinLabel(4, "TOTAL");
    mLaneStatusSummaryGlobal->GetXaxis()->CenterLabels();
    mLaneStatusSummaryGlobal->SetStats(0);
  }

  if (mFlag1Check) {
    setAxisTitle(mFlag1Check, "QC FEEId", "Flag");
  }

  if (mDecodingCheck) {
    setAxisTitle(mDecodingCheck, "QC FEEId", "Error ID");
  }

  if (mPayloadSize) {
    setAxisTitle(mPayloadSize, "QC FEEId", "Avg. Payload size");
    mPayloadSize->SetStats(0);
    drawLayerName(mPayloadSize);
  }
}

void ITSFeeTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity : " << activity.mId << ENDM;
}

void ITSFeeTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;

  if (nCycleID % nResetCycle == 0) {
    mTrailerCount_reset->Reset();
    mTriggerVsFeeId_reset->Reset();
  }
  nCycleID++;
}

void ITSFeeTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // set timer
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  std::chrono::time_point<std::chrono::high_resolution_clock> end;
  int difference;
  start = std::chrono::high_resolution_clock::now();

  int nStops[NFees] = {};
  int payloadTot[NFees] = {};
  int TDTcounter[NFees] = {};

  DPLRawParser parser(ctx.inputs());

  resetLanePlotsAndCounters(false); // not full reset // action taken depending on mResetLaneStatus and mResetPayload

  // manual call of DPL data iterator to catch exceptoin:
  try {
    auto it = parser.begin();
  } catch (const std::runtime_error& error) {
    ILOG(Error, Support) << "Error during parsing DPL data: " << error.what() << ENDM;
    return;
  }

  for (auto it = parser.begin(), end = parser.end(); it != end; ++it) {

    auto rdh = reinterpret_cast<const o2::header::RDHAny*>(it.raw());
    // Decoding data format (RDHv* --> v6 and v7 have same bits for what is considered here)
    auto feeID = o2::raw::RDHUtils::getFEEID(rdh);
    int istave = (int)(feeID & 0x00ff);
    int ilink = (int)((feeID & 0x0f00) >> 8);
    int ilayer = (int)((feeID & 0xf000) >> 12);
    int ifee = 3 * StaveBoundary[ilayer] - (StaveBoundary[ilayer] - StaveBoundary[NLayerIB]) * (ilayer >= NLayerIB) + istave * (3 - (ilayer >= NLayerIB)) + ilink;
    int memorysize = (int)(o2::raw::RDHUtils::getMemorySize(rdh));
    int headersize = o2::raw::RDHUtils::getHeaderSize(rdh);
    payloadTot[ifee] += memorysize - headersize;
    bool clockEvt = false;
    bool RecoveryOngoing = false;
    bool RampOngoing = false;

    // Operations at first or last page of the orbit:
    //   - detector field decoding
    if ((int)(o2::raw::RDHUtils::getStop(rdh)) || (int)(o2::raw::RDHUtils::getPageCounter(rdh)) == 0) {

      uint32_t summaryLaneStatus = o2::raw::RDHUtils::getDetectorField(rdh);

      for (int ibin = 0; ibin < mRDHDetField.size(); ibin++) {
        if (summaryLaneStatus & (1 << mRDHDetField.at(ibin).first)) {
          mRDHSummary->Fill(ifee, ibin);
          mRDHSummaryCumulative->Fill(ifee, ibin);
          TString description = mRDHDetField.at(ibin).second;
          if (description == "ClockEvt") {
            clockEvt = true;
          }
          if (description == "TriggerRamp") {
            RampOngoing = true;
          }
          if (description == "Recovery") {
            RampOngoing = RecoveryOngoing = true;
          }
        }
      }
    }

    // Operations at any page inside the enabled orbit:
    //   - decoding identifier of each payload word and increasing counter of TDTs (if enabled)
    //   - decoding identifier of each payload word and decode full CDWs (if enabled)

    bool doLookForTDT = (mPayloadParseEvery_n_HBF_per_TF > 0) && (nStops[ifee] % mPayloadParseEvery_n_HBF_per_TF == 0) && (mTimeFrameId % mPayloadParseEvery_n_TF == 0);

    if (doLookForTDT || mDecodeCDW) {
      int dataformat = (int)o2::raw::RDHUtils::getDataFormat(rdh);
      if (dataformat != 0 && dataformat != 2) {
        mDecodingCheck->Fill(ifee, 0);
      }
      auto const* payload = it.data();
      size_t payloadSize = it.size();
      int PayloadPerGBTW = (dataformat < 2) ? 16 : 10;
      int PaddingBytes = PayloadPerGBTW - 10;
      const uint16_t* gbtw_bb; // identifier and byte before

      for (int32_t ip = PayloadPerGBTW; ip <= payloadSize; ip += PayloadPerGBTW) {
        gbtw_bb = (const uint16_t*)&payload[ip - PaddingBytes - 2];
        if (doLookForTDT && (*gbtw_bb & 0xff01) == 0xf001) { // checking that it is a TDT (0xf0) with packet_done (0x<any>1)
          TDTcounter[ifee]++;
        }
        if (mDecodeCDW && (*gbtw_bb & 0xff00) == 0xf800) { // chedking that it is a CDW (0xf8)
          const CalibrationWordUserField* cdw;
          try {
            cdw = reinterpret_cast<const CalibrationWordUserField*>(&payload[ip - PayloadPerGBTW]);
          } catch (const std::runtime_error& error) {
            ILOG(Error, Support) << "Error during reading of calibration data word: " << error.what() << ENDM;
            return;
          }

          mCalibrationWordCount->Fill(ifee);
          if (cdw->userField2.content.cdwver == 1) {
            mCalibStage->Fill(ifee, (int)(cdw->userField0.content.rowid));
            mCalibLoop->Fill(ifee, (int)(cdw->userField1.content.loopvalue));
          } else { // TODO: add compatibility to older versions of CDW
            mDecodingCheck->Fill(ifee, 4);
          }

        } // if mDecodeCDW and it is CDW

      } // loop payload

    } // if doLookForTDT || mDecodeCDW

    //

    // Operations at the first page of each orbit
    //  - decoding ITS header work and fill histogram with number of active lanes
    if (mEnableIHWReading) {
      if ((int)(o2::raw::RDHUtils::getPageCounter(rdh)) == 0) {
        const GBTITSHeaderWord* ihw;
        try {
          ihw = reinterpret_cast<const GBTITSHeaderWord*>(it.data());
        } catch (const std::runtime_error& error) {
          ILOG(Error, Support) << "Error during reading of its header data: " << error.what() << ENDM;
          return;
        }

        uint8_t ihwID = ihw->indexWord.indexBits.id;

        if (ihwID != 0xe0) {
          mDecodingCheck->Fill(ifee, 3);
        }

        uint32_t activelaneMask = ihw->IHWcontent.laneBits.activeLanes;

        int nactivelanes = 0;
        for (int i = 0; i < NLanesMax; i++) {
          nactivelanes += ((activelaneMask >> i) & 0x1);
        }

        // if (!RecoveryOngoing) use it if we want a cleaner situation for the checker
        mActiveLanes->Fill(ifee, nactivelanes);
      }
    }

    // Operations at last page of each orbit:

    if ((int)(o2::raw::RDHUtils::getStop(rdh)) && !it.size()) {
      mEmptyPayload->Fill(ifee);
    }

    //  - decoding Diagnostic Word DDW0 and fill lane status plots and vectors
    if ((int)(o2::raw::RDHUtils::getStop(rdh)) && it.size()) {

      //  - read triggers in RDH and fill histogram
      //  - fill histogram with packet_done TDTs counted so far and reset counter
      // fill trailer count histo and reset counters
      if (doLookForTDT) {

        if (!RampOngoing && !clockEvt) {
          mTrailerCount->Fill(ifee, TDTcounter[ifee] < 21 ? TDTcounter[ifee] : -1);
          mTrailerCount_reset->Fill(ifee, TDTcounter[ifee] < 21 ? TDTcounter[ifee] : -1);
        }
        TDTcounter[ifee] = 0;
      }

      nStops[ifee]++;
      for (int i = 0; i < mTriggerType.size(); i++) {
        if (((o2::raw::RDHUtils::getTriggerType(rdh)) >> mTriggerType.at(i).first & 1) == 1) {
          mTrigger->Fill(i + 1);
          mTriggerVsFeeId->Fill(ifee, i + 1);
          mTriggerVsFeeId_reset->Fill(ifee, i + 1);
        }
      }

      if (precisePayload) {
        mPayloadSize->Fill(ifee, payloadTot[ifee]);
        payloadTot[ifee] = 0;
      }

      const GBTDiagnosticWord* ddw;
      try {
        ddw = reinterpret_cast<const GBTDiagnosticWord*>(it.data());
      } catch (const std::runtime_error& error) {
        ILOG(Error, Support) << "Error during reading late diagnostic data: " << error.what() << ENDM;
        return;
      }

      uint64_t laneInfo = ddw->laneWord.laneBits.laneStatus;

      uint8_t flag1 = ddw->indexWord.indexBits.flag1;
      for (int i = 0; i < 3; i++) {
        if (flag1 >> i & 0x1) {
          mFlag1Check->Fill(ifee, i);
        }
      }

      uint8_t index = ddw->indexWord.indexBits.index;
      if (index != 0) {
        mDecodingCheck->Fill(ifee, 1);
      }

      uint8_t id = ddw->indexWord.indexBits.id;
      if (id != 0xe4) {
        mDecodingCheck->Fill(ifee, 2);
      }

      for (int i = 0; i < NLanesMax; i++) {
        int laneValue = laneInfo >> (2 * i) & 0x3;
        if (laneValue) {
          mStatusFlagNumber[ilayer][istave][i][laneValue - 1]++;
          mLaneStatus[laneValue - 1]->Fill(ifee, i);
          mLaneStatusCumulative[laneValue - 1]->Fill(ifee, i);
        }
      }

      for (int iflag = 0; iflag < 3; iflag++) {
        if (clockEvt) {
          int feeInStave = (ifee - feeBoundary[ilayer]) - (feePerStave[ilayer] * (istave));
          int startingLane = (feeInStave - 1) * lanesPerFeeId[ilayer];
          for (int indexLaneFee = indexFeeLow[ilayer]; indexLaneFee < indexFeeUp[ilayer]; indexLaneFee++) {
            mLaneStatus[iflag]->Fill(ifee, indexLaneFee);
            mLaneStatusCumulative[iflag]->Fill(ifee, indexLaneFee);
          }
          for (int indexLane = startingLane; indexLane < (startingLane + lanesPerFeeId[ilayer]); indexLane++) {
            mStatusFlagNumber[ilayer][istave][indexLane][iflag]++;
          }
        }
      }
    }
  }

  // Filling histograms: loop over mStatusFlagNumber[ilayer][istave][ilane][iflag]
  int counterSummary[4][3] = { { 0 } };
  int layerSummary[7][3] = { { 0 } };

  int mapLayerToBarrel[7] = { 1, 1, 1, 2, 2, 3, 3 };

  for (int iflag = 0; iflag < NFlags; iflag++) {
    for (int ilayer = 0; ilayer < NLayer; ilayer++) {
      for (int istave = 0; istave < NStaves[ilayer]; istave++) {
        for (int ilane = 0; ilane < NLanesMax; ilane++) {
          if (mStatusFlagNumber[ilayer][istave][ilane][iflag] > 0) {
            counterSummary[0][iflag]++;
            counterSummary[mapLayerToBarrel[ilayer]][iflag]++; // IB, ML, OL
            layerSummary[ilayer][iflag]++;
          }
        }
      }
      mLaneStatusSummary[ilayer]->SetBinContent(iflag + 1, layerSummary[ilayer][iflag]);
    }
    mLaneStatusSummaryGlobal->SetBinContent(iflag + 1, 1. * counterSummary[0][iflag] / NLanesTotal);
    mLaneStatusSummaryIB->SetBinContent(iflag + 1, 1. * counterSummary[1][iflag] / NLanesIB);
    mLaneStatusSummaryML->SetBinContent(iflag + 1, 1. * counterSummary[2][iflag] / NLanesML);
    mLaneStatusSummaryOL->SetBinContent(iflag + 1, 1. * counterSummary[3][iflag] / NLanesOL);
  }
  mLaneStatusSummaryGlobal->SetBinContent(4, 1. * (counterSummary[0][0] + counterSummary[0][1] + counterSummary[0][2]) / NLanesTotal);

  for (int ilayer = 0; ilayer < NLayer; ilayer++) {
    for (int istave = 0; istave < NStaves[ilayer]; istave++) {
      int countWarning = 0;
      int countNOK = 0;
      for (int ilane = 0; ilane < NLanesMax; ilane++) {
        if (mStatusFlagNumber[ilayer][istave][ilane][0] > 0)
          countWarning++;
        if (mStatusFlagNumber[ilayer][istave][ilane][1] > 0 || (mStatusFlagNumber[ilayer][istave][ilane][2] > 0))
          countNOK++;
      }
      mLaneStatusOverview[0]->SetBinContent(istave + 1 + StaveBoundary[ilayer], (float)(countWarning) / (float)(NLanePerStaveLayer[ilayer]));
      mLaneStatusOverview[0]->SetBinError(istave + 1 + StaveBoundary[ilayer], 1e-15);
      mLaneStatusOverview[1]->SetBinContent(istave + 1 + StaveBoundary[ilayer], (float)(countNOK) / (float)(NLanePerStaveLayer[ilayer]));
      mLaneStatusOverview[1]->SetBinError(istave + 1 + StaveBoundary[ilayer], 1e-15);
    }
  }

  if (!precisePayload) {
    for (int i = 0; i < NFees; i++) {
      if (nStops[i]) {
        float payloadAvg = (float)payloadTot[i] / nStops[i];
        mPayloadSize->Fill(i, payloadAvg);
      }
    }
  }

  mTimeFrameId++;
  mTFInfo->Fill(mTimeFrameId % 10000);
  end = std::chrono::high_resolution_clock::now();
  difference = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  mProcessingTime2->Fill(difference < 120000 ? difference : 120001);
  if (mTimeFrameId < 10000) {
    mProcessingTime->SetBinContent(mTimeFrameId, difference);
  }
}

void ITSFeeTask::getParameters()
{
  mNPayloadSizeBins = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "NPayloadSizeBins", mNPayloadSizeBins);
  mResetLaneStatus = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "ResetLaneStatus", mResetLaneStatus);
  mResetPayload = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "ResetPayload", mResetPayload);
  mPayloadParseEvery_n_HBF_per_TF = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "PayloadParsingEvery_n_HBFperTF", mPayloadParseEvery_n_HBF_per_TF);
  mPayloadParseEvery_n_TF = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "PayloadParsingEvery_n_TF", mPayloadParseEvery_n_TF);
  mEnableIHWReading = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "EnableIHWReading", mEnableIHWReading);
  mDecodeCDW = o2::quality_control_modules::common::getFromConfig<bool>(mCustomParameters, "DecodeCDW", mDecodeCDW);
  nResetCycle = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "nResetCycle", nResetCycle);
  precisePayload = o2::quality_control_modules::common::getFromConfig<bool>(mCustomParameters, "precisePayload", precisePayload);
}

void ITSFeeTask::getStavePoint(int layer, int stave, double* px, double* py)
{
  float stepAngle = TMath::Pi() * 2 / NStaves[layer];             // the angle between to stave
  float midAngle = StartAngle[layer] + (stave * stepAngle);       // mid point angle
  float staveRotateAngle = TMath::Pi() / 2 - (stave * stepAngle); // how many angle this stave rotate(compare with first stave)
  px[1] = MidPointRad[layer] * TMath::Cos(midAngle);              // there are 4 point to decide this TH2Poly bin
                                                                  // 0:left point in this stave;
                                                                  // 1:mid point in this stave;
                                                                  // 2:right point in this stave;
                                                                  // 3:higher point int this stave;
  py[1] = MidPointRad[layer] * TMath::Sin(midAngle);              // 4 point calculated accord the blueprint
                                                                  // roughly calculate
  if (layer < NLayerIB) {
    px[0] = 7.7 * TMath::Cos(staveRotateAngle) + px[1];
    py[0] = -7.7 * TMath::Sin(staveRotateAngle) + py[1];
    px[2] = -7.7 * TMath::Cos(staveRotateAngle) + px[1];
    py[2] = 7.7 * TMath::Sin(staveRotateAngle) + py[1];
    px[3] = 5.623 * TMath::Sin(staveRotateAngle) + px[1];
    py[3] = 5.623 * TMath::Cos(staveRotateAngle) + py[1];
  } else {
    px[0] = 21 * TMath::Cos(staveRotateAngle) + px[1];
    py[0] = -21 * TMath::Sin(staveRotateAngle) + py[1];
    px[2] = -21 * TMath::Cos(staveRotateAngle) + px[1];
    py[2] = 21 * TMath::Sin(staveRotateAngle) + py[1];
    px[3] = 40 * TMath::Sin(staveRotateAngle) + px[1];
    py[3] = 40 * TMath::Cos(staveRotateAngle) + py[1];
  }
}

void ITSFeeTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void ITSFeeTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void ITSFeeTask::resetGeneralPlots()
{
  mTFInfo->Reset();
  mTriggerVsFeeId->Reset();
  mTrigger->Reset();
}

void ITSFeeTask::resetLanePlotsAndCounters(bool isFullReset)
{
  if (mResetLaneStatus || isFullReset) {
    mRDHSummary->Reset("ICES"); // option ICES is to not remove layer lines and labels
    mFlag1Check->Reset();
    mLaneStatusSummaryIB->Reset();
    mLaneStatusSummaryML->Reset();
    mLaneStatusSummaryOL->Reset();
    mLaneStatusSummaryGlobal->Reset("ICES");
    for (int i = 0; i < NFlags; i++) {
      mLaneStatus[i]->Reset("ICES");
    }
    mLaneStatusOverview[0]->Reset("content");
    mLaneStatusOverview[1]->Reset("content");
    for (int i = 0; i < NLayer; i++) {
      mLaneStatusSummary[i]->Reset();
    }

    memset(mStatusFlagNumber, 0, sizeof(mStatusFlagNumber)); // reset counters
  }
  if (mResetPayload || isFullReset) {
    mPayloadSize->Reset("ICES");
  }
}

void ITSFeeTask::reset()
{
  // it is expected that this reset function will be executed only at the end of run
  resetGeneralPlots();
  resetLanePlotsAndCounters(true); // full reset of all plots
  mTimeFrameId = 0;
  mDecodingCheck->Reset();
  mRDHSummaryCumulative->Reset();
  mTrailerCount->Reset();
  mActiveLanes->Reset();
  for (int i = 0; i < NFlags; i++) {
    mLaneStatusCumulative[i]->Reset("ICES");
  }
  mProcessingTime->Reset();
  mProcessingTime2->Reset();
  ILOG(Debug, Devel) << "Reset" << ENDM;

  if (mDecodeCDW) {
    mCalibrationWordCount->Reset();
    mCalibLoop->Reset();
    mCalibStage->Reset();
  }
}

} // namespace o2::quality_control_modules::its
