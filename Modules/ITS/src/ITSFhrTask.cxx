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
#include "CCDB/BasicCCDBManager.h"
#include "CCDB/CCDBTimeStampUtils.h"
#include "Framework/TimingInfo.h"

#include "Common/Utils.h"

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
  delete mErrorPlots;
  delete mErrorVsFeeid;
  delete mChipStaveOccupancy;
  delete mChipStaveEventHitCheck;
  delete mOccupancyPlot;
  delete mDeadChipPos;
  delete mAliveChipPos;
  delete mTotalDeadChipPos;
  delete mTotalAliveChipPos;
  for (int istave = 0; istave < 48; istave++) {
    delete mStaveHitmap[istave];
  }
  for (int istave = 0; istave < NStaves[mLayer]; istave++) {
    delete[] mHitnumberLane[istave];
    delete[] mOccupancyLane[istave];
    delete[] mChipPhi[istave];
    delete[] mChipZ[istave];
    delete[] mChipStat[istave];
    int maxlink = mLayer < NLayerIB ? 3 : 2;
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
  delete[] mChipZ;
  delete[] mChipStat;
  delete[] mErrorCount;
  delete[] mHitPixelID_InStave;
}

void ITSFhrTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize ITSFhrTask" << ENDM;
  getParameters();

  mGeneralOccupancy = new TH2Poly();
  mGeneralOccupancy->SetTitle("General Occupancy;mm (IB 3x);mm (IB 3x)");
  mGeneralOccupancy->SetName("General/General_Occupancy");
  mGeneralOccupancy->SetStats(0);
  mGeneralOccupancy->SetMinimum(pow(10, mMinGeneralAxisRange));
  mGeneralOccupancy->SetMaximum(pow(10, mMaxGeneralAxisRange));

  mGeneralNoisyPixel = new TH2Poly();
  mGeneralNoisyPixel->SetTitle("Noisy Pixel Number;mm (IB 3x);mm (IB 3x)");
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
  mDecoder->setUserDataOrigin(header::DataOrigin("DS")); // set user data origin in dpl
  mDecoder->setUserDataDescription(header::DataDescription("RAWDATA0"));
  mChipsBuffer.resize(24120);

  if (mLayer != -1) {
    // define the hitnumber, occupancy, errorcount array
    mHitPixelID_InStave = new std::unordered_map<unsigned int, int>**[NStaves[mLayer]];
    mHitnumberLane = new int*[NStaves[mLayer]];
    mOccupancyLane = new double*[NStaves[mLayer]];
    mChipPhi = new double*[NStaves[mLayer]];
    mChipZ = new double*[NStaves[mLayer]];

    mChipStat = new int*[NStaves[mLayer]];
    mErrorCount = new int**[NStaves[mLayer]];

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
        mChipZ[istave] = new double[nChipsPerHic[mLayer]];

        mChipStat[istave] = new int[nChipsPerHic[mLayer]];
        mHitPixelID_InStave[istave] = new std::unordered_map<unsigned int, int>*[nHicPerStave[mLayer]];
        for (int ihic = 0; ihic < nHicPerStave[mLayer]; ihic++) {
          mHitPixelID_InStave[istave][ihic] = new std::unordered_map<unsigned int, int>[nChipsPerHic[mLayer]];
        }
        for (int ichip = 0; ichip < nChipsPerHic[mLayer]; ichip++) {
          mHitnumberLane[istave][ichip] = 0;
          mOccupancyLane[istave][ichip] = 0;
          mChipPhi[istave][ichip] = 0;
          mChipZ[istave][ichip] = 0;

          mChipStat[istave][ichip] = 0;
          mChipStaveOccupancy->GetXaxis()->SetBinLabel(ichip + 1, Form("Chip %i", ichip));
          mChipStaveEventHitCheck->GetXaxis()->SetBinLabel(ichip + 1, Form("Chip %i", ichip));
        }
      }
    } else {
      for (int istave = 0; istave < NStaves[mLayer]; istave++) {
        mHitnumberLane[istave] = new int[nHicPerStave[mLayer] * 2];
        mOccupancyLane[istave] = new double[nHicPerStave[mLayer] * 2];
        mChipPhi[istave] = new double[nHicPerStave[mLayer] * nChipsPerHic[mLayer]];
        mChipZ[istave] = new double[nHicPerStave[mLayer] * nChipsPerHic[mLayer]];

        mChipStat[istave] = new int[nHicPerStave[mLayer] * nChipsPerHic[mLayer]];
        mHitPixelID_InStave[istave] = new std::unordered_map<unsigned int, int>*[nHicPerStave[mLayer]];
        for (int ihic = 0; ihic < nHicPerStave[mLayer]; ihic++) {
          mHitPixelID_InStave[istave][ihic] = new std::unordered_map<unsigned int, int>[nChipsPerHic[mLayer]];
        }
        for (int ichip = 0; ichip < nHicPerStave[mLayer] * nChipsPerHic[mLayer]; ichip++) {
          mChipPhi[istave][ichip] = 0;
          mChipZ[istave][ichip] = 0;

          mChipStat[istave][ichip] = 0;
        }
        mChipStaveOccupancy->GetYaxis()->SetBinLabel(istave + 1, Form("Stave %i", istave));
        mChipStaveEventHitCheck->GetYaxis()->SetBinLabel(istave + 1, Form("Stave %i", istave));
        if (mLayer < 5) {
          for (int ihic = 0; ihic < nHicPerStave[mLayer]; ihic++) {
            mHitnumberLane[istave][2 * ihic] = 0;
            mHitnumberLane[istave][2 * ihic + 1] = 0;
            mOccupancyLane[istave][2 * ihic] = 0;
            mOccupancyLane[istave][2 * ihic + 1] = 0;
            mChipStaveOccupancy->GetXaxis()->SetBinLabel(2 * ihic + 1, Form("%s", OBLabel34[2 * ihic]));
            mChipStaveOccupancy->GetXaxis()->SetBinLabel(2 * ihic + 2, Form("%s", OBLabel34[2 * ihic + 1]));
            mChipStaveOccupancy->GetXaxis()->SetLabelSize(0.02);
            mChipStaveEventHitCheck->GetXaxis()->SetBinLabel(2 * ihic + 1, Form("%s", OBLabel34[2 * ihic]));
            mChipStaveEventHitCheck->GetXaxis()->SetBinLabel(2 * ihic + 2, Form("%s", OBLabel34[2 * ihic + 1]));
            mChipStaveEventHitCheck->GetXaxis()->SetLabelSize(0.02);
          }

        } else {
          for (int ihic = 0; ihic < nHicPerStave[mLayer]; ihic++) {
            mHitnumberLane[istave][2 * ihic] = 0;
            mHitnumberLane[istave][2 * ihic + 1] = 0;
            mOccupancyLane[istave][2 * ihic] = 0;
            mOccupancyLane[istave][2 * ihic + 1] = 0;
            mChipStaveOccupancy->GetXaxis()->SetBinLabel(2 * ihic + 1, Form("%s", OBLabel56[2 * ihic]));
            mChipStaveOccupancy->GetXaxis()->SetBinLabel(2 * ihic + 2, Form("%s", OBLabel56[2 * ihic + 1]));
            mChipStaveEventHitCheck->GetXaxis()->SetBinLabel(2 * ihic + 1, Form("%s", OBLabel56[2 * ihic]));
            mChipStaveEventHitCheck->GetXaxis()->SetBinLabel(2 * ihic + 2, Form("%s", OBLabel56[2 * ihic + 1]));
          }
        }
      }
    }
  }
}

