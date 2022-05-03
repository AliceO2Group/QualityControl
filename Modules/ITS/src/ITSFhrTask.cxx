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
/// \file   ITSFhrTask.cxx
/// \author Liang Zhang
/// \author Jian Liu
/// \author Zhen Zhang
///

#include "ITS/ITSFhrTask.h"

#include <DPLUtils/RawParser.h>
#include <DPLUtils/DPLRawParser.h>
#ifdef WITH_OPENMP
#include <omp.h>
#endif

using namespace o2::framework;
using namespace o2::itsmft;
using namespace o2::header;

namespace o2::quality_control_modules::its
{

ITSFhrTask::ITSFhrTask()
  : TaskInterface()
{
}

ITSFhrTask::~ITSFhrTask()
{
  delete mGeneralOccupancy;
  delete mGeneralNoisyPixel;
  delete mDecoder;
  delete mChipDataBuffer;
  delete mTFInfo;
  delete mErrorPlots;
  delete mErrorVsFeeid;
  delete mTriggerPlots;
  delete mTriggerVsFeeid;
  delete mInfoCanvasComm;
  delete mInfoCanvasOBComm;
  delete mTextForShifter;
  delete mTextForShifter2;
  delete mTextForShifterOB;
  delete mTextForShifterOB2;
  delete mGeom;
  delete mChipStaveOccupancy[mLayer];
  delete mChipStaveEventHitCheck[mLayer];
  delete mOccupancyPlot[mLayer];
  delete mDeadChipPos[mLayer];
  delete mAliveChipPos[mLayer];
  delete mTotalDeadChipPos;
  delete mTotalAliveChipPos;
  for (int istave = 0; istave < NStaves[mLayer]; istave++) {
    delete mStaveHitmap[mLayer][istave];
  }
  for (int istave = 0; istave < NStaves[mLayer]; istave++) {
    delete[] mHitnumberLane[istave];
    delete[] mOccupancyLane[istave];
    delete[] mChipPhi[istave];
    delete[] mChipEta[istave];
    delete[] mChipStat[istave];
    int maxlink = mLayer < NLayerIB ? 2 : 3;
    for (int ilink = 0; ilink < maxlink; ilink++) {
      delete[] mErrorCount[istave][ilink];
    }
    delete[] mErrorCount[istave];
    for (int ihic = 0; ihic < nHicPerStave[mLayer]; ihic++) {
      delete[] mHitPixelID_InStave[istave][ihic];
    }
    delete[] mHitPixelID_InStave[istave];
  }
  delete[] mHitnumberLane;
  delete[] mOccupancyLane;
  delete[] mChipPhi;
  delete[] mChipEta;
  delete[] mChipStat;
  delete[] mErrorCount;
  delete[] mHitPixelID_InStave;
}

void ITSFhrTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize ITSFhrTask" << ENDM;
  getParameters();
  o2::base::GeometryManager::loadGeometry(mGeomPath.c_str());
  mGeom = o2::its::GeometryTGeo::Instance();
  int numOfChips = mGeom->getNumberOfChips();

  mGeneralOccupancy = new TH2Poly();
  mGeneralOccupancy->SetTitle("General Occupancy;mm;mm");
  mGeneralOccupancy->SetName("General/General_Occupancy");
  mGeneralOccupancy->SetStats(0);
  mGeneralOccupancy->SetMinimum(pow(10, mMinGeneralAxisRange));
  mGeneralOccupancy->SetMaximum(pow(10, mMaxGeneralAxisRange));

  mGeneralNoisyPixel = new TH2Poly();
  mGeneralNoisyPixel->SetTitle("Noisy Pixel Number;mm;mm");
  mGeneralNoisyPixel->SetName("General/Noisy_Pixel");
  mGeneralNoisyPixel->SetStats(0);
  mGeneralNoisyPixel->SetMinimum(mMinGeneralNoisyAxisRange);
  mGeneralNoisyPixel->SetMaximum(mMaxGeneralNoisyAxisRange);

  createGeneralPlots();
  createOccupancyPlots();
  setPlotsFormat();
  mDecoder = new o2::itsmft::RawPixelDecoder<o2::itsmft::ChipMappingITS>();
  mDecoder->init();
  mDecoder->setNThreads(mNThreads);
  mDecoder->setFormat(GBTLink::NewFormat);               // Using RDHv6 (NewFormat)
  mDecoder->setUserDataOrigin(header::DataOrigin("DS")); // set user data origin in dpl
  mDecoder->setUserDataDescription(header::DataDescription("RAWDATA0"));
  mChipsBuffer.resize(mGeom->getNumberOfChips());

  mGeom->fillMatrixCache(o2::math_utils::bit2Mask(o2::math_utils::TransformType::L2G));
  const math_utils::Point3D<float> loc(0., 0., 0.);

