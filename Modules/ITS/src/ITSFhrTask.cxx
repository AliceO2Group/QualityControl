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
/// \file   ITSFhrTask.cxx
/// \author Liang Zhang
/// \author Jian Liu
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
  for (int istave = 0; istave < NStaves[mLayer]; istave++) {
    delete mStaveHitmap[mLayer][istave];
  }
  for (int istave = 0; istave < NStaves[mLayer]; istave++) {
    delete[] mHitnumber[istave];
    delete[] mOccupancy[istave];
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
  delete[] mHitnumber;
  delete[] mOccupancy;
  delete[] mErrorCount;
  delete[] mHitPixelID_InStave;
}

void ITSFhrTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info) << "initialize ITSFhrTask" << ENDM;
  getParameters();
  o2::base::GeometryManager::loadGeometry(mGeomPath.c_str());
  mGeom = o2::its::GeometryTGeo::Instance();

  mGeneralOccupancy = new TH2Poly();
  mGeneralOccupancy->SetTitle("General Occupancy;mm;mm");
  mGeneralOccupancy->SetName("General/General_Occupancy");
  mGeneralOccupancy->SetStats(0);

  createGeneralPlots();
  createOccupancyPlots();
  setPlotsFormat();
  mDecoder = new o2::itsmft::RawPixelDecoder<o2::itsmft::ChipMappingITS>();
  mDecoder->init();
  mDecoder->setNThreads(mNThreads);
  mDecoder->setFormat(GBTLink::NewFormat);               //Using RDHv6 (NewFormat)
  mDecoder->setUserDataOrigin(header::DataOrigin("DS")); //set user data origin in dpl
  mDecoder->setUserDataDescription(header::DataDescription("RAWDATA0"));
  mChipsBuffer.resize(mGeom->getNumberOfChips());

  if (mLayer != -1) {
    //define the hitnumber, occupancy, errorcount array
    mHitPixelID_InStave = new std::unordered_map<unsigned int, int>**[NStaves[mLayer]];
    mHitnumber = new int*[NStaves[mLayer]];
    mOccupancy = new double*[NStaves[mLayer]];
    mErrorCount = new int**[NStaves[mLayer]];

    for (int istave = 0; istave < NStaves[mLayer]; istave++) {
      double* px = new double[4];
      double* py = new double[4];
      getStavePoint(mLayer, istave, px, py);
      mGeneralOccupancy->AddBin(4, px, py);
    }
    if (mGeneralOccupancy) {
      getObjectsManager()->startPublishing(mGeneralOccupancy);
    }

    //define the errorcount array, there is some reason cause break when I define errorcount and hitnumber, occupancy at same block.
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
    //define the hitnumber and occupancy array
    if (mLayer < NLayerIB) {
      for (int istave = 0; istave < NStaves[mLayer]; istave++) {
        mHitnumber[istave] = new int[nChipsPerHic[mLayer]];
        mOccupancy[istave] = new double[nChipsPerHic[mLayer]];
        mHitPixelID_InStave[istave] = new std::unordered_map<unsigned int, int>*[nHicPerStave[mLayer]];
        for (int ihic = 0; ihic < nHicPerStave[mLayer]; ihic++) {
          mHitPixelID_InStave[istave][ihic] = new std::unordered_map<unsigned int, int>[nChipsPerHic[mLayer]];
        }
        for (int ichip = 0; ichip < nChipsPerHic[mLayer]; ichip++) {
          mHitnumber[istave][ichip] = 0;
          mOccupancy[istave][ichip] = 0;
        }
      }
    } else {
      for (int istave = 0; istave < NStaves[mLayer]; istave++) {
        mHitnumber[istave] = new int[nHicPerStave[mLayer]];
        mOccupancy[istave] = new double[nHicPerStave[mLayer]];
        mHitPixelID_InStave[istave] = new std::unordered_map<unsigned int, int>*[nHicPerStave[mLayer]];
        for (int ihic = 0; ihic < nHicPerStave[mLayer]; ihic++) {
          mHitPixelID_InStave[istave][ihic] = new std::unordered_map<unsigned int, int>[nChipsPerHic[mLayer]];
        }
        for (int ihic = 0; ihic < nHicPerStave[mLayer]; ihic++) {
          mHitnumber[istave][ihic] = 0;
          mOccupancy[istave][ihic] = 0;
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
  getObjectsManager()->startPublishing(mErrorPlots); //mErrorPlots

  mTriggerPlots = new TH1D("General/TriggerPlots", "Decoding Triggers", mNTrigger, 0.5, mNTrigger + 0.5);
  mTriggerPlots->SetMinimum(0);
  mTriggerPlots->SetFillColor(kBlue);
  getObjectsManager()->startPublishing(mTriggerPlots); //mTriggerPlots
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

  mInfoCanvasComm->GetZaxis()->SetRangeUser(0, 2);
  mInfoCanvasComm->GetYaxis()->SetBinLabel(2, "ibt");
  mInfoCanvasComm->GetYaxis()->SetBinLabel(4, "ibb");

  mInfoCanvasOBComm->GetZaxis()->SetRangeUser(0, 2);
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
  getObjectsManager()->startPublishing(mTFInfo); //mTFInfo

  mErrorVsFeeid = new TH2I("General/ErrorVsFeeid", "Error count vs Error id and Fee id", (3 * StaveBoundary[3]) + (2 * (StaveBoundary[7] - StaveBoundary[3])), 0, (3 * StaveBoundary[3]) + (2 * (StaveBoundary[7] - StaveBoundary[3])), o2::itsmft::GBTLinkDecodingStat::NErrorsDefined, 0.5, o2::itsmft::GBTLinkDecodingStat::NErrorsDefined + 0.5);
  mTriggerVsFeeid = new TH2I("General/TriggerVsFeeid", "Trigger count vs Trigger id and Fee id", (3 * StaveBoundary[3]) + (2 * (StaveBoundary[7] - StaveBoundary[3])), 0, (3 * StaveBoundary[3]) + (2 * (StaveBoundary[7] - StaveBoundary[3])), mNTrigger, 0.5, mNTrigger + 0.5);
  mErrorVsFeeid->SetMinimum(0);
  mTriggerVsFeeid->SetMinimum(0);
  mErrorVsFeeid->SetStats(0);
  mTriggerVsFeeid->SetStats(0);
  getObjectsManager()->startPublishing(mErrorVsFeeid);
  getObjectsManager()->startPublishing(mTriggerVsFeeid);
}

void ITSFhrTask::createOccupancyPlots() //create general plots like error, trigger, TF id plots and so on....
                                        //create occupancy plots like chip stave occupancy, occupancy distribution, hic hit map plots and so on....
{
  const int nDim(2);
  int nBins[nDim] = { 1024, 512 };
  double Min[nDim] = { 0, 0 };
  double Max[nDim] = { 1024, 512 };

  //create IB plots
  if (mLayer < NLayerIB) {
    int nBinstmp[nDim] = { nBins[0] * nChipsPerHic[mLayer] / ReduceFraction, nBins[1] / ReduceFraction };
    double Maxtmp[nDim] = { Max[0] * nChipsPerHic[mLayer], Max[1] };
    for (int istave = 0; istave < NStaves[mLayer]; istave++) {
      mStaveHitmap[mLayer][istave] = new THnSparseI(Form("Occupancy/Layer%d/Stave%d/Layer%dStave%dHITMAP", mLayer, istave, mLayer, istave), Form("Hits on Layer %d, Stave %d", mLayer, istave), nDim, nBinstmp, Min, Maxtmp);
      getObjectsManager()->startPublishing(mStaveHitmap[mLayer][istave]);
    }

    mChipStaveOccupancy[mLayer] = new TH2D(Form("Occupancy/Layer%d/Layer%dChipStave", mLayer, mLayer), Form("ITS Layer%d, Occupancy vs Chip and Stave", mLayer), nHicPerStave[mLayer] * nChipsPerHic[mLayer], -0.5, nHicPerStave[mLayer] * nChipsPerHic[mLayer] - 0.5, NStaves[mLayer], -0.5, NStaves[mLayer] - 0.5);
    mChipStaveOccupancy[mLayer]->SetStats(0);
    getObjectsManager()->startPublishing(mChipStaveOccupancy[mLayer]); //mChipStaveOccupancy

    mChipStaveEventHitCheck[mLayer] = new TH2I(Form("Occupancy/Layer%d/Layer%dChipStaveEventHit", mLayer, mLayer), Form("ITS Layer%d, Event Hit Check vs Chip and Stave", mLayer), nHicPerStave[mLayer] * nChipsPerHic[mLayer], -0.5, nHicPerStave[mLayer] * nChipsPerHic[mLayer] - 0.5, NStaves[mLayer], -0.5, NStaves[mLayer] - 0.5);
    mChipStaveEventHitCheck[mLayer]->SetStats(0);
    getObjectsManager()->startPublishing(mChipStaveEventHitCheck[mLayer]);

    mOccupancyPlot[mLayer] = new TH1D(Form("Occupancy/Layer%dOccupancy", mLayer), Form("ITS Layer %d Occupancy Distribution", mLayer), 300, -15, 0);
    getObjectsManager()->startPublishing(mOccupancyPlot[mLayer]); //mOccupancyPlot
  } else {
    //Create OB plots
    int nBinstmp[nDim] = { (nBins[0] * (nChipsPerHic[mLayer] / 2) * (nHicPerStave[mLayer] / 2) / ReduceFraction), (nBins[1] * 2 * NSubStave[mLayer] / ReduceFraction) };
    double Maxtmp[nDim] = { (double)(nBins[0] * (nChipsPerHic[mLayer] / 2) * (nHicPerStave[mLayer] / 2)), (double)(nBins[1] * 2 * NSubStave[mLayer]) };
    for (int istave = 0; istave < NStaves[mLayer]; istave++) {
      mStaveHitmap[mLayer][istave] = new THnSparseI(Form("Occupancy/Layer%d/Stave%d/Layer%dStave%dHITMAP", mLayer, istave, mLayer, istave), Form("Hits on Layer %d, Stave %d", mLayer, istave), nDim, nBinstmp, Min, Maxtmp);
      getObjectsManager()->startPublishing(mStaveHitmap[mLayer][istave]);
    }
    mChipStaveOccupancy[mLayer] = new TH2D(Form("Occupancy/Layer%d/Layer%dChipStave", mLayer, mLayer), Form("ITS Layer%d, Occupancy vs Chip and Stave", mLayer), nHicPerStave[mLayer], -0.5, nHicPerStave[mLayer] - 0.5, NStaves[mLayer], -0.5, NStaves[mLayer] - 0.5);
    mChipStaveOccupancy[mLayer]->SetStats(0);
    getObjectsManager()->startPublishing(mChipStaveOccupancy[mLayer]);

    mChipStaveEventHitCheck[mLayer] = new TH2I(Form("Occupancy/Layer%d/Layer%dChipStaveEventHit", mLayer, mLayer), Form("ITS Layer%d, Event Hit Check vs Chip and Stave", mLayer), nHicPerStave[mLayer], -0.5, nHicPerStave[mLayer] - 0.5, NStaves[mLayer], -0.5, NStaves[mLayer] - 0.5);
    mChipStaveEventHitCheck[mLayer]->SetStats(0);
    getObjectsManager()->startPublishing(mChipStaveEventHitCheck[mLayer]);

    mOccupancyPlot[mLayer] = new TH1D(Form("Occupancy/Layer%dOccupancy", mLayer), Form("ITS Layer %d Occupancy Distribution", mLayer), 300, -15, 0);
    getObjectsManager()->startPublishing(mOccupancyPlot[mLayer]); //mOccupancyPlot
  }
}

void ITSFhrTask::setAxisTitle(TH1* object, const char* xTitle, const char* yTitle)
{
  object->GetXaxis()->SetTitle(xTitle);
  object->GetYaxis()->SetTitle(yTitle);
}

void ITSFhrTask::setPlotsFormat()
{
  //set general plots format
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
    } else {
      if (mChipStaveOccupancy[ilayer]) {
        setAxisTitle(mChipStaveOccupancy[ilayer], "Hic Number", "Stave Number");
      }
      if (mChipStaveEventHitCheck[ilayer]) {
        setAxisTitle(mChipStaveEventHitCheck[ilayer], "Hic Number", "Stave Number");
      }
    }
  }
}

void ITSFhrTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity : " << activity.mId << ENDM;
  mRunNumber = activity.mId;
  reset();
}