void ITSFhrTask::createErrorTriggerPlots()
{
  mErrorPlots = new TH1D("General/ErrorPlots", "Decoding Errors", o2::itsmft::GBTLinkDecodingStat::NErrorsDefined, 0.5, o2::itsmft::GBTLinkDecodingStat::NErrorsDefined + 0.5);
  mErrorPlots->SetMinimum(0);
  mErrorPlots->SetFillColor(kRed);
  getObjectsManager()->startPublishing(mErrorPlots); // mErrorPlots
}

void ITSFhrTask::createGeneralPlots()
{

  createErrorTriggerPlots();

  mErrorVsFeeid = new TH2I("General/ErrorVsFeeid", "Error count vs Error id and Fee id", (3 * StaveBoundary[3]) + (2 * (StaveBoundary[7] - StaveBoundary[3])), 0, (3 * StaveBoundary[3]) + (2 * (StaveBoundary[7] - StaveBoundary[3])), o2::itsmft::GBTLinkDecodingStat::NErrorsDefined, 0.5, o2::itsmft::GBTLinkDecodingStat::NErrorsDefined + 0.5);
  mErrorVsFeeid->SetMinimum(0);
  mErrorVsFeeid->SetStats(0);
  getObjectsManager()->startPublishing(mErrorVsFeeid);
}

