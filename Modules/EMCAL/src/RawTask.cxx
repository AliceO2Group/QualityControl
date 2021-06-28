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
/// \file   RawTask.cxx
/// \author Cristina Terrevoli
/// \author Markus Fasel
///

#include <TCanvas.h>
#include <TH1.h>
#include <TProfile2D.h>
#include <TMath.h>
#include <cfloat>

#include "QualityControl/QcInfoLogger.h"
#include "DetectorsRaw/RDHUtils.h"
#include "EMCAL/RawTask.h"
#include "Headers/RAWDataHeader.h"
#include "EMCALReconstruction/AltroDecoder.h"
#include "EMCALReconstruction/RawReaderMemory.h"
#include "EMCALReconstruction/RawHeaderStream.h"
#include <Framework/InputRecord.h>
#include <CommonConstants/Triggers.h>

using namespace o2::emcal;

namespace o2::quality_control_modules::emcal
{

RawTask::~RawTask()
{
  if (mHistogram) {
    delete mHistogram;
  }
  if (mPayloadSizePerDDL) {
    delete mPayloadSizePerDDL;
  }
  if (mMessageCounter) {
    delete mMessageCounter;
  }
  if (mPageCounter) {
    delete mPageCounter;
  }
  if (mSuperpageCounter) {
    delete mSuperpageCounter;
  }
  if (mNumberOfPagesPerMessage) {
    delete mNumberOfPagesPerMessage;
  }
  if (mNumberOfSuperpagesPerMessage) {
    delete mNumberOfSuperpagesPerMessage;
  }
  if (mErrorTypeAltro) {
    delete mErrorTypeAltro;
  }

  for (auto& histos : mRawAmplitudeEMCAL) {
    for (auto h : histos.second) {

      delete h;
    }
  }

  for (auto& histos : mRawAmplMaxEMCAL) {
    for (auto h : histos.second) {
      delete h;
    }
  }

  for (auto& histos : mRawAmplMinEMCAL) {
    for (auto h : histos.second) {
      delete h;
    }
  }
  for (auto& histos : mRMSperSM) {
    for (auto h : histos.second) {
      delete h;
    }
  }

  for (auto& histos : mMEANperSM) {
    for (auto h : histos.second) {
      delete h;
    }
  }

  for (auto& histos : mMAXperSM) {
    for (auto h : histos.second) {
      delete h;
    }
  }
  for (auto& histos : mMINperSM) {
    for (auto h : histos.second) {
      delete h;
    }
  }
}
void RawTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  using infoCONTEXT = AliceO2::InfoLogger::InfoLoggerContext;
  infoCONTEXT context;
  context.setField(infoCONTEXT::FieldName::Facility, "QC");
  context.setField(infoCONTEXT::FieldName::System, "QC");
  context.setField(infoCONTEXT::FieldName::Detector, "EMC");
  QcInfoLogger::GetInstance().setContext(context);
  QcInfoLogger::GetInstance() << "initialize RawTask" << AliceO2::InfoLogger::InfoLogger::endm;

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
    QcInfoLogger::GetInstance() << "Custom parameter - myOwnKey : " << param->second << AliceO2::InfoLogger::InfoLogger::endm;
  }
  mMappings = std::unique_ptr<o2::emcal::MappingHandler>(new o2::emcal::MappingHandler); //initialize the unique pointer to Mapper

  // Statistics histograms
  mMessageCounter = new TH1F("NumberOfMessages", "Number of messages in time interval", 1, 0.5, 1.5);
  mMessageCounter->GetXaxis()->SetTitle("MonitorData");
  mMessageCounter->GetYaxis()->SetTitle("Number of messages");
  getObjectsManager()->startPublishing(mMessageCounter);

  mSuperpageCounter = new TH1F("NumberOfSuperpages", "Number of superpages in time interval", 1, 0.5, 1.5);
  mSuperpageCounter->GetXaxis()->SetTitle("MonitorData");
  mSuperpageCounter->GetYaxis()->SetTitle("Number of superpages");
  getObjectsManager()->startPublishing(mSuperpageCounter);

  mPageCounter = new TH1F("NumberOfPages", "Number of pages in time interval", 1, 0.5, 1.5);
  mPageCounter->GetXaxis()->SetTitle("MonitorData");
  mPageCounter->GetYaxis()->SetTitle("Number of pages");
  getObjectsManager()->startPublishing(mPageCounter);

  mNumberOfSuperpagesPerMessage = new TH1F("NumberOfSuperpagesPerMessage", "Number of superpages per message", 40., 0., 40.);
  mNumberOfSuperpagesPerMessage->GetXaxis()->SetTitle("Number of superpages");
  mNumberOfSuperpagesPerMessage->GetYaxis()->SetTitle("Number of messages");
  getObjectsManager()->startPublishing(mNumberOfSuperpagesPerMessage);

  mNumberOfPagesPerMessage = new TH1F("NumberOfPagesPerMessage", "Number of pages per message", 400, 0., 400.);
  mNumberOfPagesPerMessage->GetXaxis()->SetTitle("Number of pages");
  mNumberOfPagesPerMessage->GetYaxis()->SetTitle("Number of messages");
  getObjectsManager()->startPublishing(mNumberOfPagesPerMessage);

  mTotalDataVolume = new TH1D("TotalDataVolume", "Total data volume", 1, 0.5, 1.5);
  mTotalDataVolume->GetXaxis()->SetTitle("MonitorData");
  mTotalDataVolume->GetYaxis()->SetTitle("Total data volume (Byte)");
  getObjectsManager()->startPublishing(mTotalDataVolume);

  // EMCAL related histograms
  mPayloadSizePerDDL = new TH2F("PayloadSizePerDDL", "PayloadSizePerDDL", 40, 0, 40, 100, 0, 1);
  mPayloadSizePerDDL->GetXaxis()->SetTitle("ddl");
  mPayloadSizePerDDL->GetYaxis()->SetTitle("PayloadSize");
  getObjectsManager()->startPublishing(mPayloadSizePerDDL);

  mHistogram = new TH1F("PayloadSize", "PayloadSize", 20, 0, 60000000); //
  mHistogram->GetXaxis()->SetTitle("bytes");
  mHistogram->GetYaxis()->SetTitle("Counts");
  getObjectsManager()->startPublishing(mHistogram);

  mErrorTypeAltro = new TH2F("ErrorTypePerSM", "ErrorTypeForSM", 40, 0, 40, 8, 0, 8);
  mErrorTypeAltro->GetXaxis()->SetTitle("SM");
  mErrorTypeAltro->GetYaxis()->SetTitle("Error Type");
  mErrorTypeAltro->GetYaxis()->SetBinLabel(1, "RCU Trailer");
  mErrorTypeAltro->GetYaxis()->SetBinLabel(2, "RCU Version");
  mErrorTypeAltro->GetYaxis()->SetBinLabel(3, "RCU Trailer Size");
  mErrorTypeAltro->GetYaxis()->SetBinLabel(4, "ALTRO Bunch Header");
  mErrorTypeAltro->GetYaxis()->SetBinLabel(5, "ALTRO Bunch Length");
  mErrorTypeAltro->GetYaxis()->SetBinLabel(6, "ALTRO Payload");
  mErrorTypeAltro->GetYaxis()->SetBinLabel(7, "ALTRO Mapping");
  mErrorTypeAltro->GetYaxis()->SetBinLabel(8, "Channel");
  getObjectsManager()->startPublishing(mErrorTypeAltro);

  //histos per SM

  std::array<std::string, 2> triggers = { { "CAL", "PHYS" } };
  for (const auto& trg : triggers) {
    std::array<TH1*, 20> histosRawAmplEMCALSM;
    std::array<TH1*, 20> histosRawAmplMaxEMCALSM;
    std::array<TH1*, 20> histosRawAmplMinEMCALSM;
    std::array<TProfile2D*, 20> histosRawAmplRmsSM;
    std::array<TProfile2D*, 20> histosRawAmplMeanSM;
    std::array<TProfile2D*, 20> histosRawAmplMaxSM;
    std::array<TProfile2D*, 20> histosRawAmplMinSM;

    for (auto ism = 0; ism < 20; ism++) {

      histosRawAmplEMCALSM[ism] = new TH1F(Form("RawAmplitudeEMCAL_sm%d_%s", ism, trg.data()), Form(" RawAmplitudeEMCAL%d, %s", ism, trg.data()), 100, 0., 100.);
      histosRawAmplEMCALSM[ism]->GetXaxis()->SetTitle("Raw Amplitude");
      histosRawAmplEMCALSM[ism]->GetYaxis()->SetTitle("Counts");
      getObjectsManager()->startPublishing(histosRawAmplEMCALSM[ism]);

      histosRawAmplMaxEMCALSM[ism] = new TH1F(Form("RawAmplMaxEMCAL_sm%d_%s", ism, trg.data()), Form(" RawAmplMaxEMCAL_sm%d_%s", ism, trg.data()), 500, 0., 500.);
      histosRawAmplMaxEMCALSM[ism]->GetXaxis()->SetTitle("Max Raw Amplitude [ADC]");
      histosRawAmplMaxEMCALSM[ism]->GetYaxis()->SetTitle("Counts");
      getObjectsManager()->startPublishing(histosRawAmplMaxEMCALSM[ism]);

      histosRawAmplMinEMCALSM[ism] = new TH1F(Form("RawAmplMinEMCAL_sm%d_%s", ism, trg.data()), Form("RawAmplMinEMCAL_sm%d_%s", ism, trg.data()), 100, 0., 100.);
      histosRawAmplMinEMCALSM[ism]->GetXaxis()->SetTitle("Min Raw Amplitude");
      histosRawAmplMinEMCALSM[ism]->GetYaxis()->SetTitle("Counts");
      getObjectsManager()->startPublishing(histosRawAmplMinEMCALSM[ism]);

      histosRawAmplRmsSM[ism] = new TProfile2D(Form("RMSADCperSM%d_%s", ism, trg.data()), Form("RMSperSM%d_%s", ism, trg.data()), 48, 0, 48, 24, 0, 24);
      histosRawAmplRmsSM[ism]->GetXaxis()->SetTitle("col");
      histosRawAmplRmsSM[ism]->GetYaxis()->SetTitle("row");
      getObjectsManager()->startPublishing(histosRawAmplRmsSM[ism]);

      histosRawAmplMeanSM[ism] = new TProfile2D(Form("MeanADCperSM%d_%s", ism, trg.data()), Form("MeanADCperSM%d_%s", ism, trg.data()), 48, 0, 48, 24, 0, 24);
      histosRawAmplMeanSM[ism]->GetXaxis()->SetTitle("col");
      histosRawAmplMeanSM[ism]->GetYaxis()->SetTitle("row");
      getObjectsManager()->startPublishing(histosRawAmplMeanSM[ism]);

      histosRawAmplMaxSM[ism] = new TProfile2D(Form("MaxADCperSM%d_%s", ism, trg.data()), Form("MaxADCperSM%d_%s", ism, trg.data()), 48, 0, 47, 24, 0, 23);
      histosRawAmplMaxSM[ism]->GetXaxis()->SetTitle("col");
      histosRawAmplMaxSM[ism]->GetYaxis()->SetTitle("row");
      getObjectsManager()->startPublishing(histosRawAmplMaxSM[ism]);

      histosRawAmplMinSM[ism] = new TProfile2D(Form("MinADCperSM%d_%s", ism, trg.data()), Form("MinADCperSM%d_%s", ism, trg.data()), 48, 0, 47, 24, 0, 23);
      histosRawAmplMinSM[ism]->GetXaxis()->SetTitle("col");
      histosRawAmplMinSM[ism]->GetYaxis()->SetTitle("raw");
      getObjectsManager()->startPublishing(histosRawAmplMinSM[ism]);
    } //loop SM
    mRawAmplitudeEMCAL[trg] = histosRawAmplEMCALSM;
    mRawAmplMaxEMCAL[trg] = histosRawAmplMaxEMCALSM;
    mRawAmplMinEMCAL[trg] = histosRawAmplMinEMCALSM;
    mRMSperSM[trg] = histosRawAmplRmsSM;
    mMEANperSM[trg] = histosRawAmplMeanSM;
    mMAXperSM[trg] = histosRawAmplMaxSM;
    mMINperSM[trg] = histosRawAmplMinSM;

  } //loop trigger case
}

