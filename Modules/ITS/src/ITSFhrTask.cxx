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
#include "QualityControl/QcInfoLogger.h"

#include <DPLUtils/RawParser.h>
#include <DPLUtils/DPLRawParser.h>

using namespace o2::framework;
using namespace o2::itsmft;
using namespace o2::header;

namespace o2::quality_control_modules::its
{

ITSFhrTask::ITSFhrTask()
  : TaskInterface()
{
  o2::base::GeometryManager::loadGeometry();
  mGeom = o2::its::GeometryTGeo::Instance();
}

ITSFhrTask::~ITSFhrTask()
{
  delete mDecoder;
  delete mChipDataBuffer;
  delete mTFInfo;
  delete mErrorPlots;
  delete mErrorVsFeeid;
  delete mTriggerPlots;
  delete mTriggerVsFeeid;
  //delete mInfoCanvas;
  delete mInfoCanvasComm;
  delete mTextForShifter;
  delete mTextForShifter2;

  for (int ilayer = 0; ilayer < 7; ilayer++) {
    delete mChipStaveOccupancy[ilayer];
    delete mOccupancyPlot[ilayer];
    delete mChipStaveEventHitCheck[ilayer];
    for (int istave = 0; istave < 48; istave++) {
      for (int isubstave = 0; isubstave < 2; isubstave++) {
        for (int ihic = 0; ihic < 14; ihic++) {
          delete mHicHitmapAddress[ilayer][istave][isubstave][ihic];
          delete mHicHitmap[ilayer][istave][isubstave][ihic];
        }
      }
    }
  }
  delete mGeom;
}

void ITSFhrTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  QcInfoLogger::GetInstance() << "initialize ITSFhrTask" << AliceO2::InfoLogger::InfoLogger::endm;
  getEnableLayers();
  int barrel = 0;
  if (mEnableLayers[0] or mEnableLayers[1] or mEnableLayers[2]) {
    barrel += 1;
  }
  if (mEnableLayers[3] or mEnableLayers[4] or mEnableLayers[5] or mEnableLayers[6]) {
    barrel += 2;
  }
  createGeneralPlots(barrel);
  createOccupancyPlots();
  setPlotsFormat();
  mDecoder = new o2::itsmft::RawPixelDecoder<o2::itsmft::ChipMappingITS>();
  mDecoder->init();
  mDecoder->setNThreads(mNThreads);
  mDecoder->setFormat(GBTLink::NewFormat);               //Using RDHv6 (NewFormat)
  mDecoder->setUserDataOrigin(header::DataOrigin("DS")); //set user data origin in dpl
  mDecoder->setUserDataDescription(header::DataDescription("RAWDATA0"));
  mChipsBuffer.resize(mGeom->getNumberOfChips());
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

void ITSFhrTask::createGeneralPlots(int barrel = 0)
{

  mInfoCanvasComm = new TH2I("General/InfoCanvas", "InfoCanvas", 3, -0.5, 2.5, 4, -0.5, 3.5);
  for (int i = 0; i < 3; i++) {
    mInfoCanvasComm->GetXaxis()->SetBinLabel(i + 1, Form("layer%d", i));
  }
  mInfoCanvasComm->GetZaxis()->SetRangeUser(0, 2);
  mInfoCanvasComm->GetYaxis()->SetBinLabel(2, "ibt");
  mInfoCanvasComm->GetYaxis()->SetBinLabel(4, "ibb");
  mTextForShifter = new TText(.5, 1.5, "DarkGreen -> Processing");
  mTextForShifter2 = new TText(.5, 1.2, "Yellow    -> Finished");
  mInfoCanvasComm->SetStats(0);
  mInfoCanvasComm->GetListOfFunctions()->Add(mTextForShifter);
  mInfoCanvasComm->GetListOfFunctions()->Add(mTextForShifter2);
  getObjectsManager()->startPublishing(mInfoCanvasComm);

  createErrorTriggerPlots();

  mTFInfo = new TH1F("General/TFInfo", "TF vs count", 15000, 0, 15000);
  getObjectsManager()->startPublishing(mTFInfo); //mTFInfo

  if (barrel == 0) { //No Need to create
    QcInfoLogger::GetInstance() << "No General Plots need to create, Please check you config file of QC" << AliceO2::InfoLogger::InfoLogger::endm;
  } else if (barrel == 1) { //create for IB
    mErrorVsFeeid = new TH2I("General/ErrorVsFeeid", "Error count vs Error id and Fee id", 3 * StaveBoundary[3], 0, 3 * StaveBoundary[3], mNError, 0.5, mNError + 0.5);
    mErrorVsFeeid->SetMinimum(0);
    mErrorVsFeeid->SetStats(0);
    getObjectsManager()->startPublishing(mErrorVsFeeid);

    mTriggerVsFeeid = new TH2I("General/TriggerVsFeeid", "Trigger count vs Trigger id and Fee id", 3 * StaveBoundary[3], 0, 3 * StaveBoundary[3], mNTrigger, 0.5, mNTrigger + 0.5);
    mTriggerVsFeeid->SetMinimum(0);
    mTriggerVsFeeid->SetStats(0);
    getObjectsManager()->startPublishing(mTriggerVsFeeid);
  } else if (barrel == 2) { //create for OB		TODO:create the Error/Trigger vs feeid plots
    mErrorVsFeeid = new TH2I("General/ErrorVsFeeid", "Error count vs Error id and Fee id", 3 * StaveBoundary[3], 0, 3 * StaveBoundary[3], mNError, 0.5, mNError + 0.5);
    mErrorVsFeeid->SetMinimum(0);
    mErrorVsFeeid->SetStats(0);
    getObjectsManager()->startPublishing(mErrorVsFeeid);

    mTriggerVsFeeid = new TH2I("General/TriggerVsFeeid", "Trigger count vs Trigger id and Fee id", 3 * StaveBoundary[3], 0, 3 * StaveBoundary[3], mNTrigger, 0.5, mNTrigger + 0.5);
    mTriggerVsFeeid->SetMinimum(0);
    mTriggerVsFeeid->SetStats(0); //create for OB		TODO: now I am just copy the code from IB to make it will not crash
    getObjectsManager()->startPublishing(mTriggerVsFeeid);

  } else if (barrel == 3) { //create for All	TODO:create the Error/Trigger vs feeid plots
  }
}

void ITSFhrTask::createOccupancyPlots() //create general plots like error, trigger, TF id plots and so on....
                                        //create occupancy plots like chip stave occupancy, occupancy distribution, hic hit map plots and so on....
{
  //create occupancy plots
  const int nDim(2);
  int nBins[nDim] = { 1024, 512 };
  double Min[nDim] = { 0, 0 };
  double Max[nDim] = { 1024, 512 };
  //char* nDimTitle[nDim] = {"Column", "Row"};

  //create IB plots
  for (int ilayer = 0; ilayer < NLayerIB; ilayer++) {
    if (!mEnableLayers[ilayer]) {
      continue;
    }
    int nBinstmp[nDim] = { nBins[0] * nChipsPerHic[ilayer] / ReduceFraction, nBins[1] / ReduceFraction };
    double Maxtmp[nDim] = { Max[0] * nChipsPerHic[ilayer], Max[1] };
    for (int istave = 0; istave < NStaves[ilayer]; istave++) {
      for (int isubstave = 0; isubstave < NSubStave[ilayer]; isubstave++) {
        for (int ihic = 0; ihic < nHicPerStave[ilayer]; ihic++) {
          mHicHitmap[ilayer][istave][isubstave][ihic] = new THnSparseI(Form("Occupancy/Layer%d/Stave%d/Layer%dStave%dHITMAP", ilayer, istave, ilayer, istave), Form("Hits on Layer %d, Stave %d", ilayer, istave), nDim, nBinstmp, Min, Maxtmp);
          getObjectsManager()->startPublishing(mHicHitmap[ilayer][istave][isubstave][ihic]); //mHicHItmapAddress
        }
      }
    }

    mChipStaveOccupancy[ilayer] = new TH2D(Form("Occupancy/Layer%d/Layer%dChipStave", ilayer, ilayer), Form("ITS Layer%d, Occupancy vs Chip and Stave", ilayer), nHicPerStave[ilayer] * nChipsPerHic[ilayer], -0.5, nHicPerStave[ilayer] * nChipsPerHic[ilayer] - 0.5, NStaves[ilayer], -0.5, NStaves[ilayer] - 0.5);
    mChipStaveOccupancy[ilayer]->SetStats(0);
    getObjectsManager()->startPublishing(mChipStaveOccupancy[ilayer]); //mChipStaveOccupancy

    mChipStaveEventHitCheck[ilayer] = new TH2I(Form("Occupancy/Layer%d/Layer%dChipStaveEventHit", ilayer, ilayer), Form("ITS Layer%d, Event Hit Check vs Chip and Stave", ilayer), nHicPerStave[ilayer] * nChipsPerHic[ilayer], -0.5, nHicPerStave[ilayer] * nChipsPerHic[ilayer] - 0.5, NStaves[ilayer], -0.5, NStaves[ilayer] - 0.5);
    mChipStaveEventHitCheck[ilayer]->SetStats(0);
    getObjectsManager()->startPublishing(mChipStaveEventHitCheck[ilayer]);

    mOccupancyPlot[ilayer] = new TH1D(Form("Occupancy/Layer%dOccupancy", ilayer), Form("ITS Layer %d Occupancy Distribution", ilayer), 300, -15, 0);
    getObjectsManager()->startPublishing(mOccupancyPlot[ilayer]); //mOccupancyPlot
  }

  //Create OB plots
  for (int ilayer = NLayerIB; ilayer < NLayer; ilayer++) {
    if (!mEnableLayers[ilayer]) {
      continue;
    }
    for (int istave = 0; istave < NStaves[ilayer]; istave++) {
      for (int isubstave = 0; isubstave < NSubStave[ilayer]; isubstave++) {
        for (int ihic = 0; ihic < (nHicPerStave[ilayer] / NSubStave[ilayer]); ihic++) {
          mHicHitmapAddress[ilayer][istave][isubstave][ihic] = new TH2I(Form("Occupancy/Layer%d/Stave%d/HIC%d/Layer%dStave%dHIC%dHITMAP", ilayer, istave, ihic + isubstave * (nHicPerStave[ilayer] / NSubStave[ilayer]), ilayer, istave, ihic + +isubstave * (nHicPerStave[ilayer] / NSubStave[ilayer])), Form("Hits on Layer %d, Stave %d, Hic %d", ilayer, istave, ihic + +isubstave * (nHicPerStave[ilayer] / NSubStave[ilayer])), (1024 / (ReduceFraction * 2)) * (nChipsPerHic[ilayer] / 2), 0, 1024 * (nChipsPerHic[ilayer] / 2), (512 / (ReduceFraction * 2)) * 2, 0, 512 * 2);
          mHicHitmapAddress[ilayer][istave][isubstave][ihic]->SetStats(0);
          getObjectsManager()->startPublishing(mHicHitmapAddress[ilayer][istave][isubstave][ihic]); //mHicHItmapAddress
        }
      }
    }

    mChipStaveOccupancy[ilayer] = new TH2D(Form("Occupancy/Layer%d/Layer%dChipStave", ilayer, ilayer), Form("ITS Layer%d, Occupancy vs Chip and Stave", ilayer), nHicPerStave[ilayer], -0.5, nHicPerStave[ilayer] - 0.5, NStaves[ilayer], -0.5, NStaves[ilayer] - 0.5);
    mChipStaveOccupancy[ilayer]->SetStats(0);
    getObjectsManager()->startPublishing(mChipStaveOccupancy[ilayer]);

    mChipStaveEventHitCheck[ilayer] = new TH2I(Form("Occupancy/Layer%d/Layer%dChipStaveEventHit", ilayer, ilayer), Form("ITS Layer%d, Event Hit Check vs Chip and Stave", ilayer), nHicPerStave[ilayer], -0.5, nHicPerStave[ilayer] - 0.5, NStaves[ilayer], -0.5, NStaves[ilayer] - 0.5);
    mChipStaveEventHitCheck[ilayer]->SetStats(0);
    getObjectsManager()->startPublishing(mChipStaveEventHitCheck[ilayer]);

    mOccupancyPlot[ilayer] = new TH1D(Form("Occupancy/Layer%dOccupancy", ilayer), Form("ITS Layer %d Occupancy Distribution", ilayer), 300, -15, 0);
    getObjectsManager()->startPublishing(mOccupancyPlot[ilayer]); //mOccupancyPlot
  }
  //create occupancy plots end
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
      mOccupancyPlot[ilayer]->GetXaxis()->SetTitle("log(Occupancy/PixelNumber)");
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

void ITSFhrTask::startOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void ITSFhrTask::startOfCycle() { QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm; }

void ITSFhrTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // in a loop
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  std::chrono::time_point<std::chrono::high_resolution_clock> end;
  int difference;
  start = std::chrono::high_resolution_clock::now();

  int lay, sta, ssta, mod, chip; //layer, stave, sub stave, module, chip

  mDecoder->startNewTF(ctx.inputs());
  mDecoder->setDecodeNextAuto(true);

  //decode the raw data and Fill hit map
  while ((mChipDataBuffer = mDecoder->getNextChipData(mChipsBuffer))) {
    if (mChipDataBuffer) {
      const auto& pixels = mChipDataBuffer->getData();
      int chipHitNumber[2][7][14] = { { { 0 } } }; //substave, module
      for (auto& pixel : pixels) {
        mGeom->getChipId(mChipDataBuffer->getChipID(), lay, sta, ssta, mod, chip);
        mHitNumberOfChip[lay][sta][ssta][mod][chip]++;
        chipHitNumber[ssta][mod][chip]++;
        if (lay < NLayerIB) {
          double filltmp[2] = { (double)pixel.getCol() + (double)chip * 1024, (double)pixel.getRow() };
          mHicHitmap[lay][sta][ssta][mod]->Fill(filltmp, 1);
        } else {
          if (chip < 7) {
            mHicHitmapAddress[lay][sta][ssta][mod]->Fill((int)(pixel.getCol() + chip * 1024), (int)pixel.getRow());
          } else {
            mHicHitmapAddress[lay][sta][ssta][mod]->Fill((int)(pixel.getCol() + ((chip - 7) * 1024)), (int)(pixel.getRow() + 512.));
          }
        }

        bool flag = true;
        for (int jpos = 0; jpos < (int)mHitPixelID[lay][sta][mod][chip].size(); jpos++) {
          if ((pixel.getCol() == mHitPixelID[lay][sta][mod][chip][jpos].first) and (pixel.getRow() == mHitPixelID[lay][sta][mod][chip][jpos].second)) {
            mPixelHitNumber[lay][sta][mod][chip][jpos]++;
            flag = false;
            break;
          }
        }
        if (flag) {
          mHitPixelID[lay][sta][mod][chip].push_back(std::make_pair(pixel.getCol(), pixel.getRow()));
          mPixelHitNumber[lay][sta][mod][chip].push_back(1);
        }
      }
      if (lay < NLayerIB) {
        if (pixels.size() > 100) {
          mChipStaveEventHitCheck[lay]->Fill(chip, sta);
        }
      } else {
        if (pixels.size() > 0) {
          mChipStaveEventHitCheck[lay]->Fill(mod + (ssta * (nHicPerStave[lay] / 2)), sta);
        }
      }
    }
  }
  //decode raw data and fill hit map end

  //Reset Error plots
  mErrorPlots->Reset();
  mErrorVsFeeid->Reset(); //Error is   statistic by decoder so if we didn't reset decoder, then we need reset Error plots, and use TH::SetBinContent function
  //mTriggerVsFeeid->Reset();			  Trigger is statistic by ourself so we don't need reset this plot, just use TH::Fill function
  //Reset Error plots end

  //Fill Error plots and occpancy plots
  int istavemax = 0;
  int istavemin = 999;
  o2::itsmft::ChipMappingITS mp;
  for (int ilayer = 0; ilayer < NLayer; ilayer++) {
    if (mOccupancyPlot[ilayer]) {
      mOccupancyPlot[ilayer]->Reset();
    }
    for (int istave = 0; istave < NStaves[ilayer]; istave++) {
      int RUid = istave;
      for (int jlayer = 0; jlayer < ilayer; jlayer++) {
        RUid += NStaves[jlayer];
      }
      const o2::itsmft::RUDecodeData* RUdecode;
      const auto* mDecoderTmp = mDecoder;
      RUdecode = mDecoderTmp->getRUDecode(RUid);
      if (!RUdecode) {
        continue;
      }
      if (istave > istavemax) {
        istavemax = istave;
      }
      if (istave < istavemin) {
        istavemin = istave;
      }
      for (int ilink = 0; ilink < RUDecodeData::MaxLinksPerRU; ilink++) {
        const auto* GBTLinkInfo = mDecoderTmp->getGBTLink(RUdecode->links[ilink]);
        if (!GBTLinkInfo) {
          continue;
        }
        if (ilayer < NLayerIB) {
          for (int ichip = 0; ichip < nChipsPerHic[ilayer]; ichip++) {
            if ((GBTLinkInfo->statistics.nPackets > 0) and (mHitNumberOfChip[ilayer][istave][0][0][ichip] >= 0)) {
              mChipStaveOccupancy[ilayer]->SetBinContent(ichip + 1, istave + 1, (mHitNumberOfChip[ilayer][istave][0][0][ichip]) / (GBTLinkInfo->statistics.nPackets * 1024. * 512.));
              for (unsigned int ipixel = 0; ipixel < mHitPixelID[ilayer][istave][0][ichip].size(); ipixel++) {
                double pixelOccupancy = (double)mPixelHitNumber[ilayer][istave][0][ichip][ipixel];
                if (pixelOccupancy > 0) {
                  pixelOccupancy /= GBTLinkInfo->statistics.nPackets;
                  mOccupancyPlot[ilayer]->Fill(log10(pixelOccupancy));
                }
              }
            }
          }
        } else {
          int isubstave = ilink;
          for (int ihic = 0; ihic < ((nHicPerStave[ilayer] / NSubStave[ilayer])); ihic++) {
            double chipOccupancy = 0;
            for (int ichip = 0; ichip < nChipsPerHic[ilayer]; ichip++) {
              chipOccupancy += mHitNumberOfChip[ilayer][istave][isubstave][ihic][ichip];
            }
            chipOccupancy = chipOccupancy / (GBTLinkInfo->statistics.nPackets * 1024. * 512.);
            if (chipOccupancy == 0) {
              continue;
            }
            mChipStaveOccupancy[ilayer]->SetBinContent(ihic + (isubstave * (nHicPerStave[ilayer] / NSubStave[ilayer])) + 1, istave + 1, chipOccupancy / (double)nChipsPerHic[ilayer]);
          }
        }
        for (int ierror = 0; ierror < o2::itsmft::GBTLinkDecodingStat::NErrorsDefined; ierror++) {
          mErrorPlots->AddBinContent(ierror + 1, GBTLinkInfo->statistics.errorCounts[ierror]);
          mErrorVsFeeid->SetBinContent((RUid * 3) + ilink + 1, ierror + 1, GBTLinkInfo->statistics.errorCounts[ierror]);
        }
      }
    }
  }
  //Fill Error plots and occupancy plots end

  //Fill the Trigger plots
  DPLRawParser parser(ctx.inputs());
  int StaveStart = 0;
  for (int i = 0; !mEnableLayers[i]; i++) {
    StaveStart = StaveBoundary[i + 1];
  }
  for (auto it = parser.begin(), end = parser.end(); it != end; ++it) {
    auto const* rdh = it.get_if<o2::header::RAWDataHeaderV6>(); //Decoding new data format (RDHv6)
    int istave = (int)(rdh->feeId & 0x00ff);
    int ilink = (int)((rdh->feeId & 0x0f00) >> 8);
    istave += StaveStart;
    for (int i = 0; i < 13; i++) {
      if (((uint32_t)(rdh->triggerType) >> i & 1) == 1) {
        mTriggerPlots->Fill(i + 1);
        mTriggerVsFeeid->Fill((istave * 3) + ilink, i + 1);
        if ((i + 1) == 11) {
          lay = (int)(rdh->feeId >> 12);
        }
      }
    }
  }
  //Fill the Trigger plots end

  //Calculate the part id dependent the lay(if there are multiple layer enabled it will confuse)	TODO: maybe we need to make it work?
  int part = -1;                                            //which part in this layer. For layer 0, 1: 0-1 -> ibt; 2-3 -> ibb
  if (lay == 2) {                                           //                              layer 2: 0 -> ibt-a; 1 -> ibt-b
    part = (int)(4 * (((double)istavemax) / NStaves[lay])); //                              layer 2: 2 -> ibb-a; 3 -> ibb-b
  }                                                         //                              default: -1
  if (lay == 0) {
    if (istavemax == 11) {
      part = 2;
    } else if (istavemax == 5) {
      part = 0;
    }
  } else if (lay == 1) {
    if (istavemax == 15) {
      part = 2;
    } else if (istavemax == 7) {
      part = 0;
    }
  }
  //calculate the part id end

  //check the start and end point of this run
  if (mTriggerPlots->GetBinContent(10)) {
    mInfoCanvasComm->SetBinContent(lay + 1, part + 1, 1); //Now it is just work for IB, TODO: enable OB
    if (lay < 2) {
      mInfoCanvasComm->SetBinContent(lay + 1, part + 2, 1);
    }
  }

  if (mTriggerPlots->GetBinContent(11)) {
    mInfoCanvasComm->SetBinContent(lay + 1, part + 1, 2); //Now it is just work for IB, TODO: enable OB
    if (lay < 2) {
      mInfoCanvasComm->SetBinContent(lay + 1, part + 2, 2);
    }
  }
  //check the start and end point end

  //get the Time Frame ID
  mTimeFrameId = ctx.inputs().get<int>("G");
  //get the Time Frame ID end

  mTFInfo->Fill(mTimeFrameId);
  end = std::chrono::high_resolution_clock::now();
  difference = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  ILOG(Info) << "time until thread all end is " << difference << ", and TF ID == " << mTimeFrameId << ENDM;
}

void ITSFhrTask::getEnableLayers()
{
  std::ifstream configFile("Config/ConfigFakeRateOnline.dat");
  configFile >> mNThreads;
  for (int ilayer = 0; ilayer < NLayer; ilayer++) {
    configFile >> mEnableLayers[ilayer];
    if (mEnableLayers[ilayer]) {
      ILOG(Info) << "enable layer : " << ilayer << ENDM;
    }
  }
  configFile >> mRunNumberPath;
}

void ITSFhrTask::endOfCycle()
{
  std::ifstream runNumberFile("/home/its/QC_Online/workdir/infiles/RunNumber.dat"); //catching ITS run number in commissioning
  if (runNumberFile) {
    std::string runNumber;
    runNumberFile >> runNumber;
    ILOG(Info) << "runNumber : " << runNumber << ENDM;
    mInfoCanvasComm->SetTitle(Form("run%s", runNumber.c_str()));
    if (runNumber == mRunNumber) {
      goto pass;
    }
    getObjectsManager()->addMetadata(mTFInfo->GetName(), "Run", runNumber);
    getObjectsManager()->addMetadata(mErrorPlots->GetName(), "Run", runNumber);
    getObjectsManager()->addMetadata(mErrorVsFeeid->GetName(), "Run", runNumber);
    getObjectsManager()->addMetadata(mTriggerVsFeeid->GetName(), "Run", runNumber);
    getObjectsManager()->addMetadata(mTriggerPlots->GetName(), "Run", runNumber);
    getObjectsManager()->addMetadata(mInfoCanvasComm->GetName(), "Run", runNumber);
    for (int ilayer = 0; ilayer < NLayer; ilayer++) {
      if (!mEnableLayers[ilayer]) {
        continue;
      }
      getObjectsManager()->addMetadata(mChipStaveOccupancy[ilayer]->GetName(), "Run", runNumber);
      getObjectsManager()->addMetadata(mOccupancyPlot[ilayer]->GetName(), "Run", runNumber);
      getObjectsManager()->addMetadata(mChipStaveEventHitCheck[ilayer]->GetName(), "Run", runNumber);
      for (int istave = 0; istave < NStaves[ilayer]; istave++) {
        for (int isubstave = 0; isubstave < NSubStave[ilayer]; isubstave++) {
          for (int ihic = 0; ihic < nHicPerStave[ilayer]; ihic++) {
            if (mHicHitmap[ilayer][istave][isubstave][ihic]) {
              getObjectsManager()->addMetadata(mHicHitmap[ilayer][istave][isubstave][ihic]->GetName(), "Run", runNumber);
            }
          }
        }
      }
    }
    mRunNumber = runNumber;
  pass:;
  }
  ILOG(Info) << "endOfCycle" << ENDM;
}

void ITSFhrTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "endOfActivity" << ENDM;
}