void ITSFhrTask::startOfCycle() { ILOG(Info) << "startOfCycle" << ENDM; }

void ITSFhrTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  //set timer
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  std::chrono::time_point<std::chrono::high_resolution_clock> end;
  int difference;
  start = std::chrono::high_resolution_clock::now();
  //get TF id by dataorigin and datadescription
  const InputSpec TFIdFilter{ "", ConcreteDataTypeMatcher{ "DS", "RAWDATA1" }, Lifetime::Timeframe }; //after Data Sampling the dataorigin will become to "DS" and the datadescription will become  to "RAWDATAX"
  if (!mGetTFFromBinding) {
    for (auto& input : ctx.inputs()) {
      if (DataRefUtils::match(input, TFIdFilter)) {
        mTimeFrameId = (unsigned int)*input.payload;
      }
    }
  } else {
    mTimeFrameId = ctx.inputs().get<int>("G");
  }

  //set Decoder
  mDecoder->startNewTF(ctx.inputs());
  mDecoder->setDecodeNextAuto(true);
  std::vector<InputSpec> rawDataFilter{ InputSpec{ "", ConcreteDataTypeMatcher{ "DS", "RAWDATA0" }, Lifetime::Timeframe } };
  DPLRawParser parser(ctx.inputs(), rawDataFilter); //set input data

  //get data information from RDH(like witch layer, stave, link, trigger type)
  int lay = 0;
  for (auto it = parser.begin(), end = parser.end(); it != end; ++it) {
    auto const* rdh = it.get_if<o2::header::RAWDataHeaderV6>(); //Decoding new data format (RDHv6)
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
      istave += StaveBoundary[lay - NLayerIB];
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

  //update general information according trigger type
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

  //define digit hit vector
  std::vector<Digit>** digVec = new std::vector<Digit>*[NStaves[lay]];            //IB : digVec[stave][0]; OB : digVec[stave][hic]
  std::vector<ROFRecord>** digROFVec = new std::vector<ROFRecord>*[NStaves[lay]]; //IB : digROFVec[stave][0]; OB : digROFVec[stave][hic]
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

  //decode raw data and save digit hit to digit hit vector, and save hitnumber per chip/hic
  while ((mChipDataBuffer = mDecoder->getNextChipData(mChipsBuffer))) {
    if (mChipDataBuffer) {
      int layer, stave, ssta, mod, chip;
      int hic = 0;
      const auto& pixels = mChipDataBuffer->getData();
      for (auto& pixel : pixels) {
        if (lay < NLayerIB) {
          stave = mChipDataBuffer->getChipID() / 9 - StaveBoundary[lay];
          chip = mChipDataBuffer->getChipID() % 9;
          hic = 0;
          mHitnumber[stave][chip]++;
        } else {
          mGeom->getChipId(mChipDataBuffer->getChipID(), layer, stave, ssta, mod, chip);
          if (lay == 3 || lay == 4) {
            hic = mod + ssta * 4;
          } else {
            hic = mod + ssta * 7;
          }
          mHitnumber[stave][hic]++;
        }
        digVec[stave][hic].emplace_back(mChipDataBuffer->getChipID(), pixel.getRow(), pixel.getCol());
      }
      if (lay < NLayerIB) {
        if (pixels.size() > (unsigned int)mHitCutForCheck) {
          mChipStaveEventHitCheck[lay]->Fill(chip, stave);
        }
      } else {
        if (pixels.size() > (unsigned int)mHitCutForCheck) {
          mChipStaveEventHitCheck[lay]->Fill(mod + (ssta * (nHicPerStave[lay] / 2)), stave);
        }
      }
    }
  }

  //calculate active staves according digit hit vector
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
  //save digit hit vector to unordered_map by openMP multiple threads
  //the reason of this step is: it will spend many time If we THnSparse::Fill the THnspase hit by hit.
  //So we want save hit information to undordered_map and fill THnSparse by THnSparse::SetBinContent (pixel by pixel)
  for (int i = 0; i < (int)activeStaves.size(); i++) {
    int istave = activeStaves[i];
    if (lay < NLayerIB) {
      for (auto& digit : digVec[istave][0]) {
        mHitPixelID_InStave[istave][0][digit.getChipIndex() % 9][1000 * digit.getColumn() + digit.getRow()]++;
      }
    } else {
      for (int ihic = 0; ihic < nHicPerStave[lay]; ihic++) {
        for (auto& digit : digVec[istave][ihic]) {
          int lay, stave, ssta, mod, chip;
          mGeom->getChipId(digit.getChipIndex(), lay, stave, ssta, mod, chip);
          mHitPixelID_InStave[istave][ihic][chip][1000 * digit.getColumn() + digit.getRow()]++;
        }
      }
    }
  }

  end = std::chrono::high_resolution_clock::now();
  difference = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  ILOG(Debug) << "time untile decode over " << difference << ENDM;

  //Reset Error plots
  mErrorPlots->Reset();
  mErrorVsFeeid->Reset(); //Error is   statistic by decoder so if we didn't reset decoder, then we need reset Error plots, and use TH::SetBinContent function
  //mTriggerVsFeeid->Reset();			  Trigger is statistic by ourself so we don't need reset this plot, just use TH::Fill function
  mOccupancyPlot[lay]->Reset();

  //define tmp occupancy plot, which will use for multiple threads
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
  //fill Monitor Objects use openMP multiple threads, and calculate the occupancy
  for (int i = 0; i < (int)activeStaves.size(); i++) {
    int istave = activeStaves[i];
    if (digVec[istave][0].size() < 1) {
      continue;
    }
    const auto* DecoderTmp = mDecoder;
    int RUid = StaveBoundary[lay] + istave;
    const o2::itsmft::RUDecodeData* RUdecode = DecoderTmp->getRUDecode(RUid);
    if (!RUdecode) {
      continue;
    }

    if (lay < NLayerIB) {
      for (int ilink = 0; ilink < RUDecodeData::MaxLinksPerRU; ilink++) {
        const auto* GBTLinkInfo = DecoderTmp->getGBTLink(RUdecode->links[ilink]);
        if (!GBTLinkInfo) {
          continue;
        }
        for (int ichip = 0 + (ilink * 3); ichip < (ilink * 3) + 3; ichip++) {
          std::unordered_map<unsigned int, int>::iterator iter;
          for (iter = mHitPixelID_InStave[istave][0][ichip].begin(); iter != mHitPixelID_InStave[istave][0][ichip].end(); iter++) {
            int pixelPos[2] = { (int)(iter->first / 1000) + (1024 * ichip), (int)(iter->first % 1000) };
            mStaveHitmap[lay][istave]->SetBinContent(pixelPos, (double)iter->second);
            totalhit += (int)iter->second;
            occupancyPlotTmp[i]->Fill(log10((double)iter->second / GBTLinkInfo->statistics.nTriggers));
          }
          mOccupancy[istave][ichip] = mHitnumber[istave][ichip] / (GBTLinkInfo->statistics.nTriggers * 1024. * 512.);
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
              for (iter = mHitPixelID_InStave[istave][ihic][ichip].begin(); iter != mHitPixelID_InStave[istave][ihic][ichip].end(); iter++) {
                double pixelOccupancy = (double)iter->second;
                occupancyPlotTmp[i]->Fill(log10(pixelOccupancy / GBTLinkInfo->statistics.nTriggers));
                if (ichip < 7) {
                  int pixelPos[2] = { (ihic * ((nChipsPerHic[lay] / 2) * NCols)) + ichip * NCols + (int)(iter->first / 1000) + 1, NRows - ((int)iter->first % 1000) - 1 + (1024 * ilink) + 1 };
                  mStaveHitmap[lay][istave]->SetBinContent(pixelPos, pixelOccupancy);
                } else {
                  int pixelPos[2] = { (ihic * ((nChipsPerHic[lay] / 2) * NCols)) + (nChipsPerHic[lay] / 2) * NCols - (ichip - 7) * NCols - ((int)iter->first / 1000) + 1, NRows + ((int)iter->first % 1000) + (1024 * ilink) + 1 };
                  mStaveHitmap[lay][istave]->SetBinContent(pixelPos, pixelOccupancy);
                }
              }
            }
          }
          if (lay == 3 || lay == 4) {
            mOccupancy[istave][ihic + (ilink * 4)] = mHitnumber[istave][ihic + (ilink * 4)] / (GBTLinkInfo->statistics.nTriggers * 1024. * 512. * nChipsPerHic[lay]);
          } else {
            mOccupancy[istave][ihic + (ilink * 7)] = mHitnumber[istave][ihic + (ilink * 7)] / (GBTLinkInfo->statistics.nTriggers * 1024. * 512. * nChipsPerHic[lay]);
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

  //fill Occupancy plots, chip stave occupancy plots and error statistic plots
  for (int i = 0; i < (int)activeStaves.size(); i++) {
    int istave = activeStaves[i];
    mOccupancyPlot[lay]->Add(occupancyPlotTmp[i]);
    if (lay < NLayerIB) {
      for (int ichip = 0; ichip < nChipsPerHic[lay]; ichip++) {
        mChipStaveOccupancy[lay]->SetBinContent(ichip + 1, istave + 1, mOccupancy[istave][ichip]);
        int ilink = ichip / 3;
        for (int ierror = 0; ierror < o2::itsmft::GBTLinkDecodingStat::NErrorsDefined; ierror++) {
          if (mErrorVsFeeid && (mErrorCount[istave][ilink][ierror] > 0)) {
            mErrorVsFeeid->SetBinContent((istave * 3) + ilink + 1, ierror + 1, mErrorCount[istave][ilink][ierror]);
          }
        }
      }
      mGeneralOccupancy->SetBinContent(istave + 1, *(std::max_element(mOccupancy[istave], mOccupancy[istave] + nChipsPerHic[lay])));
    } else {
      for (int ihic = 0; ihic < nHicPerStave[lay]; ihic++) {
        int ilink = ihic / (nHicPerStave[lay] / 2);
        mChipStaveOccupancy[lay]->SetBinContent(ihic + 1, istave + 1, mOccupancy[istave][ihic]);
        if (ihic == 0 || ihic == 7) {
          for (int ierror = 0; ierror < o2::itsmft::GBTLinkDecodingStat::NErrorsDefined; ierror++) {
            if (mErrorVsFeeid && (mErrorCount[istave][ilink][ierror] != 0)) {
              mErrorVsFeeid->SetBinContent((3 * StaveBoundary[3]) + (istave * 2) + ilink + 1, ierror + 1, mErrorCount[istave][ilink][ierror]);
            }
          }
        }
      }
    }
    mGeneralOccupancy->SetBinContent(istave + 1, *(std::max_element(mOccupancy[istave], mOccupancy[istave] + nChipsPerHic[lay])));
  }
  for (int ierror = 0; ierror < o2::itsmft::GBTLinkDecodingStat::NErrorsDefined; ierror++) {
    int feeError = mErrorVsFeeid->Integral(1, mErrorVsFeeid->GetXaxis()->GetNbins(), ierror + 1, ierror + 1);
    mErrorPlots->SetBinContent(ierror + 1, feeError);
  }

  //delete pointor in monitorData()
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
  //temporarily reverting to get TFId by querying binding
  //  mTimeFrameId = ctx.inputs().get<int>("G");
  //Timer LOG
  mTFInfo->Fill(mTimeFrameId);
  end = std::chrono::high_resolution_clock::now();
  difference = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  mAverageProcessTime += difference;
  ILOG(Debug) << "average process time == " << (double)mAverageProcessTime / mTimeFrameId << ENDM;
  ILOG(Debug) << "time until thread all end is " << difference << ", and TF ID == " << mTimeFrameId << ENDM;
}

void ITSFhrTask::getParameters()
{
  mNThreads = std::stoi(mCustomParameters["decoderThreads"]);
  mLayer = std::stoi(mCustomParameters["Layer"]);
  mHitCutForCheck = std::stoi(mCustomParameters["HitNumberCut"]);
  mGetTFFromBinding = std::stoi(mCustomParameters["GetTFFromBinding"]);
  mRunNumberPath = mCustomParameters["runNumberPath"];
  mGeomPath = mCustomParameters["geomPath"];
}

void ITSFhrTask::endOfCycle()
{
  std::ifstream runNumberFile("infiles/RunNumber.dat"); //catching ITS run number in commissioning
  if (mRunNumber != "000000") {
    ILOG(Info) << "runNumber : " << mRunNumber << ENDM;
    mInfoCanvasComm->SetTitle(Form("run%s", mRunNumber.c_str()));
    getObjectsManager()->addMetadata(mTFInfo->GetName(), "Run", mRunNumber);
    getObjectsManager()->addMetadata(mErrorPlots->GetName(), "Run", mRunNumber);
    getObjectsManager()->addMetadata(mErrorVsFeeid->GetName(), "Run", mRunNumber);
    getObjectsManager()->addMetadata(mTriggerVsFeeid->GetName(), "Run", mRunNumber);
    getObjectsManager()->addMetadata(mTriggerPlots->GetName(), "Run", mRunNumber);
    getObjectsManager()->addMetadata(mInfoCanvasComm->GetName(), "Run", mRunNumber);
    getObjectsManager()->addMetadata(mChipStaveOccupancy[mLayer]->GetName(), "Run", mRunNumber);
    getObjectsManager()->addMetadata(mOccupancyPlot[mLayer]->GetName(), "Run", mRunNumber);
    getObjectsManager()->addMetadata(mChipStaveEventHitCheck[mLayer]->GetName(), "Run", mRunNumber);
    for (int istave = 0; istave < NStaves[mLayer]; istave++) {
      if (mStaveHitmap[mLayer][istave]) {
        getObjectsManager()->addMetadata(mStaveHitmap[mLayer][istave]->GetName(), "Run", mRunNumber);
      }
    }
  }
  ILOG(Info) << "endOfCycle" << ENDM;
}

void ITSFhrTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "endOfActivity" << ENDM;
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

  if (mLayer < NLayerIB) {
    for (int istave = 0; istave < NStaves[mLayer]; istave++) {
      for (int ilink = 0; ilink < 3; ilink++) {
        for (int ierror = 0; ierror < o2::itsmft::GBTLinkDecodingStat::NErrorsDefined; ierror++) {
          mErrorCount[istave][ilink][ierror] = 0;
        }
      }
      for (int ichip = 0; ichip < nChipsPerHic[mLayer]; ichip++) {
        mHitnumber[istave][ichip] = 0;
        mOccupancy[istave][ichip] = 0;
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
        mHitnumber[istave][ihic] = 0;
        mOccupancy[istave][ihic] = 0;
        for (int ichip = 0; ichip < nChipsPerHic[mLayer]; ichip++) {
          mHitPixelID_InStave[istave][ihic][ichip].clear();
        }
      }
    }
  }

  ILOG(Info) << "Reset" << ENDM;
}

void ITSFhrTask::getStavePoint(int layer, int stave, double* px, double* py)
{
  float stepAngle = TMath::Pi() * 2 / NStaves[layer];             //the angle between to stave
  float midAngle = StartAngle[layer] + (stave * stepAngle);       //mid point angle
  float staveRotateAngle = TMath::Pi() / 2 - (stave * stepAngle); //how many angle this stave rotate(compare with first stave)
  px[1] = MidPointRad[layer] * TMath::Cos(midAngle);              //there are 4 point to decide this TH2Poly bin
                                                                  //0:left point in this stave;
                                                                  //1:mid point in this stave;
                                                                  //2:right point in this stave;
                                                                  //3:higher point int this stave;
  py[1] = MidPointRad[layer] * TMath::Sin(midAngle);              //4 point calculated accord the blueprint
                                                                  //roughly calculate
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
