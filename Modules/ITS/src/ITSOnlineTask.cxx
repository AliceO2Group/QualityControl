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
/// \file   ITSOnlineTask.cxx
/// \author Liang Zhang
/// \author Jian Liu
///

#include "ITS/ITSOnlineTask.h"
#include "QualityControl/QcInfoLogger.h"

#include <DataFormatsITSMFT/Digit.h>
#include <DataFormatsITSMFT/ROFRecord.h>
#include <ITSMFTReconstruction/GBTLink.h>
#include <Common/DataBlock.h>
#include <DPLUtils/RawParser.h>
#include <DPLUtils/DPLRawParser.h>
#include <TCanvas.h>
#include <TDatime.h>
#include <TGraph.h>
#include <TH1.h>
#include <TPaveText.h>

#include <time.h>

using namespace std;

namespace o2::quality_control_modules::its
{

ITSOnlineTask::ITSOnlineTask()
  : TaskInterface()
{
  o2::base::GeometryManager::loadGeometry();
  mGeom = o2::its::GeometryTGeo::Instance();
}

ITSOnlineTask::~ITSOnlineTask()
{
  delete mDecoder;
  delete mProcessingTime;
  delete mProcessingTimevsTF;
  delete mTFInfo;
  delete mErrorPlots;
  delete mTriggerPlots;
  for (int ilayer = 0; ilayer < 3; ilayer++) {
    delete mChipStaveOccupancy[ilayer];
    delete mOccupancyPlot[ilayer];
    for (int istave = 0; istave < 20; istave++) {
      delete mHicHitmapAddress[ilayer][istave];
    }
  }
  delete mGeom;
}

void ITSOnlineTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  QcInfoLogger::GetInstance() << "initialize ITSOnlineTask" << AliceO2::InfoLogger::InfoLogger::endm;
  ///////////RawPixelDecoder/////////////////
  mDecoder = new o2::itsmft::RawPixelDecoder<o2::itsmft::ChipMappingITS>();
  mDecoder->init();
  mDecoder->setNThreads(2);
  mDecoder->setFormat(GBTLink::OldFormat);
  mDecoder->setUserDataOrigin(header::DataOrigin("DS"));
  mChipsBuffer.resize(432); //temporarily hardcoded for IB; TODO: extend to OB.
                            ///////////RawPixelDecode end//////////////

  mErrorPlots = new TH1D("General/ErrorPlots", "Decoding Errors", mNError, 0.5, mNError + 0.5);
  mErrorPlots->SetMinimum(0);
  mErrorPlots->SetFillColor(kRed);
  getObjectsManager()->startPublishing(mErrorPlots); //mErrorPlots

  mTriggerPlots = new TH1D("General/TriggerPlots", "Decoding Triggers", mNTrigger, 0.5, mNTrigger + 0.5);
  mTriggerPlots->SetMinimum(0);
  mTriggerPlots->SetFillColor(kBlue);
  getObjectsManager()->startPublishing(mTriggerPlots); //mTriggerPlots

  mProcessingTime = new TH1F("processingtime", "Processing Time of one TF", 1000, 1000, 4000);
  getObjectsManager()->startPublishing(mProcessingTime); //mProcessingTime

  mProcessingTimevsTF = new TH1F("processing_time_vs_TF", "Processing Time vs TF", 20000, 0, 20000);
  getObjectsManager()->startPublishing(mProcessingTimevsTF); //mProcessingTimevsTF

  mTFInfo = new TH1F("General/TFInfo", "TF vs count", 15000, 0, 15000);
  getObjectsManager()->startPublishing(mTFInfo); //mTFInfo

  for (int ilayer = 0; ilayer < 3; ilayer++) {
    for (int istave = 0; istave < (12 + (4 * ilayer)); istave++) {
      mHicHitmapAddress[ilayer][istave] = new TH2I(Form("Occupancy/Layer%d/Stave%d/Layer%dStave%dHITMAP", ilayer, istave, ilayer, istave), Form("Hits on Layer %d, Stave %d", ilayer, istave), 256 * 9, 0, 1024 * 9, 128, 0, 512);
      getObjectsManager()->startPublishing(mHicHitmapAddress[ilayer][istave]); //mHicHItmapAddress
    }

    mChipStaveOccupancy[ilayer] = new TH2D(Form("Occupancy/Layer%d/Layer%dChipStave", ilayer, ilayer), Form("ITS Layer%d, Occupancy vs Chip and Stave", ilayer), 9, -0.5, 8.5, 12 + (ilayer * 4), -0.5, 11.5 + (ilayer * 4));
    getObjectsManager()->startPublishing(mChipStaveOccupancy[ilayer]); //mChipStaveOccupancy

    mOccupancyPlot[ilayer] = new TH1D(Form("Occupancy/Layer%dOccupancy", ilayer), Form("ITS Layer %d Occupancy Distribution", ilayer), 300, -15, 0);
    getObjectsManager()->startPublishing(mOccupancyPlot[ilayer]); //mOccupancyPlot
  }
}

