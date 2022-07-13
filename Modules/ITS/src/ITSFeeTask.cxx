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
/// \author Zhen Zhang
///

#include "ITS/ITSFeeTask.h"
#include "QualityControl/QcInfoLogger.h"
#include "ITSMFTReconstruction/DecodingStat.h"

#include <DPLUtils/RawParser.h>
#include <DPLUtils/DPLRawParser.h>
#include <iostream>

using namespace o2::framework;
using namespace o2::itsmft;
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
  delete mDecoder;
  delete mTriggerVsFeeId;
  delete mLaneInfo;
  delete mErrorPlots;
  delete mFlag1Check;
  delete mIndexCheck;
  delete mIdCheck;
  delete mProcessingTime;
  delete mPayloadSize;
  delete mLaneStatusSummaryIB;
  delete mLaneStatusSummaryML;
  delete mLaneStatusSummaryOL;
  delete mLinkErrorVsFeeid;
  delete mChipErrorVsFeeid;
  delete mLaneStatusSummaryGlobal;
  for (int i = 0; i < NFlags; i++) {
    delete mLaneStatus[i];
  }
  for (int i = 0; i < NFlags; i++) {
    delete mLaneStatusOverview[i];
  }
  for (int i = 0; i < NLayer; i++) {
    delete mLaneStatusSummary[i];
  }
  for (int ilayer = 0; ilayer < NLayer; ilayer++) {
    int maxlink = ilayer < NLayerIB ? 3 : 2;
    for (int istave = 0; istave < NStaves[ilayer]; istave++) {
      for(int ilink = 0; ilink < maxlink; ilink++) {
      	delete[] mLinkErrorCount[ilayer][istave][ilink];
      	delete[] mChipErrorCount[ilayer][istave][ilink];
      }
	
    delete[] mLinkErrorCount[ilayer][istave];
    delete[] mChipErrorCount[ilayer][istave];
    }
    delete[] mLinkErrorCount[ilayer];
    delete[] mChipErrorCount[ilayer];
  }
  delete[] mLinkErrorCount;
  delete[] mChipErrorCount;

  // delete mInfoCanvas;
}

void ITSFeeTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "Initializing the ITSFeeTask" << ENDM;
  getParameters();
  createFeePlots();
  setPlotsFormat();

  mDecoder = new o2::itsmft::RawPixelDecoder<o2::itsmft::ChipMappingITS>();
  mDecoder->init();
  mDecoder->setNThreads(mNThreads);
  mDecoder->setFormat(GBTLink::NewFormat);               // Using RDHv6 (NewFormat)
  mDecoder->setUserDataOrigin(header::DataOrigin("DS")); // set user data origin in dpl
  mDecoder->setUserDataDescription(header::DataDescription("RAWDATA0"));

  
  mLinkErrorCount = new int***[NLayer];
  mChipErrorCount = new int***[NLayer];
  for (int ilayer = 0; ilayer < NLayer; ilayer++) {
    mLinkErrorCount[ilayer] = new int**[NStaves[ilayer]];
    mChipErrorCount[ilayer] = new int**[NStaves[ilayer]];
    int maxlink = ilayer < NLayerIB ? 3 : 2;
    for (int istave = 0; istave < NStaves[ilayer]; istave++) {
      mLinkErrorCount[ilayer][istave] = new int*[maxlink];
      mChipErrorCount[ilayer][istave] = new int*[maxlink];
	// for link statistic
      for (int ilink = 0; ilink < maxlink; ilink++) {
	mLinkErrorCount[ilayer][istave][ilink] = new int[o2::itsmft::GBTLinkDecodingStat::NErrorsDefined];
	mChipErrorCount[ilayer][istave][ilink] = new int[o2::itsmft::ChipStat::NErrorsDefined];
	for (int ierror = 0; ierror < o2::itsmft::GBTLinkDecodingStat::NErrorsDefined; ierror++) {
	  mLinkErrorCount[ilayer][istave][ilink][ierror] = 0;
	}
	for (int ierror = 0; ierror < o2::itsmft::ChipStat::NErrorsDefined; ierror++) {
	  mChipErrorCount[ilayer][istave][ilink][ierror] = 0;
	}
      }
    }
  }
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

  mLinkErrorVsFeeid = new TH2I("General/LinkErrorVsFeeid", "Error count vs Error id and Fee id", (3 * StaveBoundary[3]) + (2 * (StaveBoundary[7] - StaveBoundary[3])), 0, (3 * StaveBoundary[3]) + (2 * (StaveBoundary[7] - StaveBoundary[3])), o2::itsmft::GBTLinkDecodingStat::NErrorsDefined, 0.5, o2::itsmft::GBTLinkDecodingStat::NErrorsDefined + 0.5);
  mLinkErrorVsFeeid->SetMinimum(0);
  mLinkErrorVsFeeid->SetStats(0);
  getObjectsManager()->startPublishing(mLinkErrorVsFeeid);
  
  mChipErrorVsFeeid = new TH2I("General/ChipErrorVsFeeid", "Error count vs Error id and Fee id", (3 * StaveBoundary[3]) + (2 * (StaveBoundary[7] - StaveBoundary[3])), 0, (3 * StaveBoundary[3]) + (2 * (StaveBoundary[7] - StaveBoundary[3])), o2::itsmft::ChipStat::NErrorsDefined, 0.5, o2::itsmft::ChipStat::NErrorsDefined + 0.5);
  mChipErrorVsFeeid->SetMinimum(0);
  mChipErrorVsFeeid->SetStats(0);
  getObjectsManager()->startPublishing(mChipErrorVsFeeid);

  mErrorPlots = new TH1D("General/ErrorPlots", "Decoding Errors", o2::itsmft::GBTLinkDecodingStat::NErrorsDefined, 0.5, o2::itsmft::GBTLinkDecodingStat::NErrorsDefined + 0.5);
  mErrorPlots->SetMinimum(0);
  mErrorPlots->SetFillColor(kRed);
  getObjectsManager()->startPublishing(mErrorPlots); // mErrorPlots

  for (int i = 0; i < NFlags; i++) {
    mLaneStatus[i] = new TH2I(Form("LaneStatus/laneStatusFlag%s", mLaneStatusFlag[i].c_str()), Form("Lane Status Flag : %s", mLaneStatusFlag[i].c_str()), NFees, 0, NFees, NLanesMax, 0, NLanesMax);
    getObjectsManager()->startPublishing(mLaneStatus[i]); // mlaneStatus
  }

  for (int i = 0; i < NFlags; i++) {
    mLaneStatusOverview[i] = new TH2Poly();
    mLaneStatusOverview[i]->SetName(Form("LaneStatus/laneStatusOverviewFlag%s", mLaneStatusFlag[i].c_str()));
  }

  for (int i = 0; i < NLayer; i++) {
    mLaneStatusSummary[i] = new TH1I(Form("LaneStatusSummary/LaneStatusSummaryL%i", i), Form("Lane Status Summary L%i", i), 3, 0, 3);
    getObjectsManager()->startPublishing(mLaneStatusSummary[i]); // mLaneStatusSummary
  }

  mLaneStatusSummaryIB = new TH1I("LaneStatusSummary/LaneStatusSummaryIB", "Lane Status Summary IB", 3, 0, 3);
  getObjectsManager()->startPublishing(mLaneStatusSummaryIB); // mLaneStatusSummaryIB
  mLaneStatusSummaryML = new TH1I("LaneStatusSummary/LaneStatusSummaryML", "Lane Status Summary ML", 3, 0, 3);
  getObjectsManager()->startPublishing(mLaneStatusSummaryML); // mLaneStatusSummaryML
  mLaneStatusSummaryOL = new TH1I("LaneStatusSummary/LaneStatusSummaryOL", "Lane Status Summary OL", 3, 0, 3);
  getObjectsManager()->startPublishing(mLaneStatusSummaryOL); // mLaneStatusSummaryOL
  mLaneStatusSummaryGlobal = new TH1I("LaneStatusSummary/LaneStatusSummaryGlobal", "Lane Status Summary Global", 3, 0, 3);
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