  if (mLayer != -1) {
    // define the hitnumber, occupancy, errorcount array
    mHitPixelID_InStave = new std::unordered_map<unsigned int, int>**[NStaves[mLayer]];
    mHitnumberLane = new int*[NStaves[mLayer]];
    mOccupancyLane = new double*[NStaves[mLayer]];
    mChipPhi = new double*[NStaves[mLayer]];
    mChipEta = new double*[NStaves[mLayer]];
    mChipStat = new int*[NStaves[mLayer]];
    mErrorCount = new int**[NStaves[mLayer]];

    for (int ilayer = 0; ilayer < 7; ilayer++) {
      for (int istave = 0; istave < NStaves[ilayer]; istave++) {
        double* px = new double[4];
        double* py = new double[4];
        getStavePoint(ilayer, istave, px, py);
        mGeneralOccupancy->AddBin(4, px, py);
        mGeneralNoisyPixel->AddBin(4, px, py);
      }
    }
    if (mGeneralOccupancy) {
      getObjectsManager()->startPublishing(mGeneralOccupancy);
    }
    if (mGeneralNoisyPixel) {
      getObjectsManager()->startPublishing(mGeneralNoisyPixel);
    }
    // define the errorcount array, there is some reason cause break when I define errorcount and hitnumber, occupancy at same block.
    if (mLayer < NLayerIB) {
      for (int istave = 0; istave < NStaves[mLayer]; istave++) {
        mErrorCount[istave] = new int*[3];
        for (int ilink = 0; ilink < 3; ilink++) {
          mErrorCount[istave][ilink] = new int[o2::itsmft::GBTLinkDecodingStat::NErrorsDefined];
          for (int ierror = 0; ierror < o2::itsmft::GBTLinkDecodingStat::NErrorsDefined; ierror++) {
            mErrorCount[istave][ilink][ierror] = 0;
          }
        }
      }
    } else {
      for (int istave = 0; istave < NStaves[mLayer]; istave++) {
        mErrorCount[istave] = new int*[2];
        for (int ilink = 0; ilink < 2; ilink++) {
          mErrorCount[istave][ilink] = new int[o2::itsmft::GBTLinkDecodingStat::NErrorsDefined];
          for (int ierror = 0; ierror < o2::itsmft::GBTLinkDecodingStat::NErrorsDefined; ierror++) {
            mErrorCount[istave][ilink][ierror] = 0;
          }
        }
      }
    }
    // define the hitnumber and occupancy array
    if (mLayer < NLayerIB) {
      for (int istave = 0; istave < NStaves[mLayer]; istave++) {
        mHitnumberLane[istave] = new int[nChipsPerHic[mLayer]];
        mOccupancyLane[istave] = new double[nChipsPerHic[mLayer]];
        mChipPhi[istave] = new double[nChipsPerHic[mLayer]];
        mChipEta[istave] = new double[nChipsPerHic[mLayer]];
        mChipStat[istave] = new int[nChipsPerHic[mLayer]];
        mHitPixelID_InStave[istave] = new std::unordered_map<unsigned int, int>*[nHicPerStave[mLayer]];
        for (int ihic = 0; ihic < nHicPerStave[mLayer]; ihic++) {
          mHitPixelID_InStave[istave][ihic] = new std::unordered_map<unsigned int, int>[nChipsPerHic[mLayer]];
        }
        for (int ichip = 0; ichip < nChipsPerHic[mLayer]; ichip++) {
          mHitnumberLane[istave][ichip] = 0;
          mOccupancyLane[istave][ichip] = 0;
          mChipPhi[istave][ichip] = 0;
          mChipEta[istave][ichip] = 0;
          mChipStat[istave][ichip] = 0;
          mChipStaveOccupancy[mLayer]->GetXaxis()->SetBinLabel(ichip + 1, Form("Chip %i", ichip));
          mChipStaveEventHitCheck[mLayer]->GetXaxis()->SetBinLabel(ichip + 1, Form("Chip %i", ichip));
        }
      }
    } else {
      for (int istave = 0; istave < NStaves[mLayer]; istave++) {
        mHitnumberLane[istave] = new int[nHicPerStave[mLayer] * 2];
        mOccupancyLane[istave] = new double[nHicPerStave[mLayer] * 2];
        mChipPhi[istave] = new double[nHicPerStave[mLayer] * nChipsPerHic[mLayer]];
        mChipEta[istave] = new double[nHicPerStave[mLayer] * nChipsPerHic[mLayer]];
        mChipStat[istave] = new int[nHicPerStave[mLayer] * nChipsPerHic[mLayer]];
        mHitPixelID_InStave[istave] = new std::unordered_map<unsigned int, int>*[nHicPerStave[mLayer]];
        for (int ihic = 0; ihic < nHicPerStave[mLayer]; ihic++) {
          mHitPixelID_InStave[istave][ihic] = new std::unordered_map<unsigned int, int>[nChipsPerHic[mLayer]];
        }
        for (int ichip = 0; ichip < nHicPerStave[mLayer] * nChipsPerHic[mLayer]; ichip++) {
          mChipPhi[istave][ichip] = 0;
          mChipEta[istave][ichip] = 0;
          mChipStat[istave][ichip] = 0;
        }
        mChipStaveOccupancy[mLayer]->GetYaxis()->SetBinLabel(istave + 1, Form("Stave %i", istave));
        mChipStaveEventHitCheck[mLayer]->GetYaxis()->SetBinLabel(istave + 1, Form("Stave %i", istave));
        if (mLayer < 5) {
          for (int ihic = 0; ihic < nHicPerStave[mLayer]; ihic++) {
            mHitnumberLane[istave][2 * ihic] = 0;
            mHitnumberLane[istave][2 * ihic + 1] = 0;
            mOccupancyLane[istave][2 * ihic] = 0;
            mOccupancyLane[istave][2 * ihic + 1] = 0;
            mChipStaveOccupancy[mLayer]->GetXaxis()->SetBinLabel(2 * ihic + 1, Form("%s", OBLabel34[2 * ihic]));
            mChipStaveOccupancy[mLayer]->GetXaxis()->SetBinLabel(2 * ihic + 2, Form("%s", OBLabel34[2 * ihic + 1]));
            mChipStaveEventHitCheck[mLayer]->GetXaxis()->SetBinLabel(2 * ihic + 1, Form("%s", OBLabel34[2 * ihic]));
            mChipStaveEventHitCheck[mLayer]->GetXaxis()->SetBinLabel(2 * ihic + 2, Form("%s", OBLabel34[2 * ihic + 1]));
          }

        } else {
          for (int ihic = 0; ihic < nHicPerStave[mLayer]; ihic++) {
            mHitnumberLane[istave][2 * ihic] = 0;
            mHitnumberLane[istave][2 * ihic + 1] = 0;
            mOccupancyLane[istave][2 * ihic] = 0;
            mOccupancyLane[istave][2 * ihic + 1] = 0;
            mChipStaveOccupancy[mLayer]->GetXaxis()->SetBinLabel(2 * ihic + 1, Form("%s", OBLabel56[2 * ihic]));
            mChipStaveOccupancy[mLayer]->GetXaxis()->SetBinLabel(2 * ihic + 2, Form("%s", OBLabel56[2 * ihic + 1]));
            mChipStaveEventHitCheck[mLayer]->GetXaxis()->SetBinLabel(2 * ihic + 1, Form("%s", OBLabel56[2 * ihic]));
            mChipStaveEventHitCheck[mLayer]->GetXaxis()->SetBinLabel(2 * ihic + 2, Form("%s", OBLabel56[2 * ihic + 1]));
          }
        }
      }
    }
  }
}

void ITSFhrTask::createErrorTriggerPlots()
{
  mErrorPlots = new TH1D("General/ErrorPlots", "Decoding Errors", mNError, 0.5, mNError + 0.5);
  mErrorPlots->SetMinimum(0);
  mErrorPlots->SetFillColor(kRed);
  getObjectsManager()->startPublishing(mErrorPlots); // mErrorPlots

  mTriggerPlots = new TH1D("General/TriggerPlots", "Decoding Triggers", mNTrigger, 0.5, mNTrigger + 0.5);
  mTriggerPlots->SetMinimum(0);
  mTriggerPlots->SetFillColor(kBlue);
  getObjectsManager()->startPublishing(mTriggerPlots); // mTriggerPlots
}

