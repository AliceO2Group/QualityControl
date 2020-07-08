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
#include <TCanvas.h>
#include <TDatime.h>
#include <TGraph.h>
#include <TH1.h>
#include <TPaveText.h>

#include <time.h>

#include <Common/DataBlock.h>
#include <DPLUtils/RawParser.h>
#include <DPLUtils/DPLRawParser.h>

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
}

void ITSOnlineTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  QcInfoLogger::GetInstance() << "initialize ITSOnlineTask" << AliceO2::InfoLogger::InfoLogger::endm;

  for (int ithread = 0; ithread < sThreadNumber; ithread++) {
    for (int idecoder = 0; idecoder < 5; idecoder++) {
      threadInfomation[ithread].rawReader[idecoder] = std::make_unique<o2::itsmft::RawPixelReader<o2::itsmft::ChipMappingITS>>();
      threadInfomation[ithread].rawReader[idecoder]->setPadding128(true);
      threadInfomation[ithread].rawReader[idecoder]->setVerbosity(0);
      threadInfomation[ithread].rawReader[idecoder]->setMinTriggersToCache(2000);
    }
    threadInfomation[ithread].chipsBuffer.resize(432);
    threadInfomation[ithread].Geom = mGeom;
  }

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
      for (int ithread = 0; ithread < sThreadNumber; ithread++) {
        threadInfomation[ithread].hicHitMap[ilayer][istave] = mHicHitmapAddress[ilayer][istave];
      }
      getObjectsManager()->startPublishing(mHicHitmapAddress[ilayer][istave]); //mHicHItmapAddress
    }

    mChipStaveOccupancy[ilayer] = new TH2D(Form("Occupancy/Layer%d/Layer%dChipStave", ilayer, ilayer), Form("ITS Layer%d, Occupancy vs Chip and Stave", ilayer), 9, -0.5, 8.5, 12 + (ilayer * 4), -0.5, 11.5 + (ilayer * 4));
    for (int ithread = 0; ithread < sThreadNumber; ithread++) {
      threadInfomation[ithread].chipStaveOccupancy[ilayer] = mChipStaveOccupancy[ilayer];
    }
    getObjectsManager()->startPublishing(mChipStaveOccupancy[ilayer]); //mChipStaveOccupancy

    mOccupancyPlot[ilayer] = new TH1D(Form("Occupancy/Layer%dOccupancy", ilayer), Form("ITS Layer %d Occupancy Distribution", ilayer), 300, -15, 0);
    getObjectsManager()->startPublishing(mOccupancyPlot[ilayer]); //mOccupancyPlot
  }

  //  createDecoder();
  //  createPlots();
}