void ITSOnlineTask::createPlots()
{
  //create general plots
  mErrorPlots = new TH1D("General/ErrorPlots", "Decoding Errors", mNError, 0.5, mNError + 0.5);
  mErrorPlots->SetMinimum(0);
  mErrorPlots->SetFillColor(kRed);
  getObjectsManager()->startPublishing(mErrorPlots); //mErrorPlots

  mTriggerPlots = new TH1D("General/TriggerPlots", "Decoding Triggers", mNTrigger, 0.5, mNTrigger + 0.5);
  mTriggerPlots->SetMinimum(0);
  mTriggerPlots->SetFillColor(kBlue);
  getObjectsManager()->startPublishing(mTriggerPlots); //mTriggerPlots

  mProcessingTime = new TH1F("processingtime", "Processing Time of one TF", 1000, 1000, 4000);
  getObjectsManager()->startPublishing(mProcessingTime); //mProcessingTime

  mProcessingTimevsTF = new TH1F("processing_time_vs_TF", "Processing Time vs TF", 20000, 0, 20000);
  getObjectsManager()->startPublishing(mProcessingTimevsTF); //mProcessingTimevsTF

  mTFInfo = new TH1F("General/TFInfo", "TF vs count", 15000, 0, 15000);
  getObjectsManager()->startPublishing(mTFInfo); //mTFInfo
                                                 //create general plots end

  //create occupancy plots
  for (int ilayer = 0; ilayer < 3; ilayer++) {
    for (int istave = 0; istave < (12 + (4 * ilayer)); istave++) {
      mHicHitmapAddress[ilayer][istave] = new TH2I(Form("Occupancy/Layer%d/Stave%d/Layer%dStave%dHITMAP", ilayer, istave, ilayer, istave), Form("Hits on Layer %d, Stave %d", ilayer, istave), 256 * 9, 0, 1024 * 9, 128, 0, 512);
      getObjectsManager()->startPublishing(mHicHitmapAddress[ilayer][istave]); //mHicHItmapAddress
    }

    mChipStaveOccupancy[ilayer] = new TH2D(Form("Occupancy/Layer%d/Layer%dChipStave", ilayer, ilayer), Form("ITS Layer%d, Occupancy vs Chip and Stave", ilayer), 9, -0.5, 8.5, 12 + (ilayer * 4), -0.5, 11.5 + (ilayer * 4));
    getObjectsManager()->startPublishing(mChipStaveOccupancy[ilayer]); //mChipStaveOccupancy

    mOccupancyPlot[ilayer] = new TH1D(Form("Occupancy/Layer%dOccupancy", ilayer), Form("ITS Layer %d Occupancy Distribution", ilayer), 300, -15, 0);
    getObjectsManager()->startPublishing(mOccupancyPlot[ilayer]); //mOccupancyPlot
  }
  //create occupancy plots end
}

void ITSOnlineTask::startOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void ITSOnlineTask::startOfCycle() { QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm; }

void ITSOnlineTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // in a loop
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  std::chrono::time_point<std::chrono::high_resolution_clock> end;
  int difference;
  start = std::chrono::high_resolution_clock::now();
  ////////////RawPixelDecoder///////////////////

  int lay, sta, ssta, mod, chip;
  std::vector<Digit> digVec;
  std::vector<ROFRecord> digROFVec;

  mDecoder->startNewTF(ctx.inputs());
  mDecoder->setDecodeNextAuto(true);

  while ((mChipDataBuffer = mDecoder->getNextChipData(mChipsBuffer))) {
    if (mChipDataBuffer) {
      const auto& pixels = mChipDataBuffer->getData();
      for (auto& pixel : pixels) {
        mGeom->getChipId(mChipDataBuffer->getChipID(), lay, sta, ssta, mod, chip);
        mHitNumberOfChip[lay][sta][chip]++;
        mHicHitmapAddress[lay][sta]->Fill((int)(pixel.getCol() + chip * 1024), (int)pixel.getRow());
        bool flag = true;
        for (int jpos = 0; jpos < (int)mHitPixelID[lay][sta][chip].size(); jpos++) {
          if ((pixel.getCol() == mHitPixelID[lay][sta][chip][jpos].first) and (pixel.getRow() == mHitPixelID[lay][sta][chip][jpos].second)) {
            flag = false;
            break;
          }
        }
        if (flag) {
          mHitPixelID[lay][sta][chip].push_back(make_pair(pixel.getCol(), pixel.getRow()));
        }
      }
    }
  }

  mErrorPlots->Reset();
  for (int ilayer = 0; ilayer < 3; ilayer++) {
    for (int istave = 0; istave < (12 + (4 * ilayer)); istave++) {
      const o2::itsmft::RUDecodeData* RUdecode;
      const auto* mDecoderTmp = mDecoder;
      int RUid = 0;
      if (ilayer == 0) {
        RUid = istave;
      } else if (ilayer == 1) {
        RUid = istave + 12;
      } else if (ilayer == 2) {
        RUid = istave + 28;
      }
      RUdecode = mDecoderTmp->getRUDecode(RUid);
      if (!RUdecode) {
        continue;
      }
      for (int ilink = 0; ilink < 1; ilink++) {
        const auto* GBTLinkInfo = RUdecode->cableLinkPtr[ilink];
        if (!GBTLinkInfo) {
          continue;
        }
        for (int ichip = 0; ichip < 9; ichip++) {
          if ((GBTLinkInfo->statistics.nPackets > 0) and (mHitNumberOfChip[ilayer][istave][ichip] >= 0)) {
            mChipStaveOccupancy[ilayer]->SetBinContent(ichip + 1, istave + 1, (mHitNumberOfChip[ilayer][istave][ichip]) / (GBTLinkInfo->statistics.nPackets * 1024. * 512.));
          }
        }
        for (int ierror = 0; ierror < o2::itsmft::GBTLinkDecodingStat::NErrorsDefined; ierror++) {
          mErrorPlots->AddBinContent(ierror + 1, GBTLinkInfo->statistics.errorCounts[ierror]);
        }
      }
    }
  }

  DPLRawParser parser(ctx.inputs());
  for (auto it = parser.begin(), end = parser.end(); it != end; ++it) {
    auto const* rdh = it.get_if<o2::header::RAWDataHeaderV4>();
    for (int i = 0; i < 13; i++) {
      if (((uint32_t)(rdh->triggerType) >> i & 1) == 1) {
        mTriggerPlots->Fill(i + 1);
      }
    }
  }

  for (auto&& input : ctx.inputs()) {
    if (input.payload != nullptr and input.header != nullptr) {
      const auto* header = o2::header::get<header::DataHeader*>(input.header);
      if ((int)header->payloadSize == 8) {
        mTimeFrameId = (int)*((uint64_t*)input.payload);
        break;
      }
    }
  }
  //push raw data to td end
  //Fill Occupancy distribution plots end

  mTFInfo->Fill(mTimeFrameId);
  end = std::chrono::high_resolution_clock::now();
  difference = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  ILOG(Info) << "time until thread all end is " << difference << ", and TF ID == " << mTimeFrameId << ENDM;
}

void ITSOnlineTask::endOfCycle()
{
  for (int ilayer = 0; ilayer < 3; ilayer++) {
    getObjectsManager()->addMetadata(mChipStaveOccupancy[ilayer]->GetName(), "Run", "000000"); //temporarily hardcoded run number; TODO: pass it through ECS
    getObjectsManager()->addMetadata(mOccupancyPlot[ilayer]->GetName(), "Run", "000000");
    for (int istave = 0; istave < (12 + (ilayer * 4)); istave++) {
      getObjectsManager()->addMetadata(mHicHitmapAddress[ilayer][istave]->GetName(), "Run", "000000");
      for (int ichip = 0; ichip < 9; ichip++) {
      }
    }
  }
  ILOG(Info) << "endOfCycle" << ENDM;
}

void ITSOnlineTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "endOfActivity" << ENDM;
}

void ITSOnlineTask::reset() { ILOG(Info) << "Reset" << ENDM; }

} // namespace o2::quality_control_modules::its