void ITSFhrTask::createGeneralPlots()
{
  mInfoCanvasComm = new TH2I("General/InfoCanvas", "InfoCanvas", 3, -0.5, 2.5, 4, -0.5, 3.5);
  mInfoCanvasOBComm = new TH2I("General/InfoCanvs_For_OB", "InfoCanvas for OB", 4, -0.5, 3.5, 4, -0.5, 3.5);
  for (int i = 0; i < 3; i++) {
    mInfoCanvasComm->GetXaxis()->SetBinLabel(i + 1, Form("layer%d", i));
  }
  for (int i = 0; i < 4; i++) {
    mInfoCanvasOBComm->GetXaxis()->SetBinLabel(i + 1, Form("layer%d", i + 3));
  }

  mInfoCanvasComm->SetMinimum(0);
  mInfoCanvasComm->SetMaximum(2);
  mInfoCanvasComm->GetYaxis()->SetBinLabel(2, "ibt");
  mInfoCanvasComm->GetYaxis()->SetBinLabel(4, "ibb");

  mInfoCanvasOBComm->SetMinimum(0);
  mInfoCanvasOBComm->SetMaximum(2);
  mInfoCanvasOBComm->GetYaxis()->SetBinLabel(2, "ibt");
  mInfoCanvasOBComm->GetYaxis()->SetBinLabel(4, "ibb");

  mTextForShifter = new TText(.5, 1.5, "DarkGreen -> Processing");
  mTextForShifterOB = new TText(.5, 1.5, "DarkGreen -> Processing");
  mTextForShifter2 = new TText(.5, 1.2, "Yellow    -> Finished");
  mTextForShifterOB2 = new TText(.5, 1.2, "Yellow    -> Finished");
  mTextForShifter->SetNDC();
  mTextForShifterOB->SetNDC();
  mTextForShifter2->SetNDC();
  mTextForShifterOB2->SetNDC();

  mInfoCanvasComm->SetStats(0);
  mInfoCanvasComm->GetListOfFunctions()->Add(mTextForShifter);
  mInfoCanvasComm->GetListOfFunctions()->Add(mTextForShifter2);
  getObjectsManager()->startPublishing(mInfoCanvasComm);

  mInfoCanvasOBComm->SetStats(0);
  mInfoCanvasOBComm->GetListOfFunctions()->Add(mTextForShifterOB);
  mInfoCanvasOBComm->GetListOfFunctions()->Add(mTextForShifterOB2);
  getObjectsManager()->startPublishing(mInfoCanvasOBComm);

  createErrorTriggerPlots();

  mTFInfo = new TH1F("General/TFInfo", "TF vs count", 15000, 0, 15000);
  getObjectsManager()->startPublishing(mTFInfo); // mTFInfo

  mErrorVsFeeid = new TH2I("General/ErrorVsFeeid", "Error count vs Error id and Fee id", (3 * StaveBoundary[3]) + (2 * (StaveBoundary[7] - StaveBoundary[3])), 0, (3 * StaveBoundary[3]) + (2 * (StaveBoundary[7] - StaveBoundary[3])), o2::itsmft::GBTLinkDecodingStat::NErrorsDefined, 0.5, o2::itsmft::GBTLinkDecodingStat::NErrorsDefined + 0.5);
  mTriggerVsFeeid = new TH2I("General/TriggerVsFeeid", "Trigger count vs Trigger id and Fee id", (3 * StaveBoundary[3]) + (2 * (StaveBoundary[7] - StaveBoundary[3])), 0, (3 * StaveBoundary[3]) + (2 * (StaveBoundary[7] - StaveBoundary[3])), mNTrigger, 0.5, mNTrigger + 0.5);
  mErrorVsFeeid->SetMinimum(0);
  mTriggerVsFeeid->SetMinimum(0);
  mErrorVsFeeid->SetStats(0);
  mTriggerVsFeeid->SetStats(0);
  getObjectsManager()->startPublishing(mErrorVsFeeid);
  getObjectsManager()->startPublishing(mTriggerVsFeeid);
}