void ITSOnlineTask::createDecoder()
{
  for (int ithread = 0; ithread < sThreadNumber; ithread++) {
    for (int idecoder = 0; idecoder < 5; idecoder++) {
      threadInfomation[ithread].rawReader[idecoder] = std::make_unique<o2::itsmft::RawPixelReader<o2::itsmft::ChipMappingITS>>();
      threadInfomation[ithread].rawReader[idecoder]->setPadding128(true);
      threadInfomation[ithread].rawReader[idecoder]->setVerbosity(0);
      threadInfomation[ithread].rawReader[idecoder]->setMinTriggersToCache(2000);
    }
    threadInfomation[ithread].chipsBuffer.resize(432);
    threadInfomation[ithread].Geom = mGeom;
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
      for (int ithread = 0; ithread < sThreadNumber; ithread++) {
        threadInfomation[ithread].hicHitMap[ilayer][istave] = mHicHitmapAddress[ilayer][istave];
      }
      getObjectsManager()->startPublishing(mHicHitmapAddress[ilayer][istave]); //mHicHItmapAddress
    }

    mChipStaveOccupancy[ilayer] = new TH2D(Form("Occupancy/Layer%d/Layer%dChipStave", ilayer, ilayer), Form("ITS Layer%d, Occupancy vs Chip and Stave", ilayer), 9, -0.5, 8.5, 12 + (ilayer * 4), -0.5, 11.5 + (ilayer * 4));
    for (int ithread = 0; ithread < sThreadNumber; ithread++) {
      threadInfomation[ithread].chipStaveOccupancy[ilayer] = mChipStaveOccupancy[ilayer];
    }
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

void* ITSOnlineTask::decodeThread(void* threadarg)
{
  int lay, sta, ssta, mod, chip;
  struct DecodeInfomation* my_data;
  my_data = (struct DecodeInfomation*)threadarg;

  my_data->errorsCount = { 0 };
  my_data->triggersCount = { 0 };
  for (int ipos = 0; ipos < (int)my_data->payloadMessage.size(); ipos++) { //loop for all payload received
    auto& rawErrorReader = reinterpret_cast<o2::itsmft::RawPixelReader<o2::itsmft::ChipMappingITS>&>(*(my_data->rawReader[ipos]));
    my_data->rawReader[ipos]->getRawBuffer().setPtr(my_data->payloadMessage[ipos]);
    my_data->rawReader[ipos]->getRawBuffer().setEnd(my_data->payloadMessage[ipos] + my_data->messageSize[ipos]);
    while ((my_data->chipDataBuffer = my_data->rawReader[ipos % 5]->getNextChipDataFromBuffer(my_data->chipsBuffer))) { //loop for payload buffer and decode trigger by trigger
      const auto* ruInfo = rawErrorReader.getCurrRUDecodeData()->ruInfo;
      const auto& statRU = rawErrorReader.getRUDecodingStatSW(ruInfo->idSW);

      for (int ierror = 0; ierror < o2::itsmft::GBTLinkDecodingStat::NErrorsDefined; ierror++) { //check error information
        my_data->errorsCount[ipos][ierror] = (unsigned int)statRU->errorCounts[ierror];
      }

      if (my_data->chipDataBuffer) { //if there are hits infomation, Fill them to plots
        const auto& pixels = my_data->chipDataBuffer->getData();
        for (auto& pixel : pixels) {
          //get the correct layer stave sub-stave module chip informtion
          my_data->Geom->getChipId(my_data->chipDataBuffer->getChipID(), lay, sta, ssta, mod, chip);
          //fill hichitmap and record the hitnumber
          my_data->hitNumberOfChip[my_data->layerId[ipos]][sta][chip]++;
          my_data->hicHitMap[my_data->layerId[ipos]][sta]->Fill((int)(pixel.getCol() + chip * 1024), (int)pixel.getRow());

          //flag for control the hitPixelID, true == hit a new pixel which have not be hit, false == hit a old pixel which have be hit
          bool flag = true;

          //check if this pixel have be hit, if yes hitnumber++, if no change flag to false
          for (int jpos = 0; jpos < (int)my_data->hitPixelID[my_data->layerId[ipos]][sta][chip].size(); jpos++) {
            if ((pixel.getCol() == my_data->hitPixelID[my_data->layerId[ipos]][sta][chip][jpos].first) and (pixel.getRow() == my_data->hitPixelID[my_data->layerId[ipos]][sta][chip][jpos].second)) {
              my_data->hitNumber[my_data->layerId[ipos]][sta][chip][jpos]++;
              flag = false;
              break;
            }
          }
          //record the new hit pixel ID
          if (flag == true) {
            my_data->hitPixelID[my_data->layerId[ipos]][sta][chip].push_back(make_pair(pixel.getCol(), pixel.getRow()));
            my_data->hitNumber[my_data->layerId[ipos]][sta][chip].push_back(1);
          }
        }
      }
    }
    uint32_t* triggerCount = my_data->rawReader[ipos]->getTriggersCount();
    for (uint32_t itrigger = 0; itrigger < my_data->NumberOfTrigger; itrigger++) {
      my_data->triggersCount[itrigger] += (int)(*(triggerCount + itrigger));
    }
  }

  //Fill the occupancy vs chip & stave plots
  for (int ipos = 0; ipos < (int)my_data->layerId.size(); ipos++) { //loop for layer received
    auto const decodestat = my_data->rawReader[ipos]->getDecodingStat();
    for (int ichip = 0; ichip < 9; ichip++) { //loop for chip
      if (my_data->hitNumberOfChip[my_data->layerId[ipos]][my_data->feeId[ipos]][ichip] == 0) {
        continue;
      }
      my_data->chipStaveOccupancy[my_data->layerId[ipos]]->SetBinContent(ichip + 1, my_data->feeId[ipos] + 1, (((double)my_data->hitNumberOfChip[my_data->layerId[ipos]][my_data->feeId[ipos]][ichip]) / (decodestat.nTriggersProcessed * 1024. * 512.)));
    }
  }

  my_data->payloadMessage.clear();
  my_data->messageSize.clear();
  my_data->layerId.clear();
  my_data->feeId.clear();
  pthread_exit(NULL);
}

void ITSOnlineTask::monitorData(o2::framework::ProcessingContext& ctx)
{

  // in a loop
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  std::chrono::time_point<std::chrono::high_resolution_clock> end;
  int difference;
  start = std::chrono::high_resolution_clock::now();

  int rc;
  pthread_t threads[sThreadNumber];
  pthread_attr_t attr;
  void* status;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  //push raw data to threadInfomation
  int threadID = 0;
  for (auto&& input : ctx.inputs()) {
    if (input.payload != nullptr and input.header != nullptr) {
      threadID = threadID % 2;
      const auto* header = o2::header::get<header::DataHeader*>(input.header);
      if ((int)header->payloadSize == 8) {
        timeFrameId = (int)*((uint64_t*)input.payload);
        continue;
      }

      o2::header::RDHLowest* rdh = reinterpret_cast<o2::header::RDHLowest*>((uint8_t*)input.payload);
      threadInfomation[threadID].messageSize.push_back((int)header->payloadSize);
      threadInfomation[threadID].payloadMessage.push_back((uint8_t*)input.payload);
      threadInfomation[threadID].feeId.push_back((int)((rdh->feeId) & 0x00ff));
      threadInfomation[threadID].layerId.push_back((int)((rdh->feeId) >> 12));
      threadID++;
    }
  }
  //push raw data to td end

  //create threads
  for (int ithread = 0; ithread < sThreadNumber; ithread++) {
    threadInfomation[ithread].threadId = ithread;
    rc = pthread_create(&threads[ithread], NULL, &ITSOnlineTask::decodeThread, (void*)&threadInfomation[ithread]);
    if (rc) {
      ILOG(Error) << "Error:unable to create thread," << rc << ENDM;
      exit(-1);
    }
  }
  //create threads end

  //reset plots if need
  for (int ilayer = 0; ilayer < 3; ilayer++) {
    mOccupancyPlot[ilayer]->Reset();
  }
  mErrorPlots->Reset();
  mTriggerPlots->Reset();
  //reset plots end

  //join threads
  pthread_attr_destroy(&attr);
  for (int ithread = 0; ithread < sThreadNumber; ithread++) {
    rc = pthread_join(threads[ithread], &status);
    if (rc) {
      ILOG(Error) << "Error:unable to join," << rc << ENDM;
      exit(-1);
    }
    for (int itrigger = 0; itrigger < mNTrigger; itrigger++) {
      mTriggerPlots->AddBinContent(itrigger + 1, threadInfomation[ithread].triggersCount[itrigger]);
    }
    for (int ierror = 0; ierror < mNError; ierror++) {
      int fillerror = 0;
      for (int ipos = 0; ipos < 5; ipos++) {
        fillerror += threadInfomation[ithread].errorsCount[ipos][ierror];
      }
      mErrorPlots->SetBinContent(ierror + 1, fillerror);
    }
  }
  //join threads end

  //Fill Occupancy distribution plots
  int pixelnumber = 0;
  for (int ithread = 0; ithread < sThreadNumber; ithread++) {
    auto const decodestat = threadInfomation[ithread].rawReader[0]->getDecodingStat();
    for (int ilayer = 0; ilayer < 3; ilayer++) {
      for (int istave = 0; istave < (12 + (ilayer * 4)); istave++) {
        for (int ichip = 0; ichip < 9; ichip++) {
          if (threadInfomation[ithread].hitPixelID[ilayer][istave][ichip].size() == 0) {
            continue;
          }
          pixelnumber += threadInfomation[ithread].hitPixelID[ilayer][istave][ichip].size();
/*          for (auto&& iPixel : threadInfomation[ithread].hitPixelID[ilayer][istave][ichip]) {
            double pixelOccupancy = (double)threadInfomation[ithread].hitNumber[ilayer][istave][ichip][counter];
            counter++;
            if (pixelOccupancy > 0) {
              pixelOccupancy /= (double)(decodestat.nTriggersProcessed);
              mOccupancyPlot[ilayer]->Fill(log10(pixelOccupancy));
            }
          }*/
          for (int counter = 0; counter < (int)threadInfomation[ithread].hitPixelID[ilayer][istave][ichip].size(); counter++) {
            double pixelOccupancy = (double)threadInfomation[ithread].hitNumber[ilayer][istave][ichip][counter];
            if (pixelOccupancy > 0) {
              pixelOccupancy /= (double)(decodestat.nTriggersProcessed);
              mOccupancyPlot[ilayer]->Fill(log10(pixelOccupancy));
            }
          }
        }
      }
    }
  }
  //Fill Occupancy distribution plots end

  mTFInfo->Fill(timeFrameId);
  end = std::chrono::high_resolution_clock::now();
  difference = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  ILOG(Info) << "time untile thread all end is " << difference << ", and TF ID == " << timeFrameId << ENDM;
}

void ITSOnlineTask::endOfCycle()
{
  for (int ilayer = 0; ilayer < 3; ilayer++) {
    getObjectsManager()->addMetadata(mChipStaveOccupancy[ilayer]->GetName(), "Run", "000000");
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