void RawTask::startOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
  reset();
}

void RawTask::startOfCycle()
{
  QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void RawTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // In this function you can access data inputs specified in the JSON config file, for example:
  //   "query": "random:ITS/RAWDATA/0"
  // which is correspondingly <binding>:<dataOrigin>/<dataDescription>/<subSpecification
  // One can also access conditions from CCDB, via separate API (see point 3)

  // Use Framework/DataRefUtils.h or Framework/InputRecord.h to access and unpack inputs (both are documented)
  // One can find additional examples at:
  // https://github.com/AliceO2Group/AliceO2/blob/dev/Framework/Core/README.md#using-inputs---the-inputrecord-api

  using CHTYP = o2::emcal::ChannelType_t;

  Int_t nPagesMessage = 0, nSuperpagesMessage = 0;
  QcInfoLogger::GetInstance() << QcInfoLogger::Debug << " Processing message " << mNumberOfMessages << AliceO2::InfoLogger::InfoLogger::endm;
  mNumberOfMessages++;
  mMessageCounter->Fill(1);

  // Some examples:
  // 1. In a loop
  for (auto&& input : ctx.inputs()) {
    // get message header
    if (input.header != nullptr && input.payload != nullptr) {
      const auto* header = header::get<header::DataHeader*>(input.header);
      // get payload of a specific input, which is a char array.
      // const char* payload = input.payload;
      QcInfoLogger::GetInstance() << QcInfoLogger::Debug << "Processing superpage " << mNumberOfSuperpages << AliceO2::InfoLogger::InfoLogger::endm;
      mNumberOfSuperpages++;
      nSuperpagesMessage++;
      mSuperpageCounter->Fill(1);
      QcInfoLogger::GetInstance() << QcInfoLogger::Debug << " EMCAL Reading Payload size: " << header->payloadSize << " for " << header->dataOrigin << AliceO2::InfoLogger::InfoLogger::endm;

      //fill the histogram with payload sizes
      mHistogram->Fill(header->payloadSize);
      mTotalDataVolume->Fill(1., header->payloadSize);

      // Skip SOX headers
      auto rdhblock = reinterpret_cast<const o2::header::RDHAny*>(input.payload);
      if (o2::raw::RDHUtils::getHeaderSize(rdhblock) == static_cast<int>(header->payloadSize)) {
        continue;
      }

      // try decoding payload
      o2::emcal::RawReaderMemory rawreader(gsl::span(input.payload, header->payloadSize));
      uint64_t currentTrigger(0);
      bool first = true; //for the first event
      short int maxADCSM[20];
      short int minADCSM[20];
      while (rawreader.hasNext()) {
        QcInfoLogger::GetInstance() << QcInfoLogger::Debug << " Processing page " << mNumberOfPages << AliceO2::InfoLogger::InfoLogger::endm;
        mNumberOfPages++;
        nPagesMessage++;
        mPageCounter->Fill(1);
        rawreader.next();
        auto payLoadSize = rawreader.getPayloadSize(); //payloadsize in byte;

        auto headerR = rawreader.getRawHeader();
        auto feeID = o2::raw::RDHUtils::getFEEID(headerR);
        auto triggerBC = o2::raw::RDHUtils::getTriggerBC(headerR);
        mPayloadSizePerDDL->Fill(feeID, payLoadSize / 1024.);

        //trigger type
        auto triggertype = o2::raw::RDHUtils::getTriggerType(headerR);
        bool isPhysTrigger = triggertype & o2::trigger::PhT, isCalibTrigger = triggertype & o2::trigger::Cal;
        std::string trgClass;
        if (isPhysTrigger)
          trgClass = "PHYS";
        else if (isCalibTrigger)
          trgClass = "CAL";
        else {
          QcInfoLogger::GetInstance() << QcInfoLogger::Error << " Unmonitored trigger class requested " << AliceO2::InfoLogger::InfoLogger::endm;
          continue;
        }

        //fill histograms with max ADC for each supermodules and reset cache
        if (!first) {                       // check if it is the first event in the payload
          if (triggerBC > currentTrigger) { // new event
            for (int sm = 0; sm < 20; sm++) {

              mRawAmplitudeEMCAL[trgClass][sm]->Fill(maxADCSM[sm]);

              maxADCSM[sm] = 0;
              //initialize
              minADCSM[sm] = SHRT_MAX;
            } //sm loop
            currentTrigger = triggerBC;
          }      //new event
        } else { //first
          currentTrigger = triggerBC;
          first = false;
        }
        if (feeID > 40)
          continue; //skip STU ddl

        o2::emcal::AltroDecoder decoder(rawreader); //(atrodecoder in Detectors/Emcal/reconstruction/src)
        //check the words of the payload exception in altrodecoder
        try {
          decoder.decode();
        } catch (AltroDecoderError& e) {
          std::stringstream errormessage;
          using AltroErrType = o2::emcal::AltroDecoderError::ErrorType_t;
          int errornum = -1;
          switch (e.getErrorType()) {
            case AltroErrType::RCU_TRAILER_ERROR:
              errornum = 0;
              errormessage << " RCU Trailer Error ";
              break;
            case AltroErrType::RCU_VERSION_ERROR:
              errornum = 1;
              errormessage << " RCU Version Error ";
              break;
            case AltroErrType::RCU_TRAILER_SIZE_ERROR:
              errornum = 2;
              errormessage << " RCU Trailer Size Error ";
              break;
            case AltroErrType::ALTRO_BUNCH_HEADER_ERROR:
              errornum = 3;
              errormessage << " ALTRO Bunch Header Error ";
              break;
            case AltroErrType::ALTRO_BUNCH_LENGTH_ERROR:
              errornum = 4;
              errormessage << " ALTRO Bunch Length Error ";
              break;
            case AltroErrType::ALTRO_PAYLOAD_ERROR:
              errornum = 5;
              errormessage << " ALTRO Payload Error ";
              break;
            case AltroErrType::ALTRO_MAPPING_ERROR:
              errornum = 6;
              errormessage << " ALTRO Mapping Error ";
              break;
            case AltroErrType::CHANNEL_ERROR:
              errornum = 7;
              errormessage << " Channel Error ";
              break;
            default:
              break;
          }
          errormessage << " in Supermodule " << feeID;
          QcInfoLogger::GetInstance() << QcInfoLogger::Error << " EMCAL raw task: " << errormessage.str() << AliceO2::InfoLogger::InfoLogger::endm;
          //fill histograms  with error types
          mErrorTypeAltro->Fill(feeID, errornum);
          continue;
        }
        int j = feeID / 2; //SM id
        auto& mapping = mMappings->getMappingForDDL(feeID);
        int col;

        int row;

        for (auto& chan : decoder.getChannels()) {
          col = mapping.getColumn(chan.getHardwareAddress());
          row = mapping.getRow(chan.getHardwareAddress());
          //exclude LED Mon, TRU
          auto chType = mapping.getChannelType(chan.getHardwareAddress());
          if (chType == CHTYP::LEDMON || chType == CHTYP::TRU)
            continue;

          Short_t maxADC = 0;
          Short_t minADC = SHRT_MAX;
          Double_t meanADC = 0;
          Double_t rmsADC = 0;
          for (auto& bunch : chan.getBunches()) {
            auto adcs = bunch.getADC();

            auto maxADCbunch = *max_element(adcs.begin(), adcs.end());
            if (maxADCbunch > maxADC)
              maxADC = maxADCbunch;
            mRawAmplMaxEMCAL[trgClass][j]->Fill(maxADCbunch); //max for each cell

            auto minADCbunch = *min_element(adcs.begin(), adcs.end());
            if (minADCbunch < minADC)
              minADC = minADCbunch;
            mRawAmplMinEMCAL[trgClass][j]->Fill(minADCbunch); // min for each cell

            meanADC = TMath::Mean(adcs.begin(), adcs.end());
            rmsADC = TMath::RMS(adcs.begin(), adcs.end());
            mRMSperSM[trgClass][j]->Fill(col, row, rmsADC);
            mMEANperSM[trgClass][j]->Fill(col, row, meanADC);
          }
          if (maxADC > maxADCSM[j])
            maxADCSM[j] = maxADC;
          mMAXperSM[trgClass][j]->Fill(col, row, maxADC);

          if (minADC < minADCSM[j])
            minADCSM[j] = minADC;
          mMINperSM[trgClass][j]->Fill(col, row, minADC);
        } //channels
      }   //new page
    }     //header
  }       //inputs
  mNumberOfPagesPerMessage->Fill(nPagesMessage);
  mNumberOfSuperpagesPerMessage->Fill(nSuperpagesMessage);
} //function monitor data