void ITSFhrTask::createOccupancyPlots() // create general plots like error, trigger, TF id plots and so on....
                                        // create occupancy plots like chip stave occupancy, occupancy distribution, hic hit map plots and so on....
{
  const int nDim(2);
  int nBins[nDim] = { 1024, 512 };
  double Min[nDim] = { 0, 0 };
  double Max[nDim] = { 1024, 512 };
  mTotalDeadChipPos = new TH2D(Form("Occupancy/TotalDeadChipPos"), Form("TotalDeadChipPos "), mEtabins, -2.405, 2.405, mPhibins, -3.24, 3.24);
  mTotalDeadChipPos->SetStats(0);
  getObjectsManager()->startPublishing(mTotalDeadChipPos);

  mTotalAliveChipPos = new TH2D(Form("Occupancy/TotalAliveChipPos"), Form("TotalAliveChipPos "), mEtabins, -2.405, 2.405, mPhibins, -3.24, 3.24);
  mTotalAliveChipPos->SetStats(0);
  getObjectsManager()->startPublishing(mTotalAliveChipPos);
  // create IB plots
  if (mLayer < NLayerIB) {
    int nBinstmp[nDim] = { nBins[0] * nChipsPerHic[mLayer] / ReduceFraction, nBins[1] / ReduceFraction };
    double Maxtmp[nDim] = { Max[0] * nChipsPerHic[mLayer], Max[1] };
    for (int istave = 0; istave < NStaves[mLayer]; istave++) {
      mStaveHitmap[mLayer][istave] = new THnSparseI(Form("Occupancy/Layer%d/Stave%d/Layer%dStave%dHITMAP", mLayer, istave, mLayer, istave), Form("Hits on Layer %d, Stave %d", mLayer, istave), nDim, nBinstmp, Min, Maxtmp);
      getObjectsManager()->startPublishing(mStaveHitmap[mLayer][istave]);
    }

    mDeadChipPos[mLayer] = new TH2D(Form("Occupancy/Layer%d/Layer%dDeadChipPos", mLayer, mLayer), Form("DeadChipPos on Layer %d", mLayer), nbinsetaIB, etabinsIB[mLayer], nbinsphiIB, phibinsIB[mLayer]);    // every nine chips have same phi
    mAliveChipPos[mLayer] = new TH2D(Form("Occupancy/Layer%d/Layer%dAliveChipPos", mLayer, mLayer), Form("AliveChipPos on Layer %d", mLayer), nbinsetaIB, etabinsIB[mLayer], nbinsphiIB, phibinsIB[mLayer]); // every nine chips have same phi
    mChipStaveOccupancy[mLayer] = new TH2D(Form("Occupancy/Layer%d/Layer%dChipStave", mLayer, mLayer), Form("ITS Layer%d, Occupancy vs Chip and Stave", mLayer), nHicPerStave[mLayer] * nChipsPerHic[mLayer], -0.5, nHicPerStave[mLayer] * nChipsPerHic[mLayer] - 0.5, NStaves[mLayer], -0.5, NStaves[mLayer] - 0.5);
    mDeadChipPos[mLayer]->SetStats(0);
    mAliveChipPos[mLayer]->SetStats(0);
    mChipStaveOccupancy[mLayer]->SetStats(0);
    mChipStaveOccupancy[mLayer]->GetYaxis()->SetTickLength(0.01);
    getObjectsManager()->startPublishing(mDeadChipPos[mLayer]);
    getObjectsManager()->startPublishing(mAliveChipPos[mLayer]);
    getObjectsManager()->startPublishing(mChipStaveOccupancy[mLayer]); // mChipStaveOccupancy

    mChipStaveEventHitCheck[mLayer] = new TH2I(Form("Occupancy/Layer%d/Layer%dChipStaveEventHit", mLayer, mLayer), Form("ITS Layer%d, Event Hit Check vs Chip and Stave", mLayer), nHicPerStave[mLayer] * nChipsPerHic[mLayer], -0.5, nHicPerStave[mLayer] * nChipsPerHic[mLayer] - 0.5, NStaves[mLayer], -0.5, NStaves[mLayer] - 0.5);
    mChipStaveEventHitCheck[mLayer]->SetStats(0);
    getObjectsManager()->startPublishing(mChipStaveEventHitCheck[mLayer]);

    mOccupancyPlot[mLayer] = new TH1D(Form("Occupancy/Layer%dOccupancy", mLayer), Form("ITS Layer %d Occupancy Distribution", mLayer), 300, -15, 0);
    getObjectsManager()->startPublishing(mOccupancyPlot[mLayer]); // mOccupancyPlot
  } else {
    // Create OB plots
    int nBinstmp[nDim] = { (nBins[0] * (nChipsPerHic[mLayer] / 2) * (nHicPerStave[mLayer] / 2) / ReduceFraction), (nBins[1] * 2 * NSubStave[mLayer] / ReduceFraction) };
    double Maxtmp[nDim] = { (double)(nBins[0] * (nChipsPerHic[mLayer] / 2) * (nHicPerStave[mLayer] / 2)), (double)(nBins[1] * 2 * NSubStave[mLayer]) };
    for (int istave = 0; istave < NStaves[mLayer]; istave++) {
      mStaveHitmap[mLayer][istave] = new THnSparseI(Form("Occupancy/Layer%d/Stave%d/Layer%dStave%dHITMAP", mLayer, istave, mLayer, istave), Form("Hits on Layer %d, Stave %d", mLayer, istave), nDim, nBinstmp, Min, Maxtmp);
      getObjectsManager()->startPublishing(mStaveHitmap[mLayer][istave]);
    }
    if (mLayer == 3) {
      mDeadChipPos[mLayer] = new TH2D(Form("Occupancy/Layer%d/Layer%dDeadChipPos", mLayer, mLayer), Form("DeadChipPos on Layer %d", mLayer), 28, etabinsOB3, 96, phibinsOB3);
      mAliveChipPos[mLayer] = new TH2D(Form("Occupancy/Layer%d/Layer%dAliveChipPos", mLayer, mLayer), Form("AliveChipPos on Layer %d", mLayer), 28, etabinsOB3, 96, phibinsOB3);
    } else if (mLayer == 4) {
      mDeadChipPos[mLayer] = new TH2D(Form("Occupancy/Layer%d/Layer%dDeadChipPos", mLayer, mLayer), Form("DeadChipPos on Layer %d", mLayer), 28, etabinsOB4, 120, phibinsOB4);
      mAliveChipPos[mLayer] = new TH2D(Form("Occupancy/Layer%d/Layer%dAliveChipPos", mLayer, mLayer), Form("AliveChipPos on Layer %d", mLayer), 28, etabinsOB4, 120, phibinsOB4);
    } else if (mLayer == 5) {
      mDeadChipPos[mLayer] = new TH2D(Form("Occupancy/Layer%d/Layer%dDeadChipPos", mLayer, mLayer), Form("DeadChipPos on Layer %d", mLayer), 49, etabinsOB5, 168, phibinsOB5);
      mAliveChipPos[mLayer] = new TH2D(Form("Occupancy/Layer%d/Layer%dAliveChipPos", mLayer, mLayer), Form("AliveChipPos on Layer %d", mLayer), 49, etabinsOB5, 168, phibinsOB5);
    } else {
      mDeadChipPos[mLayer] = new TH2D(Form("Occupancy/Layer%d/Layer%dDeadChipPos", mLayer, mLayer), Form("DeadChipPos on Layer %d", mLayer), 49, etabinsOB6, 192, phibinsOB6);
      mAliveChipPos[mLayer] = new TH2D(Form("Occupancy/Layer%d/Layer%dAliveChipPos", mLayer, mLayer), Form("AliveChipPos on Layer %d", mLayer), 49, etabinsOB6, 192, phibinsOB6);
    }
    mChipStaveOccupancy[mLayer] = new TH2D(Form("Occupancy/Layer%d/Layer%dChipStave", mLayer, mLayer), Form("ITS Layer%d, Occupancy vs Chip and Stave", mLayer), nHicPerStave[mLayer] * nLanePerHic[mLayer], -0.5, nHicPerStave[mLayer] * nLanePerHic[mLayer] - 0.5, NStaves[mLayer], -0.5, NStaves[mLayer] - 0.5);
    mDeadChipPos[mLayer]->SetStats(0);
    mAliveChipPos[mLayer]->SetStats(0);
    mChipStaveOccupancy[mLayer]->SetStats(0);
    getObjectsManager()->startPublishing(mDeadChipPos[mLayer]);
    getObjectsManager()->startPublishing(mAliveChipPos[mLayer]);
    getObjectsManager()->startPublishing(mChipStaveOccupancy[mLayer]);

    mChipStaveEventHitCheck[mLayer] = new TH2I(Form("Occupancy/Layer%d/Layer%dChipStaveEventHit", mLayer, mLayer), Form("ITS Layer%d, Event Hit Check vs Chip and Stave", mLayer), nHicPerStave[mLayer] * nLanePerHic[mLayer], -0.5, nHicPerStave[mLayer] * nLanePerHic[mLayer] - 0.5, NStaves[mLayer], -0.5, NStaves[mLayer] - 0.5);
    mChipStaveEventHitCheck[mLayer]->SetStats(0);
    getObjectsManager()->startPublishing(mChipStaveEventHitCheck[mLayer]);

    mOccupancyPlot[mLayer] = new TH1D(Form("Occupancy/Layer%dOccupancy", mLayer), Form("ITS Layer %d Occupancy Distribution", mLayer), 300, -15, 0);
    getObjectsManager()->startPublishing(mOccupancyPlot[mLayer]); // mOccupancyPlot
  }
}

void ITSFhrTask::setAxisTitle(TH1* object, const char* xTitle, const char* yTitle)
{
  object->GetXaxis()->SetTitle(xTitle);
  object->GetYaxis()->SetTitle(yTitle);
}

void ITSFhrTask::setPlotsFormat()
{
  // set general plots format
  if (mErrorPlots) {
    setAxisTitle(mErrorPlots, "Error ID", "Counts");
  }
  if (mTriggerPlots) {
    setAxisTitle(mTriggerPlots, "Trigger ID", "Counts");
    for (int i = 0; i < mNTrigger; i++) {
      mTriggerPlots->GetXaxis()->SetBinLabel(i + 1, mTriggerType[i]);
    }
  }
  if (mTFInfo) {
    setAxisTitle(mTFInfo, "TF ID", "Counts");
  }
  if (mErrorVsFeeid) {
    setAxisTitle(mErrorVsFeeid, "FeeID", "Error ID");
  }
  if (mTriggerVsFeeid) {
    setAxisTitle(mTriggerVsFeeid, "FeeID", "Trigger ID");
    for (int i = 0; i < mNTrigger; i++) {
      mTriggerVsFeeid->GetYaxis()->SetBinLabel(i + 1, mTriggerType[i]);
    }
  }

  if (mTotalDeadChipPos) {
    setAxisTitle(mTotalDeadChipPos, "ChipEta", "ChipPhi");
  }
  if (mTotalAliveChipPos) {
    setAxisTitle(mTotalAliveChipPos, "ChipEta", "ChipPhi");
  }
  for (int ilayer = 0; ilayer < NLayer; ilayer++) {
    if (mOccupancyPlot[ilayer]) {
      mOccupancyPlot[ilayer]->GetXaxis()->SetTitle("log(Occupancy)");
    }
    if (ilayer < NLayerIB) {
      if (mChipStaveOccupancy[ilayer]) {
        setAxisTitle(mChipStaveOccupancy[ilayer], "Chip Number", "Stave Number");
      }
      if (mChipStaveEventHitCheck[ilayer]) {
        setAxisTitle(mChipStaveEventHitCheck[ilayer], "Chip Number", "Stave Number");
      }
      if (mDeadChipPos[ilayer]) {
        setAxisTitle(mDeadChipPos[ilayer], "ChipEta", "ChipPhi");
      }
      if (mAliveChipPos[ilayer]) {
        setAxisTitle(mAliveChipPos[ilayer], "ChipEta", "ChipPhi");
      }
    } else {
      if (mChipStaveOccupancy[ilayer]) {
        setAxisTitle(mChipStaveOccupancy[ilayer], "", "Stave Number");
      }
      if (mChipStaveEventHitCheck[ilayer]) {
        setAxisTitle(mChipStaveEventHitCheck[ilayer], "", "Stave Number");
      }
      if (mDeadChipPos[ilayer]) {
        setAxisTitle(mDeadChipPos[ilayer], "ChipEta", "ChipPhi");
      }
      if (mAliveChipPos[ilayer]) {
        setAxisTitle(mAliveChipPos[ilayer], "ChipEta", "ChipPhi");
      }
    }
  }
}

void ITSFhrTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity : " << activity.mId << ENDM;
  reset();
}

void ITSFhrTask::startOfCycle() { ILOG(Info, Support) << "startOfCycle" << ENDM; }

void ITSFhrTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // set timer
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  std::chrono::time_point<std::chrono::high_resolution_clock> end;
  int difference;
  start = std::chrono::high_resolution_clock::now();
  // get TF id by dataorigin and datadescription
  const InputSpec TFIdFilter{ "", ConcreteDataTypeMatcher{ "DS", "RAWDATA1" }, Lifetime::Timeframe }; // after Data Sampling the dataorigin will become to "DS" and the datadescription will become  to "RAWDATAX"
  if (!mGetTFFromBinding) {
    for (auto& input : ctx.inputs()) {
      if (DataRefUtils::match(input, TFIdFilter)) {
        mTimeFrameId = (unsigned int)*input.payload;
      }
    }
  } else {
    mTimeFrameId = ctx.inputs().get<int>("G");
  }

  // set Decoder
  mDecoder->startNewTF(ctx.inputs());
  mDecoder->setDecodeNextAuto(true);
  std::vector<InputSpec> rawDataFilter{ InputSpec{ "", ConcreteDataTypeMatcher{ "DS", "RAWDATA0" }, Lifetime::Timeframe } };
  DPLRawParser parser(ctx.inputs(), rawDataFilter); // set input data

  // get data information from RDH(like witch layer, stave, link, trigger type)
  int lay = 0;
  for (auto it = parser.begin(), end = parser.end(); it != end; ++it) {
    auto const* rdh = it.get_if<o2::header::RAWDataHeaderV6>(); // Decoding new data format (RDHv6)
    int istave = (int)(rdh->feeId & 0x00ff);
    int ilink = (int)((rdh->feeId & 0x0f00) >> 8);
    lay = (int)(rdh->feeId >> 12);
    if (partID == 0) {
      partID += lay * 100;
      partID += partID <= 100 ? (istave / (NStaves[lay] / 2)) * 2 : istave / (NStaves[lay] / 4);
    }
    if (lay < NLayerIB) {
      istave += StaveBoundary[lay];
    } else {
      istave += StaveBoundary[lay] - StaveBoundary[NLayerIB];
    }
    for (int i = 0; i < 13; i++) {
      if (((uint32_t)(rdh->triggerType) >> i & 1) == 1) {
        mTriggerPlots->Fill(i + 1);
        if (lay < NLayerIB) {
          mTriggerVsFeeid->Fill((istave * 3) + ilink, i + 1);
        } else {
          mTriggerVsFeeid->Fill((3 * StaveBoundary[3]) + (istave * 2) + ilink, i + 1);
        }
      }
    }
  }

  // update general information according trigger type
  if (mTriggerPlots->GetBinContent(10) || mTriggerPlots->GetBinContent(8)) {
    if (partID / 100 < 2) {
      mInfoCanvasComm->SetBinContent(partID / 100 + 1, partID % 100 + 1, 1);
      mInfoCanvasComm->SetBinContent(partID / 100 + 1, partID % 100 + 2, 1);
    } else if (partID / 100 < NLayerIB) {
      mInfoCanvasComm->SetBinContent(partID / 100 + 1, partID % 100 + 1, 1);
    } else {
      if (partID % 100 == 4) {
        partID--;
      }
      mInfoCanvasOBComm->SetBinContent(partID / 100 + 1 - NLayerIB, partID % 100 + 1, 1);
    }
  }
  if (mTriggerPlots->GetBinContent(11) || mTriggerPlots->GetBinContent(9)) {
    if (partID / 100 < 2) {
      mInfoCanvasComm->SetBinContent(partID / 100 + 1, partID % 100 + 1, 2);
      mInfoCanvasComm->SetBinContent(partID / 100 + 1, partID % 100 + 2, 2);
    } else if (partID / 100 < NLayerIB) {
      mInfoCanvasComm->SetBinContent(partID / 100 + 1, partID % 100 + 1, 2);
    } else {
      mInfoCanvasOBComm->SetBinContent(partID / 100 + 1 - NLayerIB, partID % 100 + 1, 2);
    }
  }

  // define digit hit vector
  std::vector<Digit>** digVec = new std::vector<Digit>*[NStaves[lay]];            // IB : digVec[stave][0]; OB : digVec[stave][hic]
  std::vector<ROFRecord>** digROFVec = new std::vector<ROFRecord>*[NStaves[lay]]; // IB : digROFVec[stave][0]; OB : digROFVec[stave][hic]
  const math_utils::Point3D<float> loc(0., 0., 0.);

  if (lay < NLayerIB) {
    for (int istave = 0; istave < NStaves[lay]; istave++) {
      digVec[istave] = new std::vector<Digit>[nHicPerStave[lay]];
      digROFVec[istave] = new std::vector<ROFRecord>[nHicPerStave[lay]];
    }
  } else {
    for (int istave = 0; istave < NStaves[lay]; istave++) {
      digVec[istave] = new std::vector<Digit>[nHicPerStave[lay]];
      digROFVec[istave] = new std::vector<ROFRecord>[nHicPerStave[lay]];
    }
  }

  // decode raw data and save digit hit to digit hit vector, and save hitnumber per chip/hic

  // get the position of all chips in this layer
  for (int ichip = ChipBoundary[lay]; ichip < ChipBoundary[lay + 1]; ichip++) {
    int stave = 0, chip = 0;
    auto glo = mGeom->getMatrixL2G(ichip)(loc);
    if (lay < NLayerIB) {
      stave = ichip / 9 - StaveBoundary[lay];
      chip = ichip % 9;
      mChipEta[stave][chip] = glo.eta();
      mChipPhi[stave][chip] = glo.phi();
    } else {
      stave = (ichip - ChipBoundary[lay]) / (14 * nHicPerStave[lay]);
      chip = (ichip - ChipBoundary[lay]) % (14 * nHicPerStave[lay]);
      mChipEta[stave][chip] = glo.eta();
      mChipPhi[stave][chip] = glo.phi();
    }
  }

  while ((mChipDataBuffer = mDecoder->getNextChipData(mChipsBuffer))) {
    if (mChipDataBuffer) {
      int stave = 0, chip = 0;
      int hic = 0;
      int lane = 0;
      const auto& pixels = mChipDataBuffer->getData();
      for (auto& pixel : pixels) {
        if (lay < NLayerIB) {
          stave = mChipDataBuffer->getChipID() / 9 - StaveBoundary[lay];
          chip = mChipDataBuffer->getChipID() % 9;
          hic = 0;
          mHitnumberLane[stave][chip]++;
          mChipStat[stave][chip]++;
        } else {
          stave = (mChipDataBuffer->getChipID() - ChipBoundary[lay]) / (14 * nHicPerStave[lay]);
          int chipIdLocal = (mChipDataBuffer->getChipID() - ChipBoundary[lay]) % (14 * nHicPerStave[lay]);
          chip = chipIdLocal % 14;
          hic = (chipIdLocal % (14 * nHicPerStave[lay])) / 14;

          lane = (chipIdLocal % (14 * nHicPerStave[lay])) / (14 / 2);
          mHitnumberLane[stave][lane]++;
          mChipStat[stave][chipIdLocal]++;
        }
        digVec[stave][hic].emplace_back(mChipDataBuffer->getChipID(), pixel.getRow(), pixel.getCol());
      }
      if (lay < NLayerIB) {
        if (pixels.size() > (unsigned int)mHitCutForCheck) {
          mChipStaveEventHitCheck[lay]->Fill(chip, stave);
        }
      } else {
        if (pixels.size() > (unsigned int)mHitCutForCheck) {
          mChipStaveEventHitCheck[lay]->Fill(lane, stave);
        }
      }
    }
  }

  // calculate active staves according digit hit vector
  std::vector<int> activeStaves;
  for (int i = 0; i < NStaves[lay]; i++) {
    for (int j = 0; j < nHicPerStave[lay]; j++) {
      if (digVec[i][j].size() != 0) {
        activeStaves.push_back(i);
        break;
      }
    }
  }