void ITSFhrTask::createOccupancyPlots() // create general plots like error, trigger, TF id plots and so on....
                                        // create occupancy plots like chip stave occupancy, occupancy distribution, hic hit map plots and so on....
{
  const int nDim(2);
  int nBins[nDim] = { 1024, 512 };
  double Min[nDim] = { 0, 0 };
  double Max[nDim] = { 1024, 512 };
  mTotalDeadChipPos = new TH2D(Form("Occupancy/TotalDeadChipPos"), Form("TotalDeadChipPos "), nHicPerStave[6] * 7 * 0.5, -0.5 * mLength[6], 0.5 * mLength[6], NStaves[6] * 4, -180, 180);
  mTotalDeadChipPos->SetStats(0);
  getObjectsManager()->startPublishing(mTotalDeadChipPos);

  mTotalAliveChipPos = new TH2D(Form("Occupancy/TotalAliveChipPos"), Form("TotalAliveChipPos "), nHicPerStave[6] * 7 * 0.5, -0.5 * mLength[6], 0.5 * mLength[6], NStaves[6] * 4, -180, 180);
  mTotalAliveChipPos->SetStats(0);
  getObjectsManager()->startPublishing(mTotalAliveChipPos);
  // create IB plots
  if (mLayer < NLayerIB) {
    int nBinstmp[nDim] = { nBins[0] * nChipsPerHic[mLayer] / ReduceFraction, nBins[1] / ReduceFraction };
    double Maxtmp[nDim] = { Max[0] * nChipsPerHic[mLayer], Max[1] };
    for (int istave = 0; istave < 48; istave++) {
      mStaveHitmap[istave] = new THnSparseI(Form("Occupancy/Layer%d/Stave%d/Layer%dStave%dHITMAP", mLayer, istave, mLayer, istave), Form("Hits on Layer %d, Stave %d", mLayer, istave), nDim, nBinstmp, Min, Maxtmp);
      if (istave < NStaves[mLayer]) {
        getObjectsManager()->startPublishing(mStaveHitmap[istave]);
      }
    }

    mDeadChipPos = new TH2D(Form("Occupancy/Layer%d/Layer%dDeadChipPos", mLayer, mLayer), Form("DeadChipPos on Layer %d", mLayer), nChipsPerHic[mLayer], -0.5 * mLength[mLayer], 0.5 * mLength[mLayer], NStaves[mLayer], -180, 180);
    mAliveChipPos = new TH2D(Form("Occupancy/Layer%d/Layer%dAliveChipPos", mLayer, mLayer), Form("AliveChipPos on Layer %d", mLayer), nChipsPerHic[mLayer], -0.5 * mLength[mLayer], 0.5 * mLength[mLayer], NStaves[mLayer], -180, 180);

    mChipStaveOccupancy = new TH2D(Form("Occupancy/Layer%d/Layer%dChipStave", mLayer, mLayer), Form("ITS Layer%d, Occupancy vs Chip and Stave", mLayer), nHicPerStave[mLayer] * nChipsPerHic[mLayer], -0.5, nHicPerStave[mLayer] * nChipsPerHic[mLayer] - 0.5, NStaves[mLayer], -0.5, NStaves[mLayer] - 0.5);
    mChipStaveOccupancy->SetStats(0);
    mChipStaveOccupancy->GetYaxis()->SetTickLength(0.01);
    getObjectsManager()->startPublishing(mDeadChipPos);
    getObjectsManager()->startPublishing(mAliveChipPos);
    getObjectsManager()->startPublishing(mChipStaveOccupancy); // mChipStaveOccupancy
    mChipStaveEventHitCheck = new TH2I(Form("Occupancy/Layer%d/Layer%dChipStaveEventHit", mLayer, mLayer), Form("ITS Layer%d, Event Hit Check vs Chip and Stave", mLayer), nHicPerStave[mLayer] * nChipsPerHic[mLayer], -0.5, nHicPerStave[mLayer] * nChipsPerHic[mLayer] - 0.5, NStaves[mLayer], -0.5, NStaves[mLayer] - 0.5);
    mChipStaveEventHitCheck->SetStats(0);
    getObjectsManager()->startPublishing(mChipStaveEventHitCheck);

    mOccupancyPlot = new TH1D(Form("Occupancy/Layer%dOccupancy", mLayer), Form("ITS Layer %d Noise pixels occupancy distribution", mLayer), 300, -15, 0);
    getObjectsManager()->startPublishing(mOccupancyPlot); // mOccupancyPlot

  } else {
    // Create OB plots
    int nBinstmp[nDim] = { (nBins[0] * (nChipsPerHic[mLayer] / 2) * (nHicPerStave[mLayer] / 2) / ReduceFraction), (nBins[1] * 2 * NSubStave[mLayer] / ReduceFraction) };
    double Maxtmp[nDim] = { (double)(nBins[0] * (nChipsPerHic[mLayer] / 2) * (nHicPerStave[mLayer] / 2)), (double)(nBins[1] * 2 * NSubStave[mLayer]) };
    for (int istave = 0; istave < NStaves[mLayer]; istave++) {
      mStaveHitmap[istave] = new THnSparseI(Form("Occupancy/Layer%d/Stave%d/Layer%dStave%dHITMAP", mLayer, istave, mLayer, istave), Form("Hits on Layer %d, Stave %d", mLayer, istave), nDim, nBinstmp, Min, Maxtmp);
      getObjectsManager()->startPublishing(mStaveHitmap[istave]);
    }
    mDeadChipPos = new TH2D(Form("Occupancy/Layer%d/Layer%dDeadChipPos", mLayer, mLayer), Form("DeadChipPos on Layer %d", mLayer), nHicPerStave[mLayer] * 7 * 0.5, -0.5 * mLength[mLayer], 0.5 * mLength[mLayer], NStaves[mLayer] * 4, -180, 180);
    mAliveChipPos = new TH2D(Form("Occupancy/Layer%d/Layer%dAliveChipPos", mLayer, mLayer), Form("AliveChipPos on Layer %d", mLayer), nHicPerStave[mLayer] * 7 * 0.5, -0.5 * mLength[mLayer], 0.5 * mLength[mLayer], NStaves[mLayer] * 4, -180, 180);

    mChipStaveOccupancy = new TH2D(Form("Occupancy/Layer%d/Layer%dChipStave", mLayer, mLayer), Form("ITS Layer%d, Occupancy vs Chip and Stave", mLayer), nHicPerStave[mLayer] * nLanePerHic[mLayer], -0.5, nHicPerStave[mLayer] * nLanePerHic[mLayer] - 0.5, NStaves[mLayer], -0.5, NStaves[mLayer] - 0.5);
    mChipStaveOccupancy->SetStats(0);
    getObjectsManager()->startPublishing(mDeadChipPos);
    getObjectsManager()->startPublishing(mAliveChipPos);
    getObjectsManager()->startPublishing(mChipStaveOccupancy);

    mChipStaveEventHitCheck = new TH2I(Form("Occupancy/Layer%d/Layer%dChipStaveEventHit", mLayer, mLayer), Form("ITS Layer%d, Event Hit Check vs Chip and Stave", mLayer), nHicPerStave[mLayer] * nLanePerHic[mLayer], -0.5, nHicPerStave[mLayer] * nLanePerHic[mLayer] - 0.5, NStaves[mLayer], -0.5, NStaves[mLayer] - 0.5);
    mChipStaveEventHitCheck->SetStats(0);
    getObjectsManager()->startPublishing(mChipStaveEventHitCheck);

    mOccupancyPlot = new TH1D(Form("Occupancy/Layer%dOccupancy", mLayer), Form("ITS Layer %d Noise pixels occupancy Distribution", mLayer), 300, -15, 0);
    getObjectsManager()->startPublishing(mOccupancyPlot); // mOccupancyPlot
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
  if (mErrorVsFeeid) {
    setAxisTitle(mErrorVsFeeid, "FeeID", "Error ID");
  }
  if (mTotalDeadChipPos) {
    setAxisTitle(mTotalDeadChipPos, "ChipZ", "ChipPhi");
  }
  if (mTotalAliveChipPos) {
    setAxisTitle(mTotalAliveChipPos, "ChipZ", "ChipPhi");
  }
  if (mOccupancyPlot) {
    mOccupancyPlot->GetXaxis()->SetTitle("log(Occupancy)");
  }

  if (mDeadChipPos) {
    setAxisTitle(mDeadChipPos, "ChipZ", "ChipPhi");
  }
  if (mAliveChipPos) {
    setAxisTitle(mAliveChipPos, "ChipZ", "ChipPhi");
  }

  if (mLayer < NLayerIB) {
    if (mChipStaveOccupancy) {
      setAxisTitle(mChipStaveOccupancy, "Chip Number", "Stave Number");
    }
    if (mChipStaveEventHitCheck) {
      setAxisTitle(mChipStaveEventHitCheck, "Chip Number", "Stave Number");
    }
  } else {
    if (mChipStaveOccupancy) {
      setAxisTitle(mChipStaveOccupancy, "", "Stave Number");
    }
    if (mChipStaveEventHitCheck) {
      setAxisTitle(mChipStaveEventHitCheck, "", "Stave Number");
    }
  }
}

void ITSFhrTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity : " << activity.mId << ENDM;
  reset();
}