void ITSFeeTask::drawLayerName(TH2I* histo2D)
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
  if (mLinkErrorVsFeeid) {
    setAxisTitle(mLinkErrorVsFeeid, "FeeID", "Error ID");
  }
  if (mChipErrorVsFeeid) {
    setAxisTitle(mChipErrorVsFeeid, "FeeID", "Error ID");
  }

  if (mErrorPlots) {
    setAxisTitle(mErrorPlots, "Error ID", "Counts");
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
    }
  }

  for (int i = 0; i < NFlags; i++) {
    TString title = Form("Fraction of lanes into %s", mLaneStatusFlag[i].c_str());
    title += ";mm;mm";
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
    setAxisTitle(mLaneStatusSummaryIB, "", "#Lanes");
    for (int i = 0; i < NFlags; i++) {
      mLaneStatusSummaryIB->GetXaxis()->SetBinLabel(i + 1, mLaneStatusFlag[i].c_str());
    }
    mLaneStatusSummaryIB->GetXaxis()->CenterLabels();
    mLaneStatusSummaryIB->SetStats(0);
  }

  if (mLaneStatusSummaryML) {
    setAxisTitle(mLaneStatusSummaryML, "", "#Lanes");
    for (int i = 0; i < NFlags; i++) {
      mLaneStatusSummaryML->GetXaxis()->SetBinLabel(i + 1, mLaneStatusFlag[i].c_str());
    }
    mLaneStatusSummaryML->GetXaxis()->CenterLabels();
    mLaneStatusSummaryML->SetStats(0);
  }

  if (mLaneStatusSummaryOL) {
    setAxisTitle(mLaneStatusSummaryOL, "", "#Lanes");
    for (int i = 0; i < NFlags; i++) {
      mLaneStatusSummaryOL->GetXaxis()->SetBinLabel(i + 1, mLaneStatusFlag[i].c_str());
    }
    mLaneStatusSummaryOL->GetXaxis()->CenterLabels();
    mLaneStatusSummaryOL->SetStats(0);
  }

  if (mLaneStatusSummaryGlobal) {
    setAxisTitle(mLaneStatusSummaryGlobal, "", "#Lanes");
    for (int i = 0; i < NFlags; i++) {
      mLaneStatusSummaryGlobal->GetXaxis()->SetBinLabel(i + 1, mLaneStatusFlag[i].c_str());
    }
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

void ITSFeeTask::startOfCycle() { ILOG(Info, Support) << "startOfCycle" << ENDM; }

void ITSFeeTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // set timer
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  std::chrono::time_point<std::chrono::high_resolution_clock> end;
  int difference;
  start = std::chrono::high_resolution_clock::now();

  int nStops[NFees] = {};
  int payloadTot[NFees] = {};

  std::vector<InputSpec> rawDataFilter{ InputSpec{ "", ConcreteDataTypeMatcher{ "DS", "RAWDATA0" }, Lifetime::Timeframe } };

  rawDataFilter.push_back(InputSpec{ "", ConcreteDataTypeMatcher{ "ITS", "RAWDATA" }, Lifetime::Timeframe });
  DPLRawParser parser(ctx.inputs(), rawDataFilter);

  mDecoder->startNewTF(ctx.inputs());
  mDecoder->setDecodeNextAuto(true);

  for (auto it = parser.begin(), end = parser.end(); it != end; ++it) {
   
    auto const* rdh = it.get_if<o2::header::RAWDataHeaderV6>();
    // Decoding data format (RDHv6)
    int istave = (int)(rdh->feeId & 0x00ff);
    int ilink = (int)((rdh->feeId & 0x0f00) >> 8);
    int ilayer = (int)((rdh->feeId & 0xf000) >> 12);
    int ifee = 3 * StaveBoundary[ilayer] - (StaveBoundary[ilayer] - StaveBoundary[NLayerIB]) * (ilayer >= NLayerIB) + istave * (3 - (ilayer >= NLayerIB)) + ilink;
    int memorysize = (int)(rdh->memorySize);
    int headersize = (int)(rdh->headerSize);

    payloadTot[ifee] += memorysize - headersize;
    bool clockEvt = false;
    auto const* DecoderTmp = mDecoder;
    int RUid = StaveBoundary[ilayer] + istave;
    const o2::itsmft::RUDecodeData* RUdecode = DecoderTmp->getRUDecode(RUid);
    if (!RUdecode) {
      continue;
    }
    
    auto const* GBTLinkInfo = DecoderTmp->getGBTLink(RUdecode->links[ilink]);
    if (!GBTLinkInfo) {
      continue;
    }

    for (int ierror = 0; ierror < o2::itsmft::GBTLinkDecodingStat::NErrorsDefined; ierror++) {
      if (GBTLinkInfo->statistics.errorCounts[ierror] <= 0) {
        continue;
      }
      mLinkErrorCount[ilayer][istave][ilink][ierror] = GBTLinkInfo->statistics.errorCounts[ierror];
      if (mLinkErrorVsFeeid && (mLinkErrorCount[ilayer][istave][ilink][ierror] != 0)) {
        mLinkErrorVsFeeid->SetBinContent(ifee + 1, ierror + 1, mLinkErrorCount[ilayer][istave][ilink][ierror]);
      }
    }
    for (int ierror = 0; ierror < o2::itsmft::ChipStat::NErrorsDefined; ierror++) {
      if (GBTLinkInfo->chipStat.errorCounts[ierror] <= 0) {
        continue;
      }
      mChipErrorCount[ilayer][istave][ilink][ierror] = GBTLinkInfo->chipStat.errorCounts[ierror];
      if (mChipErrorVsFeeid && (mChipErrorCount[ilayer][istave][ilink][ierror] != 0)) {
        mChipErrorVsFeeid->SetBinContent(ifee + 1, ierror + 1, mChipErrorCount[ilayer][istave][ilink][ierror]);
      }
    }




    // RDHSummaryPlot
    //  get detector field
    uint64_t summaryLaneStatus = rdh->detectorField;
    // fill statusVsFeeId if set
    if (summaryLaneStatus & (1 << 0))
      mRDHSummary->Fill(ifee, 0); // missing data
    if (summaryLaneStatus & (1 << 1))
      mRDHSummary->Fill(ifee, 1); // warning
    if (summaryLaneStatus & (1 << 2))
      mRDHSummary->Fill(ifee, 2); // error
    if (summaryLaneStatus & (1 << 3))
      mRDHSummary->Fill(ifee, 3); // fault
    if (summaryLaneStatus & (1 << 27)) {
      mRDHSummary->Fill(ifee, 4); // clock evt
      clockEvt = true;
    }
    if (summaryLaneStatus & (1 << 26))
      mRDHSummary->Fill(ifee, 5); // Timebase evt
    if (summaryLaneStatus & (1 << 25))
      mRDHSummary->Fill(ifee, 6); // Timebase Unsync evt

    if ((int)(rdh->stop) && it.size()) { // looking into the DDW0 from the closing packet
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

      // std::cout << "Layer:"<< ilayer << "Stave:" << istave << std::endl;
      for (int i = 0; i < NLanesMax; i++) {
        int laneValue = laneInfo >> (2 * i) & 0x3;
        if (laneValue) {
          mStatusFlagNumber[ilayer][istave][i][laneValue - 1]++;
          mLaneStatus[laneValue - 1]->Fill(ifee, i);
        }
      }

      for (int iflag = 0; iflag < 3; iflag++) {
        if (clockEvt) {
          int feeInStave = (ifee - feeBoundary[ilayer]) - (feePerStave[ilayer] * (istave));
          int startingLane = (feeInStave - 1) * lanesPerFeeId[ilayer];
          for (int indexLaneFee = indexFeeLow[ilayer]; indexLaneFee < indexFeeUp[ilayer]; indexLaneFee++) {
            mLaneStatus[iflag]->Fill(ifee, indexLaneFee);
          }
          for (int indexLane = startingLane; indexLane < (startingLane + lanesPerFeeId[ilayer]); indexLane++) {
            mStatusFlagNumber[ilayer][istave][indexLane][iflag]++;
          }
        }
      }
    }

    if ((int)(rdh->stop)) {
      nStops[ifee]++;
      for (int i = 0; i < 13; i++) {
        if (((uint32_t)(rdh->triggerType) >> i & 1) == 1) {
          mTrigger->Fill(i + 1);
          mTriggerVsFeeId->Fill(ifee, i + 1);
        }
      }
    }
  }
  
  for (int ierror = 0; ierror < o2::itsmft::GBTLinkDecodingStat::NErrorsDefined; ierror++) {
    int feeError = mLinkErrorVsFeeid->Integral(1, mLinkErrorVsFeeid->GetXaxis()->GetNbins(), ierror + 1, ierror + 1);
    mErrorPlots->SetBinContent(ierror + 1, feeError);
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
          }
        }
        mLaneStatusOverview[iflag]->SetBinContent(istave + 1 + StaveBoundary[ilayer], (float)(flagCount) / (float)(NLanePerStaveLayer[ilayer]));
        mLaneStatusOverview[iflag]->SetBinError(istave + 1 + StaveBoundary[ilayer], 1e-15);
      }
      mLaneStatusSummary[ilayer]->SetBinContent(iflag + 1, layerSummary[ilayer][iflag]);
    }
    mLaneStatusSummaryGlobal->SetBinContent(iflag + 1, counterSummary[0][iflag]);
    mLaneStatusSummaryIB->SetBinContent(iflag + 1, counterSummary[1][iflag]);
    mLaneStatusSummaryML->SetBinContent(iflag + 1, counterSummary[2][iflag]);
    mLaneStatusSummaryOL->SetBinContent(iflag + 1, counterSummary[3][iflag]);
  }

  for (int i = 0; i < NFees; i++) {
    if (nStops[i]) {
      float payloadAvg = (float)payloadTot[i] / nStops[i];
      mPayloadSize->Fill(i + 1, payloadAvg);
    }
  }

  mTimeFrameId = ctx.inputs().get<int>("G");

  mTFInfo->Fill(mTimeFrameId);
  end = std::chrono::high_resolution_clock::now();
  difference = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  mProcessingTime->SetBinContent(mTimeFrameId, difference);
}

void ITSFeeTask::getParameters()
{
  mNPayloadSizeBins = std::stoi(mCustomParameters["NPayloadSizeBins"]);
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
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void ITSFeeTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void ITSFeeTask::resetGeneralPlots()
{
  mTFInfo->Reset();
  mTriggerVsFeeId->Reset();
  mTrigger->Reset();
  mLinkErrorVsFeeid->Reset(); 
  mChipErrorVsFeeid->Reset(); 
  mErrorPlots->Reset();

}

void ITSFeeTask::reset()
{
  resetGeneralPlots();
  mDecoder->clearStat();
  ILOG(Info, Support) << "Reset" << ENDM;
}

} // namespace o2::quality_control_modules::its