#ifdef WITH_OPENMP
  omp_set_num_threads(mNThreads);
#pragma omp parallel for schedule(dynamic)
#endif
  // save digit hit vector to unordered_map by openMP multiple threads
  // the reason of this step is: it will spend many time If we THnSparse::Fill the THnspase hit by hit.
  // So we want save hit information to undordered_map and fill THnSparse by THnSparse::SetBinContent (pixel by pixel)
  for (int i = 0; i < (int)activeStaves.size(); i++) {
    int istave = activeStaves[i];
    if (lay < NLayerIB) {
      for (auto& digit : digVec[istave][0]) {
        mHitPixelID_InStave[istave][0][digit.getChipIndex() % 9][1000 * digit.getColumn() + digit.getRow()]++;
      }
    } else {
      for (int ihic = 0; ihic < nHicPerStave[lay]; ihic++) {
        for (auto& digit : digVec[istave][ihic]) {
          int chip = ((digit.getChipIndex() - ChipBoundary[lay]) % (14 * nHicPerStave[lay])) % 14;
          mHitPixelID_InStave[istave][ihic][chip][1000 * digit.getColumn() + digit.getRow()]++;
        }
      }
    }
  }

  // Reset Error plots
  mErrorPlots->Reset();
  mErrorVsFeeid->Reset(); // Error is   statistic by decoder so if we didn't reset decoder, then we need reset Error plots, and use TH::SetBinContent function
  // mTriggerVsFeeid->Reset();			  Trigger is statistic by ourself so we don't need reset this plot, just use TH::Fill function
  mOccupancyPlot[lay]->Reset();

  // define tmp occupancy plot, which will use for multiple threads
  TH1D** occupancyPlotTmp = new TH1D*[(int)activeStaves.size()];
  for (int i = 0; i < (int)activeStaves.size(); i++) {
    occupancyPlotTmp[i] = new TH1D("", "", 300, -15, 0);
  }

  int totalhit = 0;
#ifdef WITH_OPENMP
  omp_set_num_threads(mNThreads);
#pragma omp parallel for schedule(dynamic) reduction(+ \
                                                     : totalhit)
