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
#include <iostream>

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
  delete mPayloadSize;
  delete mLaneStatusSummaryIB;
  delete mLaneStatusSummaryML;
  delete mLaneStatusSummaryOL;
  delete mLaneStatusSummaryGlobal;
  delete mRDHSummary;
  for (int i = 0; i < NFlags; i++) {
    delete mLaneStatus[i];
    delete mLaneStatusCumulative[i];
  }
  for (int i = 0; i < NFlags; i++) {
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
  mTrigger = new TH1I("TriggerFlag", "Trigger vs counts", NTrigger, 0.5, NTrigger + 0.5);
  getObjectsManager()->startPublishing(mTrigger); // mTrigger

  mTFInfo = new TH1I("STFInfo", "STF vs count", 15000, 0, 15000);
  getObjectsManager()->startPublishing(mTFInfo); // mTFInfo

  mLaneInfo = new TH2I("LaneInfo", "Lane Information", NLanesMax, -.5, NLanesMax - 0.5, NFlags, -.5, NFlags - 0.5);
  getObjectsManager()->startPublishing(mLaneInfo); // mLaneInfo

  mProcessingTime = new TH1I("ProcessingTime", "Processing Time", 10000, 0, 10000);
  getObjectsManager()->startPublishing(mProcessingTime); // mProcessingTime

  mTriggerVsFeeId = new TH2I("TriggerVsFeeid", "Trigger count vs Trigger ID and Fee ID", NFees, 0, NFees, NTrigger, 0.5, NTrigger + 0.5);
  getObjectsManager()->startPublishing(mTriggerVsFeeId); // mTriggervsFeeId

  for (int i = 0; i < NFlags; i++) {
    mLaneStatus[i] = new TH2I(Form("LaneStatus/laneStatusFlag%s", mLaneStatusFlag[i].c_str()), Form("Lane Status Flag: %s", mLaneStatusFlag[i].c_str()), NFees, 0, NFees, NLanesMax, 0, NLanesMax);
    mLaneStatusCumulative[i] = new TH2I(Form("LaneStatus/laneStatusFlagCumulative%s", mLaneStatusFlag[i].c_str()), Form("Lane Status Flags since SOX: %s", mLaneStatusFlag[i].c_str()), NFees, 0, NFees, NLanesMax, 0, NLanesMax);
    getObjectsManager()->startPublishing(mLaneStatus[i]);           // mlaneStatus
    getObjectsManager()->startPublishing(mLaneStatusCumulative[i]); // mlaneStatus
  }

  for (int i = 0; i < NFlags; i++) {
    mLaneStatusOverview[i] = new TH2Poly();
    mLaneStatusOverview[i]->SetName(Form("LaneStatus/laneStatusOverviewFlag%s", mLaneStatusFlag[i].c_str()));
  }

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

  mIndexCheck = new TH2I("IndexCheck", "Index Check", NFees, 0, NFees, 4, 0, 4);
  getObjectsManager()->startPublishing(mIndexCheck); // mIndexCheck

  mIdCheck = new TH2I("IdCheck", "Id Check", NFees, 0, NFees, 8, 0, 8);
  getObjectsManager()->startPublishing(mIdCheck); // mIdCheck

  mPayloadSize = new TH2F("PayloadSize", "Payload Size", NFees, 0, NFees, mNPayloadSizeBins, 0, 4.096e4);
  getObjectsManager()->startPublishing(mPayloadSize); // mPayloadSize

  mRDHSummary = new TH2I("RDHSummary", "RDH Summary", NFees, 0, NFees, 7, 0, 7);
  getObjectsManager()->startPublishing(mRDHSummary);
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
    for (int i = 0; i < NTrigger; i++) {
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
    for (int i = 0; i < NTrigger; i++) {
      mTriggerVsFeeId->GetYaxis()->SetBinLabel(i + 1, mTriggerType[i]);
    }
  }

  if (mLaneInfo) {
    setAxisTitle(mLaneInfo, "Lane", "Flag");
  }

  if (mProcessingTime) {
    setAxisTitle(mProcessingTime, "STF", "Time (us)");
  }

  // Defining RDH summary histogram
  if (mRDHSummary) {
    setAxisTitle(mRDHSummary, "FEEId", "");
    mRDHSummary->SetStats(0);
    mRDHSummary->GetYaxis()->SetBinLabel(1, "Missing data");
    mRDHSummary->GetYaxis()->SetBinLabel(2, "Warning");
    mRDHSummary->GetYaxis()->SetBinLabel(3, "Error");
    mRDHSummary->GetYaxis()->SetBinLabel(4, "Fault");
    mRDHSummary->GetYaxis()->SetBinLabel(5, "ClockEvent");
    mRDHSummary->GetYaxis()->SetBinLabel(6, "TimebaseEvent");
    mRDHSummary->GetYaxis()->SetBinLabel(7, "TimebaseUnsyncEvent");
    drawLayerName(mRDHSummary);
  }

  for (int i = 0; i < NFlags; i++) {
    if (mLaneStatus[i]) {
      setAxisTitle(mLaneStatus[i], "FEEID", "Lane");
      mLaneStatus[i]->SetStats(0);
      drawLayerName(mLaneStatus[i]);
      setAxisTitle(mLaneStatusCumulative[i], "FEEID", "Lane");
      mLaneStatusCumulative[i]->SetStats(0);
      drawLayerName(mLaneStatusCumulative[i]);
    }
  }

  for (int i = 0; i < NFlags; i++) {
    TString title = Form("Fraction of lanes into %s", mLaneStatusFlag[i].c_str());
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
    setAxisTitle(mFlag1Check, "FEEID", "Flag");
  }

  if (mIndexCheck) {
    setAxisTitle(mIndexCheck, "FEEID", "Flag");
  }

  if (mPayloadSize) {
    setAxisTitle(mPayloadSize, "FEEID", "Avg. Payload size");
    mPayloadSize->SetStats(0);
    drawLayerName(mPayloadSize);
  }

  if (mIdCheck) {
    setAxisTitle(mIdCheck, "FEEID", "Flag");
  }
}

void ITSFeeTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity : " << activity.mId << ENDM;
  mRunNumber = activity.mId;
}

void ITSFeeTask::startOfCycle() { ILOG(Debug, Devel) << "startOfCycle" << ENDM; }

void ITSFeeTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // set timer
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  std::chrono::time_point<std::chrono::high_resolution_clock> end;
  int difference;
  start = std::chrono::high_resolution_clock::now();

  int nStops[NFees] = {};
  int payloadTot[NFees] = {};

  DPLRawParser parser(ctx.inputs());

  resetLanePlotsAndCounters(); // action taken depending on mResetLaneStatus and mResetPayload

  // manual call of DPL data iterator to catch exceptoin:
  try {
    auto it = parser.begin();
  } catch (const std::runtime_error& error) {
    LOG(error) << "Error during parsing DPL data: " << error.what();
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

    // RDHSummaryPlot
    //  get detector field
    uint32_t summaryLaneStatus = o2::raw::RDHUtils::getDetectorField(rdh);
    // fill statusVsFeeId if set
    if (summaryLaneStatus & (1 << 0))
      mRDHSummary->Fill(ifee, 0); // missing data
    if (summaryLaneStatus & (1 << 1))
      mRDHSummary->Fill(ifee, 1); // warning
    if (summaryLaneStatus & (1 << 2))
      mRDHSummary->Fill(ifee, 2); // error
    if (summaryLaneStatus & (1 << 3))
      mRDHSummary->Fill(ifee, 3); // fault
    if (summaryLaneStatus & (1 << 26)) {
      mRDHSummary->Fill(ifee, 4); // clock evt
      clockEvt = true;
    }
    if (summaryLaneStatus & (1 << 25))
      mRDHSummary->Fill(ifee, 5); // Timebase evt
    if (summaryLaneStatus & (1 << 24))
      mRDHSummary->Fill(ifee, 6);                              // Timebase Unsync evt
    if ((int)(o2::raw::RDHUtils::getStop(rdh)) && it.size()) { // looking into the DDW0 from the closing packet
      const GBTDiagnosticWord* ddw;
      try {
        ddw = reinterpret_cast<const GBTDiagnosticWord*>(it.data());
      } catch (const std::runtime_error& error) {
        LOG(error) << "Error during reading late diagnostic data: " << error.what();
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

    if ((int)(o2::raw::RDHUtils::getStop(rdh))) {
      nStops[ifee]++;
      for (int i = 0; i < 13; i++) {
        if (((o2::raw::RDHUtils::getTriggerType(rdh)) >> i & 1) == 1) {
          mTrigger->Fill(i + 1);
          mTriggerVsFeeId->Fill(ifee, i + 1);
        }
      }
    }
  }

  // Filling histograms: loop over mStatusFlagNumber[ilayer][istave][ilane][iflag]
  int counterSummary[4][3] = { { 0 } };
  int layerSummary[7][3] = { { 0 } };

  for (int iflag = 0; iflag < NFlags; iflag++) {
    for (int ilayer = 0; ilayer < NLayer; ilayer++) {
      for (int istave = 0; istave < NStaves[ilayer]; istave++) {
        int flagCount = 0;
        for (int ilane = 0; ilane < NLanesMax; ilane++) {
          if (mStatusFlagNumber[ilayer][istave][ilane][iflag] > 0) {
            flagCount++;
            counterSummary[0][iflag]++;
            if (ilayer < 3) {
              counterSummary[1][iflag]++;
            } else if (ilayer < 5) {
              counterSummary[2][iflag]++;
            } else {
              counterSummary[3][iflag]++;
            }
            layerSummary[ilayer][iflag]++;
          }
        }
        mLaneStatusOverview[iflag]->SetBinContent(istave + 1 + StaveBoundary[ilayer], (float)(flagCount) / (float)(NLanePerStaveLayer[ilayer]));
        mLaneStatusOverview[iflag]->SetBinError(istave + 1 + StaveBoundary[ilayer], 1e-15);
      }
      mLaneStatusSummary[ilayer]->SetBinContent(iflag + 1, layerSummary[ilayer][iflag]);
    }
    mLaneStatusSummaryGlobal->SetBinContent(iflag + 1, 1. * counterSummary[0][iflag] / NLanesTotal);
    mLaneStatusSummaryIB->SetBinContent(iflag + 1, 1. * counterSummary[1][iflag] / NLanesIB);
    mLaneStatusSummaryML->SetBinContent(iflag + 1, 1. * counterSummary[2][iflag] / NLanesML);
    mLaneStatusSummaryOL->SetBinContent(iflag + 1, 1. * counterSummary[3][iflag] / NLanesOL);
  }

  mLaneStatusSummaryGlobal->SetBinContent(4, 1. * (counterSummary[0][0] + counterSummary[0][1] + counterSummary[0][2]) / NLanesTotal);

  for (int i = 0; i < NFees; i++) {
    if (nStops[i]) {
      float payloadAvg = (float)payloadTot[i] / nStops[i];
      mPayloadSize->Fill(i, payloadAvg);
    }
  }

  mTimeFrameId++;

  mTFInfo->Fill(mTimeFrameId);
  end = std::chrono::high_resolution_clock::now();
  difference = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  mProcessingTime->SetBinContent(mTimeFrameId, difference);
}

void ITSFeeTask::getParameters()
{
  mNPayloadSizeBins = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "NPayloadSizeBins", mNPayloadSizeBins);
  mResetLaneStatus = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "ResetLaneStatus", mResetLaneStatus);
  mResetPayload = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "ResetPayload", mResetPayload);
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

void ITSFeeTask::resetLanePlotsAndCounters()
{
  if (mResetLaneStatus) {
    mRDHSummary->Reset("ICES"); // option ICES is to not remove layer lines and labels
    mFlag1Check->Reset();
    mIndexCheck->Reset();
    mIdCheck->Reset();
    mLaneStatusSummaryIB->Reset();
    mLaneStatusSummaryML->Reset();
    mLaneStatusSummaryOL->Reset();
    mLaneStatusSummaryGlobal->Reset("ICES");
    for (int i = 0; i < NFlags; i++) {
      mLaneStatus[i]->Reset("ICES");
      mLaneStatusOverview[i]->Reset("content");
    }
    for (int i = 0; i < NLayer; i++) {
      mLaneStatusSummary[i]->Reset();
    }

    memset(mStatusFlagNumber, 0, sizeof(mStatusFlagNumber)); // reset counters
  }
  if (mResetPayload) {
    mPayloadSize->Reset("ICES");
  }
}

void ITSFeeTask::reset()
{
  resetGeneralPlots();
  ILOG(Debug, Devel) << "Reset" << ENDM;
}

} // namespace o2::quality_control_modules::its
