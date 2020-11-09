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
/// \author Dmitri Peresunko
/// \author Dmitry Blau
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TMath.h>
#include <cfloat>

#include "QualityControl/QcInfoLogger.h"
#include "PHOS/RawTask.h"
#include "Headers/RAWDataHeader.h"
//#include "PHOSReconstruction/AltroDecoder.h"
//#include "PHOSReconstruction/RawReaderMemory.h"
//#include "PHOSReconstruction/RawHeaderStream.h"
#include <Framework/InputRecord.h>

//using namespace o2::phos;

namespace o2::quality_control_modules::phos
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

  for (auto h : mRawAmplitudePHOS) {
    delete h;
  }

  for (auto h : mRawAmplMaxPHOS) {
    delete h;
  }

  for (auto h : mRawAmplMinPHOS) {
    delete h;
  }

  for (auto h : mRMSperMod) {
    delete h;
  }

  for (auto h : mMEANperMod) {
    delete h;
  }

  for (auto h : mMAXperMod) {
    delete h;
  }

  for (auto h : mMINperMod) {
    delete h;
  }
}
void RawTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  using infoCONTEXT = AliceO2::InfoLogger::InfoLoggerContext;
  infoCONTEXT context;
  context.setField(infoCONTEXT::FieldName::Facility, "QC");
  context.setField(infoCONTEXT::FieldName::System, "QC");
  context.setField(infoCONTEXT::FieldName::Detector, "PHS");
  QcInfoLogger::GetInstance().setContext(context);
  QcInfoLogger::GetInstance() << "initialize RawTask" << AliceO2::InfoLogger::InfoLogger::endm;

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
    QcInfoLogger::GetInstance() << "Custom parameter - myOwnKey : " << param->second << AliceO2::InfoLogger::InfoLogger::endm;
  }

  //  mMappings = std::unique_ptr<o2::phos::MappingHandler>(new o2::phos::MappingHandler); //initialize the unique pointer to Mapper

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

  mTotalDataVolume = new TH1F("TotalDataVolume", "Total data volume", 1, 0.5, 1.5);
  mTotalDataVolume->GetXaxis()->SetTitle("MonitorData");
  mTotalDataVolume->GetYaxis()->SetTitle("Total data volume (Byte)");
  getObjectsManager()->startPublishing(mTotalDataVolume);

  // PHOS related histograms
  mPayloadSizePerDDL = new TH2F("PayloadSizePerDDL", "PayloadSizePerDDL", 20, 0, 20, 100, 0, 1);
  mPayloadSizePerDDL->GetXaxis()->SetTitle("ddl");
  mPayloadSizePerDDL->GetYaxis()->SetTitle("PayloadSize");
  getObjectsManager()->startPublishing(mPayloadSizePerDDL);

  mHistogram = new TH1F("PayloadSize", "PayloadSize", 20, 0, 60000000); //
  mHistogram->GetXaxis()->SetTitle("bytes");
  mHistogram->GetYaxis()->SetTitle("Counts");
  getObjectsManager()->startPublishing(mHistogram);

  mErrorTypeAltro = new TH2F("ErrorTypePerDDL", "ErrorTypePerDDL", 20, 0, 20, 8, 0, 8);
  mErrorTypeAltro->GetXaxis()->SetTitle("Mod");
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

  //histos per Mod
  for (short i = 0; i < mNmod; i++) {
    mRawAmplitudePHOS[i] = new TH1F(Form("RawAmplitudePHOS_sm%d", i), Form(" RawAmplitudePHOS%d", i), 100, 0., 100.);
    mRawAmplitudePHOS[i]->GetXaxis()->SetTitle("Raw Amplitude");
    mRawAmplitudePHOS[i]->GetYaxis()->SetTitle("Counts");
    getObjectsManager()->startPublishing(mRawAmplitudePHOS[i]);

    mRawAmplMaxPHOS[i] = new TH1F(Form("RawAmplMaxPHOS_sm%d", i), Form(" RawAmpMaxPHOS%d", i), 100, 0., 100.);
    mRawAmplMaxPHOS[i]->GetXaxis()->SetTitle("Max Raw Amplitude");
    mRawAmplMaxPHOS[i]->GetYaxis()->SetTitle("Counts");
    getObjectsManager()->startPublishing(mRawAmplMaxPHOS[i]);

    mRawAmplMinPHOS[i] = new TH1F(Form("RawAmplMinPHOS_sm%d", i), Form(" RawAmpMinPHOS%d", i), 100, 0., 100.);
    mRawAmplMinPHOS[i]->GetXaxis()->SetTitle("Min Raw Amplitude");
    mRawAmplMinPHOS[i]->GetYaxis()->SetTitle("Counts");
    getObjectsManager()->startPublishing(mRawAmplMinPHOS[i]);

    mRMSperMod[i] = new TH2F(Form("RMSADCperMod%d", i), Form("RMSperMod%d", i), 48, 0, 48, 24, 0, 24);
    mRMSperMod[i]->GetXaxis()->SetTitle("col");
    mRMSperMod[i]->GetYaxis()->SetTitle("row");
    getObjectsManager()->startPublishing(mRMSperMod[i]);

    mMEANperMod[i] = new TH2F(Form("MeanADCperMod%d", i), Form("MeanADCperMod%d", i), 48, 0, 48, 24, 0, 24);
    mMEANperMod[i]->GetXaxis()->SetTitle("col");
    mMEANperMod[i]->GetYaxis()->SetTitle("row");
    getObjectsManager()->startPublishing(mMEANperMod[i]);

    mMAXperMod[i] = new TH2F(Form("MaxADCperMod%d", i), Form("MaxADCperMod%d", i), 48, 0, 47, 24, 0, 23);
    mMAXperMod[i]->GetXaxis()->SetTitle("col");
    mMAXperMod[i]->GetYaxis()->SetTitle("row");
    getObjectsManager()->startPublishing(mMAXperMod[i]);

    mMINperMod[i] = new TH2F(Form("MinADCperMod%d", i), Form("MinADCperMod%d", i), 48, 0, 47, 24, 0, 23);
    mMINperMod[i]->GetXaxis()->SetTitle("col");
    mMINperMod[i]->GetYaxis()->SetTitle("row");
    getObjectsManager()->startPublishing(mMINperMod[i]);
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

  //  using CHTYP = o2::phos::ChannelType_t;

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
      QcInfoLogger::GetInstance() << QcInfoLogger::Debug << " PHOS Reading Payload size: " << header->payloadSize << " for " << header->dataOrigin << AliceO2::InfoLogger::InfoLogger::endm;

      //fill the histogram with payload sizes
      mHistogram->Fill(header->payloadSize);
      mTotalDataVolume->Fill(1., header->payloadSize);

      // // try decoding payload
      // o2::phos::RawReaderMemory<o2::header::RAWDataHeaderV4> rawreader(gsl::span(input.payload, header->payloadSize));
      // uint64_t currentTrigger(0);
      // bool first = true; //for the first event
      // short int maxADCMod[20];
      // short int minADCMod[20];
      // while (rawreader.hasNext()) {
      //   QcInfoLogger::GetInstance() << QcInfoLogger::Debug << " Processing page " << mNumberOfPages << AliceO2::InfoLogger::InfoLogger::endm;
      //   mNumberOfPages++;
      //   nPagesMessage++;
      //   mPageCounter->Fill(1);
      //   rawreader.next();
      //   auto payLoadSize = rawreader.getPayloadSize(); //payloadsize in byte;

      //   auto headerR = rawreader.getRawHeader();
      //   mPayloadSizePerDDL->Fill(headerR.feeId, payLoadSize / 1024.);

      //   //fill histograms with max ADC for each supermodules and reset cache
      //   if (!first) {                               // check if it is the first event in the payload
      //     if (headerR.triggerBC > currentTrigger) { //new event
      //       for (int sm = 0; sm < 20; sm++) {
      //         mRawAmplitudePHOS[sm]->Fill(maxADCMod[sm]);
      //         maxADCMod[sm] = 0;
      //         //initialize
      //         minADCMod[sm] = SHRT_MAX;
      //       } //sm loop
      //       currentTrigger = headerR.triggerBC;
      //     }      //new event
      //   } else { //first
      //     currentTrigger = headerR.triggerBC;
      //     first = false;
      //   }
      //   if (headerR.feeId > 40)
      //     continue;                                                      //skip STU ddl
      //   o2::phos::AltroDecoder<decltype(rawreader)> decoder(rawreader); //(atrodecoder in Detectors/Emcal/reconstruction/src)
      //   //check the words of the payload exception in altrodecoder
      //   try {
      //     decoder.decode();
      //   } catch (AltroDecoderError& e) {
      //     std::stringstream errormessage;
      //     using AltroErrType = o2::phos::AltroDecoderError::ErrorType_t;
      //     int errornum = -1;
      //     switch (e.getErrorType()) {
      //       case AltroErrType::RCU_TRAILER_ERROR:
      //         errornum = 0;
      //         errormessage << " RCU Trailer Error ";
      //         break;
      //       case AltroErrType::RCU_VERSION_ERROR:
      //         errornum = 1;
      //         errormessage << " RCU Version Error ";
      //         break;
      //       case AltroErrType::RCU_TRAILER_SIZE_ERROR:
      //         errornum = 2;
      //         errormessage << " RCU Trailer Size Error ";
      //         break;
      //       case AltroErrType::ALTRO_BUNCH_HEADER_ERROR:
      //         errornum = 3;
      //         errormessage << " ALTRO Bunch Header Error ";
      //         break;
      //       case AltroErrType::ALTRO_BUNCH_LENGTH_ERROR:
      //         errornum = 4;
      //         errormessage << " ALTRO Bunch Length Error ";
      //         break;
      //       case AltroErrType::ALTRO_PAYLOAD_ERROR:
      //         errornum = 5;
      //         errormessage << " ALTRO Payload Error ";
      //         break;
      //       case AltroErrType::ALTRO_MAPPING_ERROR:
      //         errornum = 6;
      //         errormessage << " ALTRO Mapping Error ";
      //         break;
      //       case AltroErrType::CHANNEL_ERROR:
      //         errornum = 7;
      //         errormessage << " Channel Error ";
      //         break;
      //       default:
      //         break;
      //     }
      //     errormessage << " in Supermodule " << headerR.feeId;
      //     QcInfoLogger::GetInstance() << QcInfoLogger::Error << " PHOS raw task: " << errormessage.str() << AliceO2::InfoLogger::InfoLogger::endm;
      //     //fill histograms  with error types
      //     mErrorTypeAltro->Fill(headerR.feeId, errornum);
      //     continue;
      //   }
      //   int j = headerR.feeId / 2; //Mod id
      //   auto& mapping = mMappings->getMappingForDDL(headerR.feeId);
      //   int col;

      //   int row;

      //   for (auto& chan : decoder.getChannels()) {
      //     col = mapping.getColumn(chan.getHardwareAddress());
      //     row = mapping.getRow(chan.getHardwareAddress());
      //     //exclude LED Mon, TRU
      //     auto chType = mapping.getChannelType(chan.getHardwareAddress());
      //     if (chType == CHTYP::LEDMON || chType == CHTYP::TRU)
      //       continue;

      //     Short_t maxADC = 0;
      //     Short_t minADC = SHRT_MAX;
      //     Double_t meanADC = 0;
      //     Double_t rmsADC = 0;
      //     for (auto& bunch : chan.getBunches()) {
      //       auto adcs = bunch.getADC();

      //       auto maxADCbunch = *max_element(adcs.begin(), adcs.end());
      //       if (maxADCbunch > maxADC)
      //         maxADC = maxADCbunch;
      //       mRawAmplMaxPHOS[j]->Fill(maxADCbunch); // max for each cell

      //       auto minADCbunch = *min_element(adcs.begin(), adcs.end());
      //       if (minADCbunch < minADC)
      //         minADC = minADCbunch;
      //       mRawAmplMinPHOS[j]->Fill(minADCbunch); // min for each cell

      //       meanADC = TMath::Mean(adcs.begin(), adcs.end());
      //       rmsADC = TMath::RMS(adcs.begin(), adcs.end());
      //       mRMSperMod[j]->Fill(col, row, rmsADC);
      //       mMEANperMod[j]->Fill(col, row, meanADC);
      //     }
      //     if (maxADC > maxADCMod[j])
      //       maxADCMod[j] = maxADC;
      //     mMAXperMod[j]->Fill(col, row, maxADC);

      //     if (minADC < minADCMod[j])
      //       minADCMod[j] = minADC;
      //     mMINperMod[j]->Fill(col, row, minADC);

      //   } //channels
      // }   //new page
    } //header
  }   //inputs
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
  for (short i = 0; i < mNmod; i++) {
    mRawAmplitudePHOS[i]->Reset();
    mRawAmplMaxPHOS[i]->Reset();
    mRawAmplMinPHOS[i]->Reset();
    mRMSperMod[i]->Reset();
    mMEANperMod[i]->Reset();
    mMAXperMod[i]->Reset();
    mMINperMod[i]->Reset();
  }
  mPayloadSizePerDDL->Reset();
  mHistogram->Reset();
  mErrorTypeAltro->Reset();
}
} // namespace o2::quality_control_modules::phos