#endif
  // fill Monitor Objects use openMP multiple threads, and calculate the occupancy
  for (int i = 0; i < (int)activeStaves.size(); i++) {
    int istave = activeStaves[i];
    if (digVec[istave][0].size() < 1 && lay < NLayerIB) {
      continue;
    }
    const auto* DecoderTmp = mDecoder;
    int RUid = StaveBoundary[lay] + istave;
    const o2::itsmft::RUDecodeData* RUdecode = DecoderTmp->getRUDecode(RUid);
    if (!RUdecode) {
      continue;
    }
    mNoisyPixelNumber[lay][istave] = 0;

    if (lay < NLayerIB) {
      for (int ilink = 0; ilink < RUDecodeData::MaxLinksPerRU; ilink++) {
        const auto* GBTLinkInfo = DecoderTmp->getGBTLink(RUdecode->links[ilink]);
        if (!GBTLinkInfo) {
          continue;
        }
        for (int ichip = 0 + (ilink * 3); ichip < (ilink * 3) + 3; ichip++) {
          std::unordered_map<unsigned int, int>::iterator iter;
          for (iter = mHitPixelID_InStave[istave][0][ichip].begin(); iter != mHitPixelID_InStave[istave][0][ichip].end(); iter++) {
            if ((iter->second > mHitCutForNoisyPixel) && (iter->second / (double)GBTLinkInfo->statistics.nTriggers) > mOccupancyCutForNoisyPixel) {
              mNoisyPixelNumber[lay][istave]++;
            }
            int pixelPos[2] = { (int)(iter->first / 1000) + (1024 * ichip) + 1, (int)(iter->first % 1000) + 1 };
            mStaveHitmap[lay][istave]->SetBinContent(pixelPos, (double)iter->second);
            totalhit += (int)iter->second;
            occupancyPlotTmp[i]->Fill(log10((double)iter->second / GBTLinkInfo->statistics.nTriggers));
          }
          mOccupancyLane[istave][ichip] = mHitnumberLane[istave][ichip] / (GBTLinkInfo->statistics.nTriggers * 1024. * 512.);
        }
        for (int ierror = 0; ierror < o2::itsmft::GBTLinkDecodingStat::NErrorsDefined; ierror++) {
          if (GBTLinkInfo->statistics.errorCounts[ierror] <= 0) {
            continue;
          }
          mErrorCount[istave][ilink][ierror] = GBTLinkInfo->statistics.errorCounts[ierror];
        }
      }
    } else {
      for (int ilink = 0; ilink < RUDecodeData::MaxLinksPerRU; ilink++) {
        const auto* GBTLinkInfo = DecoderTmp->getGBTLink(RUdecode->links[ilink]);
        if (!GBTLinkInfo) {
          continue;
        }
        for (int ihic = 0; ihic < ((nHicPerStave[lay] / NSubStave[lay])); ihic++) {
          for (int ichip = 0; ichip < nChipsPerHic[lay]; ichip++) {
            if (GBTLinkInfo->statistics.nTriggers > 0) {
              std::unordered_map<unsigned int, int>::iterator iter;
              for (iter = mHitPixelID_InStave[istave][ihic + ilink * ((nHicPerStave[lay] / NSubStave[lay]))][ichip].begin(); iter != mHitPixelID_InStave[istave][ihic + ilink * ((nHicPerStave[lay] / NSubStave[lay]))][ichip].end(); iter++) {
                if ((iter->second > mHitCutForNoisyPixel) && (iter->second / (double)GBTLinkInfo->statistics.nTriggers) > mOccupancyCutForNoisyPixel) {
                  mNoisyPixelNumber[lay][istave]++;
                }
                double pixelOccupancy = (double)iter->second;
                occupancyPlotTmp[i]->Fill(log10(pixelOccupancy / GBTLinkInfo->statistics.nTriggers));
                if (ichip < 7) {
                  int pixelPos[2] = { (ihic * ((nChipsPerHic[lay] / 2) * NCols)) + ichip * NCols + (int)(iter->first / 1000) + 1, NRows - ((int)iter->first % 1000) - 1 + (1024 * ilink) + 1 };
                  mStaveHitmap[lay][istave]->SetBinContent(pixelPos, pixelOccupancy);
                } else {
                  int pixelPos[2] = { (ihic * ((nChipsPerHic[lay] / 2) * NCols)) + (nChipsPerHic[lay] / 2) * NCols - (ichip - 7) * NCols - ((int)iter->first / 1000), NRows + ((int)iter->first % 1000) + (1024 * ilink) + 1 };
                  mStaveHitmap[lay][istave]->SetBinContent(pixelPos, pixelOccupancy);
                }
              }
            }
          }
          if (lay == 3 || lay == 4) {
            mOccupancyLane[istave][2 * (ihic + (ilink * 4))] = mHitnumberLane[istave][2 * (ihic + (ilink * 4))] / (GBTLinkInfo->statistics.nTriggers * 1024. * 512. * nChipsPerHic[lay] / 2);
            mOccupancyLane[istave][2 * (ihic + (ilink * 4)) + 1] = mHitnumberLane[istave][2 * (ihic + (ilink * 4)) + 1] / (GBTLinkInfo->statistics.nTriggers * 1024. * 512. * nChipsPerHic[lay] / 2);
          } else {
            mOccupancyLane[istave][2 * (ihic + (ilink * 7))] = mHitnumberLane[istave][2 * (ihic + (ilink * 7))] / (GBTLinkInfo->statistics.nTriggers * 1024. * 512. * nChipsPerHic[lay] / 2);
            mOccupancyLane[istave][2 * (ihic + (ilink * 7)) + 1] = mHitnumberLane[istave][2 * (ihic + (ilink * 7)) + 1] / (GBTLinkInfo->statistics.nTriggers * 1024. * 512. * nChipsPerHic[lay] / 2);
          }
        }
        for (int ierror = 0; ierror < o2::itsmft::GBTLinkDecodingStat::NErrorsDefined; ierror++) {
          if (GBTLinkInfo->statistics.errorCounts[ierror] <= 0) {
            continue;
          }
          mErrorCount[istave][ilink][ierror] = GBTLinkInfo->statistics.errorCounts[ierror];
        }
      }
    }
  }
  // fill Occupancy plots, chip stave occupancy plots and error statistic plots
  for (int i = 0; i < (int)activeStaves.size(); i++) {
    int istave = activeStaves[i];
    mOccupancyPlot[lay]->Add(occupancyPlotTmp[i]);
    if (lay < NLayerIB) {
      for (int ichip = 0; ichip < nChipsPerHic[lay]; ichip++) {
        mChipStaveOccupancy[lay]->SetBinContent(ichip + 1, istave + 1, mOccupancyLane[istave][ichip]);
        if (!mChipStat[istave][ichip]) {
          mDeadChipPos[lay]->SetBinContent(mDeadChipPos[lay]->GetXaxis()->FindBin(mChipEta[istave][ichip] + 0.009), mDeadChipPos[lay]->GetYaxis()->FindBin(mChipPhi[istave][ichip] + 0.001), mChipStat[istave][ichip]);
          mTotalDeadChipPos->SetBinContent(mTotalDeadChipPos->GetXaxis()->FindBin(mChipEta[istave][ichip] + 0.009), mTotalDeadChipPos->GetYaxis()->FindBin(mChipPhi[istave][ichip] + 0.001), mChipStat[istave][ichip]);
        } else {
          mAliveChipPos[lay]->SetBinContent(mAliveChipPos[lay]->GetXaxis()->FindBin(mChipEta[istave][ichip] + 0.009), mAliveChipPos[lay]->GetYaxis()->FindBin(mChipPhi[istave][ichip] + 0.001), mChipStat[istave][ichip]);
          mTotalAliveChipPos->SetBinContent(mTotalAliveChipPos->GetXaxis()->FindBin(mChipEta[istave][ichip] + 0.009), mTotalAliveChipPos->GetYaxis()->FindBin(mChipPhi[istave][ichip] + 0.001), mChipStat[istave][ichip]);
        }
        int ilink = ichip / 3;
        for (int ierror = 0; ierror < o2::itsmft::GBTLinkDecodingStat::NErrorsDefined; ierror++) {
          if (mErrorVsFeeid && (mErrorCount[istave][ilink][ierror] != 0)) {
            mErrorVsFeeid->SetBinContent(((istave + StaveBoundary[lay]) * 3) + ilink + 1, ierror + 1, mErrorCount[istave][ilink][ierror]);
          }
        }
      }
      mGeneralOccupancy->SetBinContent(istave + 1 + StaveBoundary[mLayer], *(std::max_element(mOccupancyLane[istave], mOccupancyLane[istave] + nChipsPerHic[lay])));
      mGeneralNoisyPixel->SetBinContent(istave + 1 + StaveBoundary[mLayer], mNoisyPixelNumber[lay][istave]);
    } else {
      for (int ichip = 0; ichip < nHicPerStave[lay] * nChipsPerHic[lay]; ichip++) {
        if (!mChipStat[istave][ichip]) {
          mDeadChipPos[lay]->SetBinContent(mDeadChipPos[lay]->GetXaxis()->FindBin(mChipEta[istave][ichip] + 0.009), mDeadChipPos[lay]->GetYaxis()->FindBin(mChipPhi[istave][ichip] + 0.001), mChipStat[istave][ichip]);
          mTotalDeadChipPos->SetBinContent(mTotalDeadChipPos->GetXaxis()->FindBin(mChipEta[istave][ichip] + 0.009), mTotalDeadChipPos->GetYaxis()->FindBin(mChipPhi[istave][ichip] + 0.001), mChipStat[istave][ichip]);
        } else {
          mAliveChipPos[lay]->SetBinContent(mAliveChipPos[lay]->GetXaxis()->FindBin(mChipEta[istave][ichip] + 0.009), mAliveChipPos[lay]->GetYaxis()->FindBin(mChipPhi[istave][ichip] + 0.001), mChipStat[istave][ichip]);
          mTotalAliveChipPos->SetBinContent(mTotalAliveChipPos->GetXaxis()->FindBin(mChipEta[istave][ichip] + 0.009), mTotalAliveChipPos->GetYaxis()->FindBin(mChipPhi[istave][ichip] + 0.001), mChipStat[istave][ichip]);
        }
      }

      for (int ihic = 0; ihic < nHicPerStave[lay]; ihic++) {
        int ilink = ihic / (nHicPerStave[lay] / 2);
        mChipStaveOccupancy[lay]->SetBinContent(2 * ihic + 1, istave + 1, mOccupancyLane[istave][2 * ihic]);
        mChipStaveOccupancy[lay]->SetBinContent(2 * ihic + 2, istave + 1, mOccupancyLane[istave][2 * ihic + 1]);
        if (ihic == 0 || ihic == 7) {
          for (int ierror = 0; ierror < o2::itsmft::GBTLinkDecodingStat::NErrorsDefined; ierror++) {
            if (mErrorVsFeeid && (mErrorCount[istave][ilink][ierror] != 0)) {
              mErrorVsFeeid->SetBinContent((3 * StaveBoundary[3]) + ((StaveBoundary[lay] - StaveBoundary[NLayerIB] + istave) * 2) + ilink + 1, ierror + 1, mErrorCount[istave][ilink][ierror]);
            }
          }
        }
      }
      mGeneralOccupancy->SetBinContent(istave + 1 + StaveBoundary[mLayer], *(std::max_element(mOccupancyLane[istave], mOccupancyLane[istave] + nHicPerStave[lay] * 2)));
      mGeneralNoisyPixel->SetBinContent(istave + 1 + StaveBoundary[mLayer], mNoisyPixelNumber[lay][istave]);
    }
  }
  for (int ierror = 0; ierror < o2::itsmft::GBTLinkDecodingStat::NErrorsDefined; ierror++) {
    int feeError = mErrorVsFeeid->Integral(1, mErrorVsFeeid->GetXaxis()->GetNbins(), ierror + 1, ierror + 1);
    mErrorPlots->SetBinContent(ierror + 1, feeError);
  }

  // delete pointor in monitorData()
  for (int istave = 0; istave < NStaves[mLayer]; istave++) {
    delete[] digVec[istave];
    delete[] digROFVec[istave];
  }
  delete[] digVec;
  delete[] digROFVec;
  for (int i = 0; i < (int)activeStaves.size(); i++) {
    delete occupancyPlotTmp[i];
  }
  delete[] occupancyPlotTmp;
  // temporarily reverting to get TFId by querying binding
  //   mTimeFrameId = ctx.inputs().get<int>("G");
  // Timer LOG
  mTFInfo->Fill(mTimeFrameId);
  end = std::chrono::high_resolution_clock::now();
  difference = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  mAverageProcessTime += difference;
  mTFCount++;
}

