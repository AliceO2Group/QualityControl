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
#include "EMCAL/RawTask.h"
#include "Headers/RAWDataHeader.h"
#include "EMCALReconstruction/AltroDecoder.h"
#include "EMCALReconstruction/RawReaderMemory.h"
#include "EMCALReconstruction/RawHeaderStream.h"

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

  for (auto h : mRawAmplitudeEMCAL) {
    delete h;
  }

  for (auto h : mRawAmplMaxEMCAL) {
    delete h;
  }

  for (auto h : mRawAmplMinEMCAL) {
    delete h;
  }

  for (auto h : mRMSperSM) {
    delete h;
  }

  for (auto h : mMEANperSM) {
    delete h;
  }

  for (auto h : mMAXperSM) {
    delete h;
  }

  for (auto h : mMINperSM) {
    delete h;
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
  for (Int_t i = 0; i < 20; i++) {
    mRawAmplitudeEMCAL[i] = new TH1F(Form("RawAmplitudeEMCAL_sm%d", i), Form(" RawAmplitudeEMCAL%d", i), 100, 0., 100.);
    mRawAmplitudeEMCAL[i]->GetXaxis()->SetTitle("Raw Amplitude");
    mRawAmplitudeEMCAL[i]->GetYaxis()->SetTitle("Counts");
    getObjectsManager()->startPublishing(mRawAmplitudeEMCAL[i]);

    mRawAmplMaxEMCAL[i] = new TH1F(Form("RawAmplMaxEMCAL_sm%d", i), Form(" RawAmpMaxEMCAL%d", i), 100, 0., 100.);
    mRawAmplMaxEMCAL[i]->GetXaxis()->SetTitle("Max Raw Amplitude");
    mRawAmplMaxEMCAL[i]->GetYaxis()->SetTitle("Counts");
    getObjectsManager()->startPublishing(mRawAmplMaxEMCAL[i]);

    mRawAmplMinEMCAL[i] = new TH1F(Form("RawAmplMinEMCAL_sm%d", i), Form(" RawAmpMinEMCAL%d", i), 100, 0., 100.);
    mRawAmplMinEMCAL[i]->GetXaxis()->SetTitle("Min Raw Amplitude");
    mRawAmplMinEMCAL[i]->GetYaxis()->SetTitle("Counts");
    getObjectsManager()->startPublishing(mRawAmplMinEMCAL[i]);

    mRMSperSM[i] = new TProfile2D(Form("RMSADCperSM%d", i), Form("RMSperSM%d", i), 48, 0, 48, 24, 0, 24);
    mRMSperSM[i]->GetXaxis()->SetTitle("col");
    mRMSperSM[i]->GetYaxis()->SetTitle("row");
    getObjectsManager()->startPublishing(mRMSperSM[i]);

    mMEANperSM[i] = new TProfile2D(Form("MeanADCperSM%d", i), Form("MeanADCperSM%d", i), 48, 0, 48, 24, 0, 24);
    mMEANperSM[i]->GetXaxis()->SetTitle("col");
    mMEANperSM[i]->GetYaxis()->SetTitle("row");
    getObjectsManager()->startPublishing(mMEANperSM[i]);

    mMAXperSM[i] = new TProfile2D(Form("MaxADCperSM%d", i), Form("MaxADCperSM%d", i), 48, 0, 47, 24, 0, 23);
    mMAXperSM[i]->GetXaxis()->SetTitle("col");
    mMAXperSM[i]->GetYaxis()->SetTitle("row");
    getObjectsManager()->startPublishing(mMAXperSM[i]);

    mMINperSM[i] = new TProfile2D(Form("MinADCperSM%d", i), Form("MinADCperSM%d", i), 48, 0, 47, 24, 0, 23);
    mMINperSM[i]->GetXaxis()->SetTitle("col");
    mMINperSM[i]->GetYaxis()->SetTitle("row");
    getObjectsManager()->startPublishing(mMINperSM[i]);
  }
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

      // try decoding payload
      o2::emcal::RawReaderMemory<o2::header::RAWDataHeaderV4> rawreader(gsl::span(input.payload, header->payloadSize));
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
        mPayloadSizePerDDL->Fill(headerR.feeId, payLoadSize / 1024.);

        //fill histograms with max ADC for each supermodules and reset cache
        if (!first) {                               // check if it is the first event in the payload
          if (headerR.triggerBC > currentTrigger) { //new event
            for (int sm = 0; sm < 20; sm++) {
              mRawAmplitudeEMCAL[sm]->Fill(maxADCSM[sm]);
              maxADCSM[sm] = 0;
              //initialize
              minADCSM[sm] = SHRT_MAX;
            } //sm loop
            currentTrigger = headerR.triggerBC;
          }      //new event
        } else { //first
          currentTrigger = headerR.triggerBC;
          first = false;
        }
        if (headerR.feeId > 40)
          continue;                                                      //skip STU ddl
        o2::emcal::AltroDecoder<decltype(rawreader)> decoder(rawreader); //(atrodecoder in Detectors/Emcal/reconstruction/src)
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
          errormessage << " in Supermodule " << headerR.feeId;
          QcInfoLogger::GetInstance() << QcInfoLogger::Error << " EMCAL raw task: " << errormessage.str() << AliceO2::InfoLogger::InfoLogger::endm;
          //fill histograms  with error types
          mErrorTypeAltro->Fill(headerR.feeId, errornum);
          continue;
        }
        int j = headerR.feeId / 2; //SM id
        auto& mapping = mMappings->getMappingForDDL(headerR.feeId);
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
            mRawAmplMaxEMCAL[j]->Fill(maxADCbunch); //max for each cell

            auto minADCbunch = *min_element(adcs.begin(), adcs.end());
            if (minADCbunch < minADC)
              minADC = minADCbunch;
            mRawAmplMinEMCAL[j]->Fill(minADCbunch); //min for each cell

            meanADC = TMath::Mean(adcs.begin(), adcs.end());
            rmsADC = TMath::RMS(adcs.begin(), adcs.end());
            mRMSperSM[j]->Fill(col, row, rmsADC);
            mMEANperSM[j]->Fill(col, row, meanADC);
          }
          if (maxADC > maxADCSM[j])
            maxADCSM[j] = maxADC;
          mMAXperSM[j]->Fill(col, row, maxADC);

          if (minADC < minADCSM[j])
            minADCSM[j] = minADC;
          mMINperSM[j]->Fill(col, row, minADC);

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
  for (Int_t i = 0; i < 20; i++) {
    mRawAmplitudeEMCAL[i]->Reset();
    mRawAmplMaxEMCAL[i]->Reset();
    mRawAmplMinEMCAL[i]->Reset();
    mRMSperSM[i]->Reset();
    mMEANperSM[i]->Reset();
    mMAXperSM[i]->Reset();
    mMINperSM[i]->Reset();
  }
  mPayloadSizePerDDL->Reset();
  mHistogram->Reset();
  mErrorTypeAltro->Reset();
}
} // namespace o2::quality_control_modules::emcal