void ITSFhrTask::resetGeneralPlots()
{
  mTFInfo->Reset();
  mErrorPlots->Reset();
  mErrorVsFeeid->Reset();
  mTriggerVsFeeid->Reset();
  mTriggerPlots->Reset();
  mInfoCanvasComm->Reset();
}

void ITSFhrTask::resetOccupancyPlots()
{
  //  mHitNumberOfChip = { 0 };
  memset(mHitNumberOfChip, 0, sizeof(mHitNumberOfChip));
  memset(mTriggerTypeCount, 0, sizeof(mTriggerTypeCount));
  memset(mErrors, 0, sizeof(mErrors));
  mTimeFrameId = 0;
  for (int ilayer = 0; ilayer < NLayer; ilayer++) {
    if (!mEnableLayers[ilayer]) {
      continue;
    }
    mChipStaveOccupancy[ilayer]->Reset();
    mChipStaveEventHitCheck[ilayer]->Reset();
    mOccupancyPlot[ilayer]->Reset();
    for (int istave = 0; istave < NStaves[ilayer]; istave++) {
      for (int isubstave = 0; isubstave < NSubStave[ilayer]; isubstave++) {
        for (int ihic = 0; ihic < nHicPerStave[ilayer]; ihic++) {
          mHicHitmap[ilayer][istave][isubstave][ihic]->Reset();
        }
      }
    }
  }
}

void ITSFhrTask::reset()
{
  resetGeneralPlots();
  resetOccupancyPlots();
  ILOG(Info) << "Reset" << ENDM;
}

} // namespace o2::quality_control_modules::its