void ITSFhrTask::getParameters()
{
  mGeomPath = mCustomParameters["geomPath"];
  mNThreads = std::stoi(mCustomParameters["decoderThreads"]);
  mLayer = std::stoi(mCustomParameters["Layer"]);
  mHitCutForCheck = std::stoi(mCustomParameters["HitNumberCut"]);
  mGetTFFromBinding = std::stoi(mCustomParameters["GetTFFromBinding"]);
  mHitCutForNoisyPixel = std::stoi(mCustomParameters["HitNumberCutForNoisyPixel"]);
  mOccupancyCutForNoisyPixel = std::stof(mCustomParameters["OccupancyNumberCutForNoisyPixel"]);
  mMaxGeneralAxisRange = std::stof(mCustomParameters["MaxGeneralAxisRange"]);
  mMinGeneralAxisRange = std::stof(mCustomParameters["MinGeneralAxisRange"]);
  mMaxGeneralNoisyAxisRange = std::stof(mCustomParameters["MaxGeneralNoisyAxisRange"]);
  mMinGeneralNoisyAxisRange = std::stof(mCustomParameters["MinGeneralNoisyAxisRange"]);
  mPhibins = std::stoi(mCustomParameters["Phibins"]);
  mEtabins = std::stoi(mCustomParameters["Etabins"]);
}

void ITSFhrTask::endOfCycle()
{
  ILOG(Debug, Support) << "average process time == " << (double)mAverageProcessTime / mTFCount << ENDM;
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void ITSFhrTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void ITSFhrTask::resetGeneralPlots()
{
  resetObject(mTFInfo);
  resetObject(mErrorPlots);
  resetObject(mErrorVsFeeid);
  resetObject(mTriggerVsFeeid);
  resetObject(mTriggerPlots);
  //	resetObject(mInfoCanvasComm);
  //	resetObject(mInfoCanvasOBComm);
}

void ITSFhrTask::resetOccupancyPlots()
{
  memset(mHitNumberOfChip, 0, sizeof(mHitNumberOfChip));
  memset(mTriggerTypeCount, 0, sizeof(mTriggerTypeCount));
  memset(mErrors, 0, sizeof(mErrors));
  mTimeFrameId = 0;
  mChipStaveOccupancy[mLayer]->Reset();
  mChipStaveEventHitCheck[mLayer]->Reset();
  mOccupancyPlot[mLayer]->Reset();
  mDeadChipPos[mLayer]->Reset();
  mAliveChipPos[mLayer]->Reset();
  mTotalDeadChipPos->Reset();
  mTotalAliveChipPos->Reset();
  for (int istave = 0; istave < NStaves[mLayer]; istave++) {
    mStaveHitmap[mLayer][istave]->Reset();
  }
}

void ITSFhrTask::resetObject(TH1* obj)
{
  if (obj) {
    obj->Reset();
  }
}

void ITSFhrTask::reset()
{
  resetGeneralPlots();
  resetOccupancyPlots();

  mGeneralOccupancy->Reset("content");
  mGeneralNoisyPixel->Reset("content");
  mDecoder->clearStat();

  if (mLayer < NLayerIB) {
    for (int istave = 0; istave < NStaves[mLayer]; istave++) {
      for (int ilink = 0; ilink < 3; ilink++) {
        for (int ierror = 0; ierror < o2::itsmft::GBTLinkDecodingStat::NErrorsDefined; ierror++) {
          mErrorCount[istave][ilink][ierror] = 0;
        }
      }
      for (int ichip = 0; ichip < nChipsPerHic[mLayer]; ichip++) {
        mHitnumberLane[istave][ichip] = 0;
        mOccupancyLane[istave][ichip] = 0;
        mHitPixelID_InStave[istave][0][ichip].clear();
      }
    }
  } else {
    for (int istave = 0; istave < NStaves[mLayer]; istave++) {
      for (int ilink = 0; ilink < 2; ilink++) {
        for (int ierror = 0; ierror < o2::itsmft::GBTLinkDecodingStat::NErrorsDefined; ierror++) {
          mErrorCount[istave][ilink][ierror] = 0;
        }
      }
      for (int ihic = 0; ihic < nHicPerStave[mLayer]; ihic++) {
        mHitnumberLane[istave][2 * ihic] = 0;
        mHitnumberLane[istave][2 * ihic + 1] = 0;
        mOccupancyLane[istave][2 * ihic] = 0;
        mOccupancyLane[istave][2 * ihic + 1] = 0;
        for (int ichip = 0; ichip < nChipsPerHic[mLayer]; ichip++) {
          mHitPixelID_InStave[istave][ihic][ichip].clear();
        }
      }
    }
  }

  ILOG(Info, Support) << "Reset" << ENDM;
}

void ITSFhrTask::getStavePoint(int layer, int stave, double* px, double* py)
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
} // namespace o2::quality_control_modules::its