void RawTask::endOfCycle()
{
  QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void RawTask::endOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
  QcInfoLogger::GetInstance() << "Total amount of messages: " << mNumberOfMessages << AliceO2::InfoLogger::InfoLogger::endm;
  QcInfoLogger::GetInstance() << "Total amount of superpages: " << mNumberOfSuperpages << ", pages: " << mNumberOfPages << AliceO2::InfoLogger::InfoLogger::endm;
}

void RawTask::reset()
{
  // clean all the monitor objects here

  QcInfoLogger::GetInstance() << "Resetting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;
  mHistogram->Reset();
  std::array<std::string, 2> triggers = { { "CAL", "PHYS" } };
  for (const auto& trg : triggers) {
    for (Int_t i = 0; i < 20; i++) {
      mRawAmplitudeEMCAL[trg][i]->Reset();
      mRawAmplMaxEMCAL[trg][i]->Reset();
      mRawAmplMinEMCAL[trg][i]->Reset();
      mRMSperSM[trg][i]->Reset();
      mMEANperSM[trg][i]->Reset();
      mMAXperSM[trg][i]->Reset();
      mMINperSM[trg][i]->Reset();
    }
  }
  mPayloadSizePerDDL->Reset();
  mHistogram->Reset();
  mErrorTypeAltro->Reset();
}
} // namespace o2::quality_control_modules::emcal