void ITSFhrTask::startOfCycle() { ILOG(Debug, Devel) << "startOfCycle" << ENDM; }

void ITSFhrTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  if (ctx.services().get<o2::framework::TimingInfo>().globalRunNumberChanged) {
    mGeom = o2::its::GeometryTGeo::Instance();
    mGeom->fillMatrixCache(o2::math_utils::bit2Mask(o2::math_utils::TransformType::L2G));
  }
  // set timer
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  std::chrono::time_point<std::chrono::high_resolution_clock> end;
  int difference;
  start = std::chrono::high_resolution_clock::now();

  // set Decoder
  mDecoder->startNewTF(ctx.inputs());
  mDecoder->setDecodeNextAuto(true);

  // define digit hit vector
  std::vector<Digit>** digVec = new std::vector<Digit>*[NStaves[mLayer]]; // IB : digVec[stave][0]; OB : digVec[stave][hic]
  const math_utils::Point3D<float> loc(0., 0., 0.);

  if (mLayer < NLayerIB) {
    for (int istave = 0; istave < NStaves[mLayer]; istave++) {
      digVec[istave] = new std::vector<Digit>[nHicPerStave[mLayer]];
    }
  } else {
    for (int istave = 0; istave < NStaves[mLayer]; istave++) {
      digVec[istave] = new std::vector<Digit>[nHicPerStave[mLayer]];
    }
  }

  // decode raw data and save digit hit to digit hit vector, and save hitnumber per chip/hic
  // get the position of all chips in this layer
  for (int ichip = ChipBoundary[mLayer]; ichip < ChipBoundary[mLayer + 1]; ichip++) {
    int stave = 0, chip = 0;
    auto glo = mGeom->getMatrixL2G(ichip)(loc);
    if (mLayer < NLayerIB) {
      stave = ichip / 9 - StaveBoundary[mLayer];
      chip = ichip % 9;
      mChipPhi[stave][chip] = glo.phi() * 180 / TMath::Pi();
      mChipZ[stave][chip] = glo.Z();
    } else {
      stave = (ichip - ChipBoundary[mLayer]) / (14 * nHicPerStave[mLayer]);
      chip = (ichip - ChipBoundary[mLayer]) % (14 * nHicPerStave[mLayer]);
      mChipPhi[stave][chip] = glo.phi() * 180 / TMath::Pi();
      mChipZ[stave][chip] = glo.Z();
    }
  }

  while ((mChipDataBuffer = mDecoder->getNextChipData(mChipsBuffer))) {
    if (mChipDataBuffer) {
      int stave = 0, chip = 0;
      int hic = 0;
      int lane = 0;

      const auto& pixels = mChipDataBuffer->getData();
      if (mChipDataBuffer->getChipID() < ChipBoundary[mLayer] || mChipDataBuffer->getChipID() >= ChipBoundary[mLayer + 1]) { // useful for data replay
        continue;
      }
      for (auto& pixel : pixels) {
        if (mLayer < NLayerIB) {
          stave = mChipDataBuffer->getChipID() / 9 - StaveBoundary[mLayer];
          chip = mChipDataBuffer->getChipID() % 9;
          hic = 0;
          mHitnumberLane[stave][chip]++;
          mChipStat[stave][chip]++;
        } else {
          stave = (mChipDataBuffer->getChipID() - ChipBoundary[mLayer]) / (14 * nHicPerStave[mLayer]);
          int chipIdLocal = (mChipDataBuffer->getChipID() - ChipBoundary[mLayer]) % (14 * nHicPerStave[mLayer]);
          chip = chipIdLocal % 14;
          hic = (chipIdLocal % (14 * nHicPerStave[mLayer])) / 14;
          lane = (chipIdLocal % (14 * nHicPerStave[mLayer])) / (14 / 2);

          mHitnumberLane[stave][lane]++;
          mChipStat[stave][chipIdLocal]++;
        }
        digVec[stave][hic].emplace_back(mChipDataBuffer->getChipID(), pixel.getRow(), pixel.getCol());
      }
      if (mLayer < NLayerIB) {
        if (pixels.size() > (unsigned int)mHitCutForCheck) {
          mChipStaveEventHitCheck->Fill(chip, stave);
        }
      } else {
        if (pixels.size() > (unsigned int)mHitCutForCheck) {
          mChipStaveEventHitCheck->Fill(lane, stave);
        }
      }
    }
  }

  // calculate active staves according digit hit vector
  std::vector<int> activeStaves;
  for (int i = 0; i < NStaves[mLayer]; i++) {
    for (int j = 0; j < nHicPerStave[mLayer]; j++) {
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
    if (mLayer < NLayerIB) {
      for (auto& digit : digVec[istave][0]) {
        int chip = digit.getChipIndex() % 9;
        mHitPixelID_InStave[istave][0][chip][1000 * digit.getColumn() + digit.getRow()]++;
        if (mTFCount <= mCutTFForSparse) {
          Double_t pixelPos[2] = { 1. * (digit.getColumn() + (1024 * chip)), 1. * digit.getRow() };
          mStaveHitmap[istave]->Fill(pixelPos);
        }
      }
    } else {
      for (int ihic = 0; ihic < nHicPerStave[mLayer]; ihic++) {
        for (auto& digit : digVec[istave][ihic]) {
          int chip = ((digit.getChipIndex() - ChipBoundary[mLayer]) % (14 * nHicPerStave[mLayer])) % 14;
          mHitPixelID_InStave[istave][ihic][chip][1000 * digit.getColumn() + digit.getRow()]++;
          int ilink = ihic / (nHicPerStave[mLayer] / 2);
          if (mTFCount <= mCutTFForSparse) {
            if (chip < 7) {
              Double_t pixelPos[2] = { 1. * ((ihic % (nHicPerStave[mLayer] / NSubStave[mLayer]) * ((nChipsPerHic[mLayer] / 2) * NCols)) + chip * NCols + digit.getColumn()), 1. * (NRows - digit.getRow() - 1 + (1024 * ilink)) };
              mStaveHitmap[istave]->Fill(pixelPos);
            } else {
              Double_t pixelPos[2] = { 1. * ((ihic % (nHicPerStave[mLayer] / NSubStave[mLayer]) * ((nChipsPerHic[mLayer] / 2) * NCols)) + (nChipsPerHic[mLayer] / 2) * NCols - (chip - 7) * NCols - digit.getColumn() * 1.), 1. * (NRows + digit.getRow() + (1024 * ilink)) };
              mStaveHitmap[istave]->Fill(pixelPos);
            }
          }
        }
      }
    }
  }

  // Reset Error plots
  mErrorPlots->Reset();
  mErrorVsFeeid->Reset(); // Error is   statistic by decoder so if we didn't reset decoder, then we need reset Error plots, and use TH::SetBinContent function

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
    if (digVec[istave][0].size() < 1 && mLayer < NLayerIB) {
      continue;
    }
    const auto* DecoderTmp = mDecoder;
    int RUid = StaveBoundary[mLayer] + istave;
    const o2::itsmft::RUDecodeData* RUdecode = DecoderTmp->getRUDecode(RUid);
    if (!RUdecode) {
      continue;
    }
    if (mLayer < NLayerIB) {
      for (int ilink = 0; ilink < RUDecodeData::MaxLinksPerRU; ilink++) {
        const auto* GBTLinkInfo = DecoderTmp->getGBTLink(RUdecode->links[ilink]);
        if (!GBTLinkInfo) {
          continue;
        }

        mNoisyPixelNumber[mLayer][istave] = 0;
        for (int ichip = 0 + (ilink * 3); ichip < (ilink * 3) + 3; ichip++) {
          std::unordered_map<unsigned int, int>::iterator iter;

          if (mDoHitmapFilter == 1) {
            for (auto iter = mHitPixelID_InStave[istave][0][ichip].begin(); iter != mHitPixelID_InStave[istave][0][ichip].end();) {
              if ((double)iter->second / GBTLinkInfo->statistics.nTriggers < mPhysicalOccupancyIB) { // 40 hits/cm^2 * 5 pixels/hits * 4.5 cm^2 / 1024 / 512 = 1.7e-3/pixel/event for physics
                mHitPixelID_InStave[istave][0][ichip].erase(iter++);
              } else
                ++iter;
            }
          }

          for (auto iter = mHitPixelID_InStave[istave][0][ichip].begin(); iter != mHitPixelID_InStave[istave][0][ichip].end(); iter++) {
            if ((iter->second > mHitCutForNoisyPixel) &&
                (iter->second / (double)GBTLinkInfo->statistics.nTriggers) > mOccupancyCutForNoisyPixel) {
              mNoisyPixelNumber[mLayer][istave]++; // count only in 10000 events as soon as nTriggers is 1e6
              occupancyPlotTmp[i]->Fill(log10((double)iter->second / GBTLinkInfo->statistics.nTriggers));
            }

            totalhit += (int)iter->second;
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
        mNoisyPixelNumber[mLayer][istave] = 0;

        for (int ihic = 0; ihic < ((nHicPerStave[mLayer] / NSubStave[mLayer])); ihic++) {
          for (int ichip = 0; ichip < nChipsPerHic[mLayer]; ichip++) {
            if (GBTLinkInfo->statistics.nTriggers > 0) {

              if (mDoHitmapFilter == 1) {
                for (auto iter = mHitPixelID_InStave[istave][ihic + ilink * ((nHicPerStave[mLayer] / NSubStave[mLayer]))][ichip].begin(); iter != mHitPixelID_InStave[istave][ihic + ilink * ((nHicPerStave[mLayer] / NSubStave[mLayer]))][ichip].end();) {
                  if ((double)iter->second / GBTLinkInfo->statistics.nTriggers < mPhysicalOccupancyOB) { // 1 hits/cm^2 * 5 pixels/hits * 4.5 cm^2 / 1024 / 512 = 4.3e-5/pixel/event`
                    mHitPixelID_InStave[istave][ihic + ilink * ((nHicPerStave[mLayer] / NSubStave[mLayer]))][ichip].erase(iter++);
                  } else
                    ++iter;
                }
              }
              for (auto iter = mHitPixelID_InStave[istave][ihic + ilink * ((nHicPerStave[mLayer] / NSubStave[mLayer]))][ichip].begin(); iter != mHitPixelID_InStave[istave][ihic + ilink * ((nHicPerStave[mLayer] / NSubStave[mLayer]))][ichip].end(); iter++) {
                if ((iter->second > mHitCutForNoisyPixel) &&
                    (iter->second / (double)GBTLinkInfo->statistics.nTriggers) > mOccupancyCutForNoisyPixel) {
                  mNoisyPixelNumber[mLayer][istave]++;
                  occupancyPlotTmp[i]->Fill(log10((double)iter->second / GBTLinkInfo->statistics.nTriggers));
                }
              }
            }
          }

          if (mLayer == 3 || mLayer == 4) {
            mOccupancyLane[istave][2 * (ihic + (ilink * 4))] = mHitnumberLane[istave][2 * (ihic + (ilink * 4))] / (GBTLinkInfo->statistics.nTriggers * 1024. * 512. * nChipsPerHic[mLayer] / 2);
            mOccupancyLane[istave][2 * (ihic + (ilink * 4)) + 1] = mHitnumberLane[istave][2 * (ihic + (ilink * 4)) + 1] / (GBTLinkInfo->statistics.nTriggers * 1024. * 512. * nChipsPerHic[mLayer] / 2);
          } else {
            mOccupancyLane[istave][2 * (ihic + (ilink * 7))] = mHitnumberLane[istave][2 * (ihic + (ilink * 7))] / (GBTLinkInfo->statistics.nTriggers * 1024. * 512. * nChipsPerHic[mLayer] / 2);
            mOccupancyLane[istave][2 * (ihic + (ilink * 7)) + 1] = mHitnumberLane[istave][2 * (ihic + (ilink * 7)) + 1] / (GBTLinkInfo->statistics.nTriggers * 1024. * 512. * nChipsPerHic[mLayer] / 2);
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
    mOccupancyPlot->Add(occupancyPlotTmp[i]);
    if (mLayer < NLayerIB) {
      for (int ichip = 0; ichip < nChipsPerHic[mLayer]; ichip++) {
        mChipStaveOccupancy->SetBinContent(ichip + 1, istave + 1, mOccupancyLane[istave][ichip]);
        if (!mChipStat[istave][ichip]) {
          mDeadChipPos->SetBinContent(mDeadChipPos->GetXaxis()->FindBin(mChipZ[istave][ichip]), mDeadChipPos->GetYaxis()->FindBin(mChipPhi[istave][ichip]), 1);
          mTotalDeadChipPos->SetBinContent(mTotalDeadChipPos->GetXaxis()->FindBin(mChipZ[istave][ichip]), mTotalDeadChipPos->GetYaxis()->FindBin(mChipPhi[istave][ichip]), 1);
        } else {
          mAliveChipPos->SetBinContent(mAliveChipPos->GetXaxis()->FindBin(mChipZ[istave][ichip]), mAliveChipPos->GetYaxis()->FindBin(mChipPhi[istave][ichip]), 1);
          mTotalAliveChipPos->SetBinContent(mTotalAliveChipPos->GetXaxis()->FindBin(mChipZ[istave][ichip]), mTotalAliveChipPos->GetYaxis()->FindBin(mChipPhi[istave][ichip]), 1);
          mDeadChipPos->SetBinContent(mDeadChipPos->GetXaxis()->FindBin(mChipZ[istave][ichip]), mDeadChipPos->GetYaxis()->FindBin(mChipPhi[istave][ichip]), 0);                // not dead
          mTotalDeadChipPos->SetBinContent(mTotalDeadChipPos->GetXaxis()->FindBin(mChipZ[istave][ichip]), mTotalDeadChipPos->GetYaxis()->FindBin(mChipPhi[istave][ichip]), 0); // not dead
        }
        int ilink = ichip / 3;
        for (int ierror = 0; ierror < o2::itsmft::GBTLinkDecodingStat::NErrorsDefined; ierror++) {
          if (mErrorVsFeeid && (mErrorCount[istave][ilink][ierror] != 0)) {
            mErrorVsFeeid->SetBinContent(((istave + StaveBoundary[mLayer]) * 3) + ilink + 1, ierror + 1, mErrorCount[istave][ilink][ierror]);
          }
        }
      }
      mGeneralOccupancy->SetBinContent(istave + 1 + StaveBoundary[mLayer], *(std::max_element(mOccupancyLane[istave], mOccupancyLane[istave] + nChipsPerHic[mLayer])));
      mGeneralNoisyPixel->SetBinContent(istave + 1 + StaveBoundary[mLayer], mNoisyPixelNumber[mLayer][istave]);
    } else {
      for (int ichip = 0; ichip < nHicPerStave[mLayer] * nChipsPerHic[mLayer]; ichip++) {
        if (!mChipStat[istave][ichip]) {
          mDeadChipPos->SetBinContent(mDeadChipPos->GetXaxis()->FindBin(mChipZ[istave][ichip]), mDeadChipPos->GetYaxis()->FindBin(mChipPhi[istave][ichip]), 1);
          mTotalDeadChipPos->SetBinContent(mTotalDeadChipPos->GetXaxis()->FindBin(mChipZ[istave][ichip]), mTotalDeadChipPos->GetYaxis()->FindBin(mChipPhi[istave][ichip]), 1);
        } else {
          mAliveChipPos->SetBinContent(mAliveChipPos->GetXaxis()->FindBin(mChipZ[istave][ichip]), mAliveChipPos->GetYaxis()->FindBin(mChipPhi[istave][ichip]), 1);
          mTotalAliveChipPos->SetBinContent(mTotalAliveChipPos->GetXaxis()->FindBin(mChipZ[istave][ichip]), mTotalAliveChipPos->GetYaxis()->FindBin(mChipPhi[istave][ichip]), 1);
          mDeadChipPos->SetBinContent(mDeadChipPos->GetXaxis()->FindBin(mChipZ[istave][ichip]), mDeadChipPos->GetYaxis()->FindBin(mChipPhi[istave][ichip]), 0);                // not dead
          mTotalDeadChipPos->SetBinContent(mTotalDeadChipPos->GetXaxis()->FindBin(mChipZ[istave][ichip]), mTotalDeadChipPos->GetYaxis()->FindBin(mChipPhi[istave][ichip]), 0); // not dead
        }
      }

      for (int ihic = 0; ihic < nHicPerStave[mLayer]; ihic++) {
        int ilink = ihic / (nHicPerStave[mLayer] / 2);
        mChipStaveOccupancy->SetBinContent(2 * ihic + 1, istave + 1, mOccupancyLane[istave][2 * ihic]);
        mChipStaveOccupancy->SetBinContent(2 * ihic + 2, istave + 1, mOccupancyLane[istave][2 * ihic + 1]);
        if (ihic == 0 || ihic == 7) {
          for (int ierror = 0; ierror < o2::itsmft::GBTLinkDecodingStat::NErrorsDefined; ierror++) {
            if (mErrorVsFeeid && (mErrorCount[istave][ilink][ierror] != 0)) {
              mErrorVsFeeid->SetBinContent((3 * StaveBoundary[3]) + ((StaveBoundary[mLayer] - StaveBoundary[NLayerIB] + istave) * 2) + ilink + 1, ierror + 1, mErrorCount[istave][ilink][ierror]);
            }
          }
        }
      }
      mGeneralOccupancy->SetBinContent(istave + 1 + StaveBoundary[mLayer], *(std::max_element(mOccupancyLane[istave], mOccupancyLane[istave] + nHicPerStave[mLayer] * 2)));
      mGeneralNoisyPixel->SetBinContent(istave + 1 + StaveBoundary[mLayer], mNoisyPixelNumber[mLayer][istave]);
    }
  }

  mDeadChipPos->ResetStats();
  mAliveChipPos->ResetStats();

  for (int ierror = 0; ierror < o2::itsmft::GBTLinkDecodingStat::NErrorsDefined; ierror++) {
    int feeError = mErrorVsFeeid->Integral(1, mErrorVsFeeid->GetXaxis()->GetNbins(), ierror + 1, ierror + 1);
    mErrorPlots->SetBinContent(ierror + 1, feeError);
  }

  // delete pointor in monitorData()
  for (int istave = 0; istave < NStaves[mLayer]; istave++) {
    delete[] digVec[istave];
  }
  delete[] digVec;

  for (int i = 0; i < (int)activeStaves.size(); i++) {
    delete occupancyPlotTmp[i];
  }
  delete[] occupancyPlotTmp;

  end = std::chrono::high_resolution_clock::now();
  difference = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

  mAverageProcessTime += difference;
  mTFCount++;
}

void ITSFhrTask::getParameters()
{
  mNThreads = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "decoderThreads", mNThreads);
  mLayer = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "Layer", mLayer);
  mHitCutForCheck = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "HitNumberCut", mHitCutForCheck);
  mHitCutForNoisyPixel = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "HitNumberCutForNoisyPixel", mHitCutForNoisyPixel);
  mOccupancyCutForNoisyPixel = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "OccupancyNumberCutForNoisyPixel", mOccupancyCutForNoisyPixel);
  mMaxGeneralAxisRange = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "MaxGeneralAxisRange", mMaxGeneralAxisRange);
  mMinGeneralAxisRange = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "MinGeneralAxisRange", mMinGeneralAxisRange);
  mMaxGeneralNoisyAxisRange = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "MaxGeneralNoisyAxisRange", mMaxGeneralNoisyAxisRange);
  mMinGeneralNoisyAxisRange = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "MinGeneralNoisyAxisRange", mMinGeneralNoisyAxisRange);
  mPhibins = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "Phibins", mPhibins);
  mEtabins = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "Etabins", mEtabins);
  mCutTFForSparse = o2::quality_control_modules::common::getFromConfig<double>(mCustomParameters, "CutSparseTF", mCutTFForSparse);
  mDoHitmapFilter = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "DoHitmapFilter", mDoHitmapFilter);
  mPhysicalOccupancyIB = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "PhysicalOccupancyIB", mPhysicalOccupancyIB);
  mPhysicalOccupancyOB = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "PhysicalOccupancyOB", mPhysicalOccupancyOB);
}

void ITSFhrTask::endOfCycle()
{
  ILOG(Debug, Support) << "average process time == " << (double)mAverageProcessTime / mTFCount << ENDM;
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void ITSFhrTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void ITSFhrTask::resetGeneralPlots()
{
  resetObject(mErrorPlots);
  resetObject(mErrorVsFeeid);
}

void ITSFhrTask::resetOccupancyPlots()
{
  memset(mHitNumberOfChip, 0, sizeof(mHitNumberOfChip));
  memset(mErrors, 0, sizeof(mErrors));
  mChipStaveOccupancy->Reset();
  mChipStaveEventHitCheck->Reset();
  mOccupancyPlot->Reset();
  mDeadChipPos->Reset();
  mAliveChipPos->Reset();
  mTotalDeadChipPos->Reset();
  mTotalAliveChipPos->Reset();
  for (int istave = 0; istave < NStaves[mLayer]; istave++) {
    mStaveHitmap[istave]->Reset();
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

  ILOG(Debug, Devel) << "Reset" << ENDM;
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
