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
  if (mErrorTypeAltro) {
    delete mErrorTypeAltro;
  }

  for (auto h : mRawAmplitudeEMCAL) {
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
}

void RawTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  QcInfoLogger::GetInstance() << "initialize RawTask" << AliceO2::InfoLogger::InfoLogger::endm;

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
    QcInfoLogger::GetInstance() << "Custom parameter - myOwnKey : " << param->second << AliceO2::InfoLogger::InfoLogger::endm;
  }

  mMappings = std::unique_ptr<o2::emcal::MappingHandler>(new o2::emcal::MappingHandler); //initialize the unique pointer to Mapper

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
  getObjectsManager()->startPublishing(mErrorTypeAltro);

  //histos per SM
  for (Int_t i = 0; i < 20; i++) {
    mRawAmplitudeEMCAL[i] = new TH1F(Form("RawAmplidutdeEMCAL_sm%d", i), Form(" RawAmplitudeEMCAL%d", i), 100, 0., 100.);
    mRawAmplitudeEMCAL[i]->GetXaxis()->SetTitle("Raw Amplitude");
    mRawAmplitudeEMCAL[i]->GetYaxis()->SetTitle("Counts");
    getObjectsManager()->startPublishing(mRawAmplitudeEMCAL[i]);

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
  // Some examples:

  // 1. In a loop
  for (auto&& input : ctx.inputs()) {
    // get message header
    if (input.header != nullptr && input.payload != nullptr) {
      const auto* header = header::get<header::DataHeader*>(input.header);
      // get payload of a specific input, which is a char array.
      // const char* payload = input.payload;

      //fill the histogram with payload sizes
      mHistogram->Fill(header->payloadSize);

      // try decoding payload
      o2::emcal::RawReaderMemory<o2::header::RAWDataHeaderV4> rawreader(gsl::span(input.payload, header->payloadSize));
      uint64_t currentTrigger(0);
      bool first = true; //for the first event
      short int maxADCSM[20];
      while (rawreader.hasNext()) {
        rawreader.next();
        auto payLoad = rawreader.getPayload();
        auto payLoadSizeTest = payLoad.getPayloadWords().size() * sizeof(uint32_t); //payloadsize in byte

        auto headerR = rawreader.getRawHeader();
        mPayloadSizePerDDL->Fill(headerR.feeId, payLoadSizeTest / 1024.);

        //fill histograms with max ADC for each supermodules and reset cache
        if (!first) {                               // check if it is the first event in the payload
          if (headerR.triggerBC > currentTrigger) { //new event
            for (int sm = 0; sm < 20; sm++) {
              mRawAmplitudeEMCAL[sm]->Fill(maxADCSM[sm]);
              maxADCSM[sm] = 0;
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
        //add try block to check the words of the payload exception in  altrodecoder
        try {
          decoder.decode();
        } catch (AltroDecoderError& e) {
          // AliceO2::InfoLogger::InfoLogger logger;
          // logger << e.what();
          // and add checker for calling oncall
          using AltroErrType = o2::emcal::AltroDecoderError::ErrorType_t;
          int errornum = -1;
          switch (e.getErrorType()) {
            case AltroErrType::RCU_TRAILER_ERROR:
              errornum = 0;
              break;
            case AltroErrType::RCU_VERSION_ERROR:
              errornum = 1;
              break;
            case AltroErrType::RCU_TRAILER_SIZE_ERROR:
              errornum = 2;
              break;
            case AltroErrType::ALTRO_BUNCH_HEADER_ERROR:
              errornum = 3;
              break;
            case AltroErrType::ALTRO_BUNCH_LENGTH_ERROR:
              errornum = 4;
              break;
            case AltroErrType::ALTRO_PAYLOAD_ERROR:
              errornum = 5;
              break;
            case AltroErrType::ALTRO_MAPPING_ERROR:
              errornum = 6;
              break;
            case AltroErrType::CHANNEL_ERROR:
              errornum = 7;
              break;
            default:
              break;
          }
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

          Float_t maxADC;
          Double_t meanADC;
          Double_t rmsADC;
          for (auto& bunch : chan.getBunches()) {
            auto adcs = bunch.getADC();
            maxADC = *max_element(adcs.begin(), adcs.end());
            meanADC = TMath::Mean(adcs.begin(), adcs.end());
            rmsADC = TMath::RMS(adcs.begin(), adcs.end());
            mRMSperSM[j]->Fill(col, row, rmsADC);
            mMEANperSM[j]->Fill(col, row, meanADC);
          }
          if (maxADC > maxADCSM[j])
            maxADCSM[j] = maxADC;
          mMAXperSM[j]->Fill(col, row, maxADC);
        } //channels
      }   //new page
    }     //header
  }       //inputs
} //function monitor data

void RawTask::endOfCycle()
{
  QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void RawTask::endOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void RawTask::reset()
{
  // clean all the monitor objects here

  QcInfoLogger::GetInstance() << "Resetting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;
  mHistogram->Reset();
  for (Int_t i = 0; i < 20; i++) {
    mRawAmplitudeEMCAL[i]->Reset();
    mRMSperSM[i]->Reset();
    mMEANperSM[i]->Reset();
    mMAXperSM[i]->Reset();
  }
  mPayloadSizePerDDL->Reset();
  mHistogram->Reset();
  mErrorTypeAltro->Reset();
}
} // namespace o2::quality_control_modules::emcal
