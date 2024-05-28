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
#include <TCanvas.h>
#include <TH1.h>
#include <TProfile2D.h>
#include <TMath.h>
#include <climits>
#include <cfloat>

#include "QualityControl/QcInfoLogger.h"
#include "DetectorsRaw/RawHeaderStream.h"
#include "DetectorsRaw/RDHUtils.h"
#include "EMCAL/RawTask.h"
#include "Headers/RAWDataHeader.h"
#include "EMCALBase/Geometry.h"
#include "EMCALReconstruction/AltroDecoder.h"
#include "EMCALReconstruction/RawDecodingError.h"
#include "EMCALReconstruction/RawReaderMemory.h"
#include <Framework/ConcreteDataMatcher.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/InputRecord.h>
#include <Framework/DataRefUtils.h>
#include <Headers/DataHeader.h>
#include <CommonConstants/Triggers.h>

using namespace o2::emcal;

namespace o2::quality_control_modules::emcal
{

RawTask::~RawTask()
{
  if (mPayloadSizePerDDL) {
    delete mPayloadSizePerDDL;
  }
  if (mPayloadSizePerDDL_1D) {
    delete mPayloadSizePerDDL_1D;
  }
  if (mPayloadSizeTFPerDDL) {
    delete mPayloadSizeTFPerDDL;
  }
  if (mPayloadSizeTFPerDDL_1D) {
    delete mPayloadSizeTFPerDDL_1D;
  }
  if (mMessageCounter) {
    delete mMessageCounter;
  }
  if (mNumberOfPagesPerMessage) {
    delete mNumberOfPagesPerMessage;
  }
  if (mNumberOfSuperpagesPerMessage) {
    delete mNumberOfSuperpagesPerMessage;
  }
  if (mTotalDataVolume) {
    delete mTotalDataVolume;
  }
  if (mErrorTypeAltro) {
    delete mErrorTypeAltro;
  }
  if (mNbunchPerChan) {
    delete mNbunchPerChan;
  }
  if (mNofADCsamples) {
    delete mNofADCsamples;
  }
  if (mADCsize) {
    delete mADCsize;
  }
  if (mFECmaxCountperSM) {
    delete mFECmaxCountperSM;
  }
  if (mFECmaxIDperSM) {
    delete mFECmaxIDperSM;
  }
  if (mTFerrorCounter) {
    delete mTFerrorCounter;
  }

  for (auto& histos : mRMSBunchADCRCFull) {
    delete histos.second;
  }

  for (auto& histos : mMeanBunchADCRCFull) {
    delete histos.second;
  }

  for (auto& histos : mMaxChannelADCRCFull) {
    delete histos.second;
  }

  for (auto& histos : mMinChannelADCRCFull) {
    delete histos.second;
  }

  for (auto& histos : mBunchMinRawAmpSM) {
    delete histos.second;
  }
  for (auto& histos : mBunchMinRawAmpFEC) {
    delete histos.second;
  }
  for (auto& histos : mBunchMaxRawAmpSM) {
    delete histos.second;
  }
  for (auto& histos : mBunchMaxRawAmpFEC) {
    delete histos.second;
  }
  for (auto& histos : mSMMinRawAmpSM) {
    delete histos.second;
  }
  for (auto& histos : mSMMaxRawAmpSM) {
    delete histos.second;
  }
}

void RawTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  using infoCONTEXT = AliceO2::InfoLogger::InfoLoggerContext;
  infoCONTEXT context;
  context.setField(infoCONTEXT::FieldName::Facility, "QC");
  context.setField(infoCONTEXT::FieldName::System, "QC");
  context.setField(infoCONTEXT::FieldName::Detector, "EMC");
  QcInfoLogger::GetInfoLogger().setContext(context);
  ILOG(Debug, Devel) << "initialize RawTask" << ENDM;

  // initialize geometry
  if (!mGeometry)
    mGeometry = o2::emcal::Geometry::GetInstanceFromRunNumber(300000);

  mMappings = std::unique_ptr<o2::emcal::MappingHandler>(new o2::emcal::MappingHandler); // initialize the unique pointer to Mapper

  // Statistics histograms
  mMessageCounter = new TH1F("NumberOfMessages", "Number of messages", 3, 0.5, 3.5);
  //  mMessageCounter->GetXaxis()->SetTitle("MonitorData");
  mMessageCounter->GetXaxis()->SetBinLabel(1, "Number of message/time");
  mMessageCounter->GetXaxis()->SetBinLabel(2, "Number of super pages/time");
  mMessageCounter->GetXaxis()->SetBinLabel(3, "Number of pages/time");
  mMessageCounter->GetYaxis()->SetTitle("Number of objects");
  getObjectsManager()->startPublishing(mMessageCounter);

  mTFerrorCounter = new TH1F("NumberOfTFerror", "Number of TFbuilder errors", 2, 0.5, 2.5);
  mTFerrorCounter->GetYaxis()->SetTitle("Time Frame Builder Error");
  mTFerrorCounter->GetXaxis()->SetBinLabel(1, "empty");
  mTFerrorCounter->GetXaxis()->SetBinLabel(2, "filled");
  getObjectsManager()->startPublishing(mTFerrorCounter);

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
  mPayloadSizePerDDL = new TH2F("PayloadSizePerDDL", "Payload Size / Event", 40, 0, 40, 200, 0, 20);
  mPayloadSizePerDDL->GetXaxis()->SetTitle("DDL");
  mPayloadSizePerDDL->GetYaxis()->SetTitle("Payload Size / Event (kB)");
  mPayloadSizePerDDL->SetStats(0);
  getObjectsManager()->startPublishing(mPayloadSizePerDDL);

  mPayloadSizePerDDL_1D = new TH1F("PayloadSizePerDDL_1D", "Accumulated Payload Size / Event", 40, 0, 40);
  mPayloadSizePerDDL_1D->GetXaxis()->SetTitle("DDL");
  mPayloadSizePerDDL_1D->GetYaxis()->SetTitle("Accumulated Payload Size / Event (kB)");
  mPayloadSizePerDDL_1D->SetStats(0);
  getObjectsManager()->startPublishing(mPayloadSizePerDDL_1D);

  mPayloadSizeTFPerDDL = new TH2F("PayloadSizeTFPerDDL", "Payload Size / TF", 40, 0, 40, 100, 0, 100);
  mPayloadSizeTFPerDDL->GetXaxis()->SetTitle("DDL");
  mPayloadSizeTFPerDDL->GetYaxis()->SetTitle("Payload Size / TF (kB)");
  mPayloadSizeTFPerDDL->SetStats(0);
  getObjectsManager()->startPublishing(mPayloadSizeTFPerDDL);

  mPayloadSizeTFPerDDL_1D = new TH1F("PayloadSizeTFPerDDL_1D", "Accumulated Payload Size / TF ", 40, 0, 40);
  mPayloadSizeTFPerDDL_1D->GetXaxis()->SetTitle("DDL");
  mPayloadSizeTFPerDDL_1D->GetYaxis()->SetTitle("Accumulated Payload Size / TF (kB)");
  mPayloadSizeTFPerDDL_1D->SetStats(0);
  getObjectsManager()->startPublishing(mPayloadSizeTFPerDDL_1D);

  mErrorTypeAltro = new TH2F("ErrorTypePerSM", "ErrorTypeForSM", 40, 0, 40, 10, 0, 10); // optional? OFFLINE
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
  mErrorTypeAltro->GetYaxis()->SetBinLabel(9, "Mapper HWAddress");
  mErrorTypeAltro->GetYaxis()->SetBinLabel(10, "Geometry InvalidCell");
  mErrorTypeAltro->SetStats(0);
  getObjectsManager()
    ->startPublishing(mErrorTypeAltro);

  mNbunchPerChan = new TH1F("NumberBunchPerChannel", "Number of bunches per channel", 4, -0.5, 3.5);
  mNbunchPerChan->GetXaxis()->SetTitle("# bunches per channels");
  mNbunchPerChan->SetStats(0);
  getObjectsManager()->startPublishing(mNbunchPerChan);

  mNofADCsamples = new TH1F("NumberOfADCPerChannel", "Number od ADC samples per channel", 15, -0.5, 14.5);
  mNofADCsamples->GetXaxis()->SetTitle("# of ADC sample per channels");
  mNofADCsamples->SetStats(0);
  getObjectsManager()->startPublishing(mNofADCsamples);

  mADCsize = new TH1F("ADCsizePerBunch", "ADCsizePerBunch", 15, -0.5, 14.5);
  mADCsize->GetXaxis()->SetTitle("ADC size per bunch");
  mADCsize->SetStats(0);
  getObjectsManager()->startPublishing(mADCsize);

  mFECmaxCountperSM = new TH2F("NumberOfChWithInput_perSM", "NumberOfChWithInput_perSM", 20, 0, 20, 40, 0, 40);
  mFECmaxCountperSM->GetXaxis()->SetTitle("SM");
  mFECmaxCountperSM->GetYaxis()->SetTitle("max channel count");
  mFECmaxCountperSM->SetStats(0);
  getObjectsManager()->startPublishing(mFECmaxCountperSM); // work on it, keep them

  mFECmaxIDperSM = new TH2F("FECidMaxChWithInput_perSM", "FECidMaxChWithInput_perSM", 20, 0, 20, 40, 0, 40);
  mFECmaxIDperSM->GetXaxis()->SetTitle("SM");
  mFECmaxIDperSM->GetYaxis()->SetTitle("FEC id");
  mFECmaxIDperSM->SetStats(0);
  getObjectsManager()->startPublishing(mFECmaxIDperSM); // work on it, keep them

  // histos per SM and Trigger
  EventType triggers[2] = { EventType::CAL_EVENT, EventType::PHYS_EVENT };
  TString histoStr[2] = { "CAL", "PHYS" };
  for (auto trg = 0; trg < 2; trg++) {

    TProfile2D* histosRawAmplRmsRC; // Filling EMCAL/DCAL
    TProfile2D* histosRawAmplMeanRC;
    TProfile2D* histosRawAmplMaxRC;
    TProfile2D* histosRawAmplMinRC;
    // EMCAL+DCAL histo
    histosRawAmplRmsRC = new TProfile2D(Form("RMSADC_EMCAL_%s", histoStr[trg].Data()), Form("Bunch ADC RMS (%s)", histoStr[trg].Data()), 96, -0.5, 95.5, 208, -0.5, 207.5);
    histosRawAmplRmsRC->GetXaxis()->SetTitle("col");
    histosRawAmplRmsRC->GetYaxis()->SetTitle("row");
    histosRawAmplRmsRC->SetStats(0);
    getObjectsManager()->startPublishing(histosRawAmplRmsRC); // markus/martin full emcal not so big

    histosRawAmplMeanRC = new TProfile2D(Form("MeanADC_EMCAL_%s", histoStr[trg].Data()), Form("Bunch ADC mean (%s)", histoStr[trg].Data()), 96, -0.5, 95.5, 208, -0.5, 207.5);
    histosRawAmplMeanRC->GetXaxis()->SetTitle("col");
    histosRawAmplMeanRC->GetYaxis()->SetTitle("row");
    getObjectsManager()->startPublishing(histosRawAmplMeanRC); // markus/martin full emcal not so big

    histosRawAmplMaxRC = new TProfile2D(Form("MaxADC_EMCAL_%s", histoStr[trg].Data()), Form("Channel ADC max (%s)", histoStr[trg].Data()), 96, -0.5, 95.5, 208, -0.5, 207.5);
    histosRawAmplMaxRC->GetXaxis()->SetTitle("col");
    histosRawAmplMaxRC->GetYaxis()->SetTitle("row");
    histosRawAmplMaxRC->SetStats(0);
    getObjectsManager()->startPublishing(histosRawAmplMaxRC); // markus/martin full emcal not so big

    histosRawAmplMinRC = new TProfile2D(Form("MinADC_EMCAL_%s", histoStr[trg].Data()), Form("Channel ADC min (%s)", histoStr[trg].Data()), 96, -0.5, 95.5, 208, -0.5, 207.5);
    histosRawAmplMinRC->GetXaxis()->SetTitle("col");
    histosRawAmplMinRC->GetYaxis()->SetTitle("raw");
    histosRawAmplMinRC->SetStats(0);
    getObjectsManager()->startPublishing(histosRawAmplMinRC); // markus/martin full emcal not so big

    TH2* histosBunchMinRawAmpSM = new TH2D(Form("BunchMinRawAmplitudeSM_%s", histoStr[trg].Data()), Form("Bunch min raw amplitude per supermodule (%s)", histoStr[trg].Data()), 100, 0., 100., 20, -0.5, 19.5);
    histosBunchMinRawAmpSM->GetXaxis()->SetTitle("Min raw amplitude (ADC)");
    histosBunchMinRawAmpSM->GetYaxis()->SetTitle("Supermodule ID");
    histosBunchMinRawAmpSM->SetStats(0);
    getObjectsManager()->startPublishing(histosBunchMinRawAmpSM);

    TH2* histosBunchMinRawAmpFEC = new TH2D(Form("BunchMinRawAmplitudeFEC_%s", histoStr[trg].Data()), Form("Bunch min raw amplitude per FEC (%s)", histoStr[trg].Data()), 100, 0., 100., 800, -0.5, 799.5);
    histosBunchMinRawAmpFEC->GetXaxis()->SetTitle("Min raw amplitude (ADC)");
    histosBunchMinRawAmpFEC->GetYaxis()->SetTitle("FEC ID");
    histosBunchMinRawAmpFEC->SetStats(0);
    getObjectsManager()->startPublishing(histosBunchMinRawAmpFEC);

    TH2* histosBunchMaxRawAmpSM = new TH2D(Form("BunchMaxRawAmplitudeSM_%s", histoStr[trg].Data()), Form("Bunch max raw amplitude per supermodule (%s)", histoStr[trg].Data()), 500, 0., 500., 20, -0.5, 19.5);
    histosBunchMaxRawAmpSM->GetXaxis()->SetTitle("Max raw amplitude (ADC)");
    histosBunchMaxRawAmpSM->GetYaxis()->SetTitle("Supermodule ID");
    histosBunchMaxRawAmpSM->SetStats(0);
    getObjectsManager()->startPublishing(histosBunchMaxRawAmpSM);

    TH2* histosBunchMaxRawAmpFEC = new TH2D(Form("BunchMaxRawAmplitudeFEC_%s", histoStr[trg].Data()), Form("Bunch max raw amplitude per FEC (%s)", histoStr[trg].Data()), 500, 0., 500., 800, -0.5, 799.5);
    histosBunchMaxRawAmpFEC->GetXaxis()->SetTitle("Max raw amplitude (ADC)");
    histosBunchMaxRawAmpFEC->GetYaxis()->SetTitle("FEC ID");
    histosBunchMaxRawAmpFEC->SetStats(0);
    getObjectsManager()->startPublishing(histosBunchMaxRawAmpFEC);

    TH2* histosSMMinRawAmpSM = new TH2D(Form("SMMinRawAmplitudeSM_%s", histoStr[trg].Data()), Form("Min SM raw amplitude per supermodule (%s)", histoStr[trg].Data()), 100, 0., 100., 20, -0.5, 19.5);
    histosSMMinRawAmpSM->GetXaxis()->SetTitle("Min raw amplitude (ADC)");
    histosSMMinRawAmpSM->GetYaxis()->SetTitle("Supermodule ID");
    histosSMMinRawAmpSM->SetStats(0);
    getObjectsManager()->startPublishing(histosSMMinRawAmpSM);

    TH2* histosSMMaxRawAmpSM = new TH2D(Form("SMMaxRawAmplitudeSM_%s", histoStr[trg].Data()), Form("Max SM raw amplitude per supermodule (%s)", histoStr[trg].Data()), 500, 0., 500., 20, -0.5, 19.5);
    histosSMMaxRawAmpSM->GetXaxis()->SetTitle("Max raw amplitude (ADC)");
    histosSMMaxRawAmpSM->GetYaxis()->SetTitle("Supermodule ID");
    histosSMMaxRawAmpSM->SetStats(0);
    getObjectsManager()->startPublishing(histosSMMaxRawAmpSM);

    mRMSBunchADCRCFull[triggers[trg]] = histosRawAmplRmsRC;
    mMeanBunchADCRCFull[triggers[trg]] = histosRawAmplMeanRC;
    mMaxChannelADCRCFull[triggers[trg]] = histosRawAmplMaxRC;
    mMinChannelADCRCFull[triggers[trg]] = histosRawAmplMinRC;

    mBunchMinRawAmpSM[triggers[trg]] = histosBunchMinRawAmpSM;
    mBunchMinRawAmpFEC[triggers[trg]] = histosBunchMinRawAmpFEC;
    mBunchMaxRawAmpSM[triggers[trg]] = histosBunchMaxRawAmpSM;
    mBunchMaxRawAmpFEC[triggers[trg]] = histosBunchMaxRawAmpFEC;

    mSMMinRawAmpSM[triggers[trg]] = histosSMMinRawAmpSM;
    mSMMaxRawAmpSM[triggers[trg]] = histosSMMaxRawAmpSM;

  } // loop trigger case
}

void RawTask::startOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;
  reset();
}

void RawTask::startOfCycle()
{
  ILOG(Debug, Support) << "startOfCycle" << ENDM;
}

void RawTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  using CHTYP = o2::emcal::ChannelType_t;

  // The type DataOrigin allows only conversion of char arrays with size 4, not char *, therefore
  // the origin string has to be converted manually to the char array and checked for length.
  if (mDataOrigin.size() > 4) {
    ILOG(Error, Support) << "No valid data origin" << mDataOrigin << ", cannot process" << ENDM;
    return;
  }
  char dataOrigin[4];
  strcpy(dataOrigin, mDataOrigin.data());

  if (isLostTimeframe(ctx)) {
    mTFerrorCounter->Fill(1);
    return;
  }
  mTFerrorCounter->Fill(2);

  Int_t nPagesMessage = 0, nSuperpagesMessage = 0;
  ILOG(Debug, Support) << " Processing message " << mNumberOfMessages << ENDM;
  mNumberOfMessages++;
  mMessageCounter->Fill(0); // for expert fill bin 1 with number of messages

  const int NUMBERSM = 20;
  const int NFEESM = 40; // number of fee per sm

  double thresholdMinADCocc = 3,
         thresholdMaxADCocc = 15;

  std::unordered_map<RawEventType, std::array<int, 20>, RawEventTypeHash> maxADCSM, minADCSM;
  std::unordered_map<RawEventType, std::array<std::array<int, NFEESM>, NUMBERSM>, RawEventTypeHash> fecMaxPayload;

  // Accept only descriptor RAWDATA, discard FLP/SUBTIMEFRAME
  auto posReadout = ctx.inputs().getPos("readout");
  auto nslots = ctx.inputs().getNofParts(posReadout);
  for (decltype(nslots) islot = 0; islot < nslots; islot++) {
    auto rawData = ctx.inputs().getByPos(posReadout, islot);
    // get message header
    if (rawData.header != nullptr && rawData.payload != nullptr) {
      const auto* header = o2::framework::DataRefUtils::getHeader<header::DataHeader*>(rawData);
      const auto payloadSize = o2::framework::DataRefUtils::getPayloadSize(rawData);
      // get payload of a specific input, which is a char array.
      ILOG(Debug, Support) << "Processing superpage " << mNumberOfSuperpages << ENDM;
      mNumberOfSuperpages++;
      nSuperpagesMessage++;
      mMessageCounter->Fill(1); // fill bin 2 with mSuperpageCounter
      ILOG(Debug, Support) << " EMCAL Reading Payload size: " << payloadSize << " for " << header->dataOrigin << ENDM;

      // fill the histogram with payload sizes
      mTotalDataVolume->Fill(1., payloadSize); // for expert

      // Skip SOX headers
      auto rdhblock = reinterpret_cast<const o2::header::RDHAny*>(rawData.payload); //
      if (o2::raw::RDHUtils::getHeaderSize(rdhblock) == static_cast<int>(payloadSize)) {
        continue;
      }
      mPayloadSizeTFPerDDL->Fill(o2::raw::RDHUtils::getFEEID(rdhblock), payloadSize / 1024.); // PayLoad size per TimeFrame for shifter
      mPayloadSizeTFPerDDL_1D->Fill(o2::raw::RDHUtils::getFEEID(rdhblock), payloadSize / 1024.);

      // try decoding payload
      o2::emcal::RawReaderMemory rawreader(ctx.inputs().get<gsl::span<char>>(rawData));

      while (rawreader.hasNext()) {

        ILOG(Debug, Support) << " Processing page " << mNumberOfPages << ENDM;
        mNumberOfPages++;
        nPagesMessage++;
        mMessageCounter->Fill(2); // fill bin 3 with PageCounter
        try {
          rawreader.next();
        } catch (RawDecodingError& e) {
          // Skip page in case of page decoding errors
          // For corrupted RDHs the determination of the next page
          // will fail, leading to an infinity loop
          break;
        }
        auto rawSize = rawreader.getPayloadSize(); // payloadsize in byte;

        auto rdh = rawreader.getRawHeader();
        auto feeID = o2::raw::RDHUtils::getFEEID(rdh);

        if (feeID > 40)
          continue; // skip STU ddl

        o2::InteractionRecord triggerIR{ o2::raw::RDHUtils::getTriggerBC(rdh), o2::raw::RDHUtils::getTriggerOrbit(rdh) };
        RawEventType evIndex{ triggerIR, o2::raw::RDHUtils::getTriggerType(rdh) };

        // trigger type
        auto triggertype = o2::raw::RDHUtils::getTriggerType(rdh);
        bool isPhysTrigger = triggertype & o2::trigger::PhT, isCalibTrigger = triggertype & o2::trigger::Cal;
        if (isPhysTrigger) {
          mPayloadSizePerDDL->Fill(feeID, rawSize / 1024.);    // for shifter
          mPayloadSizePerDDL_1D->Fill(feeID, rawSize / 1024.); // for shifter
        }
        if (!(isPhysTrigger || isCalibTrigger)) {
          ILOG(Error, Support) << " Unmonitored trigger class requested " << ENDM;
          continue;
        }

        // Needs separate maps for the two trigger classes
        auto fecMaxChannelsEvent = fecMaxPayload.find(evIndex);
        if (fecMaxChannelsEvent == fecMaxPayload.end()) {
          std::array<std::array<int, NFEESM>, NUMBERSM> fecMaxCh;
          for (auto ism = 0; ism < NUMBERSM; ism++) {
            std::fill(fecMaxCh[ism].begin(), fecMaxCh[ism].end(), 0);
          }
          fecMaxChannelsEvent = (fecMaxPayload.insert({ evIndex, fecMaxCh })).first;
        }

        auto maxADCSMEvent = maxADCSM.find(evIndex);
        if (maxADCSMEvent == maxADCSM.end()) { // No entry found for key triggerIR
          std::array<int, NUMBERSM> maxadc;
          memset(maxadc.data(), 0, maxadc.size());
          maxADCSMEvent = (maxADCSM.insert({ evIndex, maxadc })).first;
        }
        auto minADCSMEvent = minADCSM.find(evIndex);
        if (minADCSMEvent == minADCSM.end()) { // No entry found for key triggerIR
          std::array<int, NUMBERSM> minadc;
          memset(minadc.data(), 0, minadc.size());
          for (auto ism = 0; ism < NUMBERSM; ism++)
            minadc[ism] = SHRT_MAX;
          minADCSMEvent = (minADCSM.insert({ evIndex, minadc })).first;
        }

        o2::emcal::AltroDecoder decoder(rawreader);
        // check the words of the payload exception in altrodecoder
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
          ILOG(Error, Support) << " EMCAL raw task: " << errormessage.str() << ENDM;
          // fill histograms  with error types
          mErrorTypeAltro->Fill(feeID, errornum); // for shifter
          continue;
        }
        int supermoduleID = feeID / 2; // SM id
        auto& mapping = mMappings->getMappingForDDL(feeID);

        auto fecIndex = 0;
        auto branchIndex = 0;
        auto fecID = 0;

        for (auto& chan : decoder.getChannels()) {
          // Row and column in online format, must be remapped to offline indexing,
          // otherwise it leads to invalid cell IDs
          int colOnline, rowOnline;
          o2::emcal::ChannelType_t chType;
          try {
            colOnline = mapping.getColumn(chan.getHardwareAddress());
            rowOnline = mapping.getRow(chan.getHardwareAddress());
            chType = mapping.getChannelType(chan.getHardwareAddress());
          } catch (o2::emcal::Mapper::AddressNotFoundException& err) {
            ILOG(Error, Support) << "DDL " << feeID << ": " << err.what() << ENDM;
            mErrorTypeAltro->Fill(feeID, 8);
            continue;
          }
          // exclude LED Mon, TRU
          if (chType == CHTYP::LEDMON || chType == CHTYP::TRU)
            continue;

          int globRow(-1), globCol(-1);
          try {
            auto [row, col] = mGeometry->ShiftOnlineToOfflineCellIndexes(supermoduleID, rowOnline, colOnline);
            // tower absolute ID
            auto cellID = mGeometry->GetAbsCellIdFromCellIndexes(supermoduleID, row, col);
            if (cellID > 17664) {
              mErrorTypeAltro->Fill(feeID, 9);
              continue;
            }
            // position in the EMCAL
            auto [globRowTmp, globColTmp] = mGeometry->GlobalRowColFromIndex(cellID);
            globRow = globRowTmp;
            globCol = globColTmp;
          } catch (o2::emcal::InvalidCellIDException& e) {
            mErrorTypeAltro->Fill(feeID, 9);
            continue;
          }

          fecIndex = chan.getFECIndex();
          branchIndex = chan.getBranchIndex();
          fecID = mMappings->getFEEForChannelInDDL(feeID, fecIndex, branchIndex);
          auto globalFecID = supermoduleID * NFEESM + fecID;
          fecMaxChannelsEvent->second[supermoduleID][fecID]++;

          Short_t maxADC = 0;
          Short_t minADC = SHRT_MAX;
          Double_t meanADC = 0;
          Double_t rmsADC = 0;
          EventType evtype = isPhysTrigger ? EventType::PHYS_EVENT : EventType::CAL_EVENT;

          mNbunchPerChan->Fill(chan.getBunches().size()); //(1 histo for EMCAL-526).//1, if high rate --> pile up.

          int numberOfADCsamples = 0;
          for (auto& bunch : chan.getBunches()) {
            const auto& adcs = bunch.getADC();
            numberOfADCsamples += adcs.size();
            mADCsize->Fill(adcs.size());

            if (adcs.size() == 0) {
              // protection against 0 bunch size (possible data corruption in bunch header)
              ILOG(Debug, Support) << "Bunch with length 0 detected" << ENDM;
              continue;
            }

            auto maxADCbunch = *max_element(adcs.begin(), adcs.end());
            if (maxADCbunch > maxADC)
              maxADC = maxADCbunch;
            mBunchMaxRawAmpFEC[evtype]->Fill(maxADCbunch, globalFecID);
            mBunchMaxRawAmpSM[evtype]->Fill(maxADCbunch, supermoduleID); // max for each cell --> for for expert only

            auto minADCbunch = *min_element(adcs.begin(), adcs.end());
            if (minADCbunch < minADC)
              minADC = minADCbunch;
            mBunchMinRawAmpFEC[evtype]->Fill(minADCbunch, globalFecID);
            mBunchMinRawAmpSM[evtype]->Fill(minADCbunch, supermoduleID); // min for each cell --> for for expert only

            meanADC = TMath::Mean(adcs.begin(), adcs.end());
            rmsADC = TMath::RMS(adcs.begin(), adcs.end());

            mRMSBunchADCRCFull[evtype]->Fill(globCol, globRow, rmsADC); // for  shifter
            // mRMSBunchADCRCSM[evtype][supermoduleID]->Fill(col, row, rmsADC); // no shifter

            mMeanBunchADCRCFull[evtype]->Fill(globCol, globRow, meanADC); // for shifter
            // mMeanBunchADCRCSM[evtype][supermoduleID]->Fill(col, row, meanADC); // no shifter
          }
          mNofADCsamples->Fill(numberOfADCsamples); // number of bunches per channel

          if (maxADC > maxADCSMEvent->second[supermoduleID])
            maxADCSMEvent->second[supermoduleID] = maxADC;

          // if (maxADC > thresholdMaxADCocc)
          // mMaxChannelADCRCSM[evtype][supermoduleID]->Fill(col, row, maxADC); //max col,row, per SM
          if (maxADC > thresholdMaxADCocc)
            mMaxChannelADCRCFull[evtype]->Fill(globCol, globRow, maxADC); // for shifter

          if (minADC < minADCSMEvent->second[supermoduleID]) {
            minADCSMEvent->second[supermoduleID] = minADC;
          }
          // if (minADC > thresholdMinADCocc)
          // mMinChannelADCRCSM[evtype][supermoduleID]->Fill(col, row, minADC); //min col,row, per SM
          if (minADC > thresholdMinADCocc)
            mMinChannelADCRCFull[evtype]->Fill(globCol, globRow, minADC); // for shifter
        }                                                                 // channels
      }                                                                   // new page
    }                                                                     // header
  }                                                                       // inputs
  mNumberOfPagesPerMessage->Fill(nPagesMessage);                          // for experts
  mNumberOfSuperpagesPerMessage->Fill(nSuperpagesMessage);

  // Fill histograms with cached values
  for (const auto& maxfec : fecMaxPayload) {
    auto triggertype = maxfec.first.mTrigger;
    bool isPhysTrigger = triggertype & o2::trigger::PhT;
    if (!isPhysTrigger)
      continue; // Only select phys event for max FEC, in case of calibration events the whole EMCAL gets the FEC pulse, so the payload size is roughly equal
    for (auto ism = 0; ism < NUMBERSM; ism++) {
      // Find maximum FEC in array of FECs
      int maxfecID(-1), maxfecCount(-1);
      auto& fecsSM = maxfec.second[ism];
      for (int ifec = 0; ifec < NFEESM; ifec++) {
        if (fecsSM[ifec] > maxfecCount) {
          maxfecCount = fecsSM[ifec];
          maxfecID = ifec;
        }
      }
      if (maxfecCount <= 0)
        continue; // Reject links on different FLP

      mFECmaxIDperSM->Fill(ism, maxfecID);       // filled as a funcion of SM (shifter)
      mFECmaxCountperSM->Fill(ism, maxfecCount); // filled as a function of SM (shifter)
    }
  }
  for (const auto& maxadc : maxADCSM) {
    auto triggertype = maxadc.first.mTrigger;
    bool isPhysTrigger = triggertype & o2::trigger::PhT;
    EventType evtype = isPhysTrigger ? EventType::PHYS_EVENT : EventType::CAL_EVENT;
    for (int ism = 0; ism < NUMBERSM; ism++) {
      if (maxadc.second[ism] == 0) {
        continue;
      }
      mSMMaxRawAmpSM[evtype]->Fill(maxadc.second[ism], ism);
    }
  }

  for (const auto& minadc : minADCSM) {
    auto triggertype = minadc.first.mTrigger;
    bool isPhysTrigger = triggertype & o2::trigger::PhT;
    EventType evtype = isPhysTrigger ? EventType::PHYS_EVENT : EventType::CAL_EVENT;
    for (int ism = 0; ism < NUMBERSM; ism++) {
      auto smminadc = minadc.second[ism];
      if (smminadc == SHRT_MAX) {
        continue;
      }
      mSMMinRawAmpSM[evtype]->Fill(smminadc, ism);
    }
  }
  // Same for other cached values
} // function monitor data

void RawTask::endOfCycle()
{
  ILOG(Debug, Support) << "endOfCycle" << ENDM;
}

void RawTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
  ILOG(Info, Support) << "Total amount of messages: " << mNumberOfMessages << ENDM;
  ILOG(Info, Support) << "Total amount of superpages: " << mNumberOfSuperpages << ", pages: " << mNumberOfPages << ENDM;
}

void RawTask::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Support) << "Resetting the histogram" << ENDM;
  EventType triggers[2] = { EventType::CAL_EVENT, EventType::PHYS_EVENT };

  for (const auto& trg : triggers) {
    mRMSBunchADCRCFull[trg]->Reset();
    mMeanBunchADCRCFull[trg]->Reset();
    mMaxChannelADCRCFull[trg]->Reset();
    mMinChannelADCRCFull[trg]->Reset();
    mBunchMinRawAmpSM[trg]->Reset();
    mBunchMinRawAmpFEC[trg]->Reset();
    mBunchMaxRawAmpSM[trg]->Reset();
    mBunchMaxRawAmpFEC[trg]->Reset();
    mSMMinRawAmpSM[trg]->Reset();
    mSMMaxRawAmpSM[trg]->Reset();
  }
  mMessageCounter->Reset();
  mNumberOfSuperpagesPerMessage->Reset();
  mNumberOfPagesPerMessage->Reset();
  mPayloadSizePerDDL->Reset();
  mPayloadSizePerDDL_1D->Reset();
  mPayloadSizeTFPerDDL->Reset();
  mPayloadSizeTFPerDDL_1D->Reset();
  mErrorTypeAltro->Reset();
  mNbunchPerChan->Reset();
  mNofADCsamples->Reset();
  mADCsize->Reset();
  mFECmaxIDperSM->Reset();
  mFECmaxCountperSM->Reset();
  mTFerrorCounter->Reset();
  mTotalDataVolume->Reset();
}

bool RawTask::isLostTimeframe(framework::ProcessingContext& ctx) const
{
  // direct data
  constexpr auto originEMC = header::gDataOriginEMC;
  o2::framework::InputSpec dummy{ "dummy",
                                  framework::ConcreteDataMatcher{ originEMC,
                                                                  header::gDataDescriptionRawData,
                                                                  0xDEADBEEF } };
  for (const auto& ref : o2::framework::InputRecordWalker(ctx.inputs(), { dummy })) {
    // auto posReadout = ctx.inputs().getPos("readout");
    // auto nslots = ctx.inputs().getNofParts(posReadout);
    // for (decltype(nslots) islot = 0; islot < nslots; islot++) {
    //   const auto& ref = ctx.inputs().getByPos(posReadout, islot);
    const auto dh = o2::framework::DataRefUtils::getHeader<o2::header::DataHeader*>(ref);
    const auto payloadSize = o2::framework::DataRefUtils::getPayloadSize(ref);
    // if (dh->subSpecification == 0xDEADBEEF) {
    if (payloadSize == 0) {
      return true;
      //  }
    }
  }
  // sampled data
  o2::framework::InputSpec dummyDS{ "dummyDS",
                                    framework::ConcreteDataMatcher{ "DS",
                                                                    "emcrawdata0",
                                                                    0xDEADBEEF } };
  for (const auto& ref : o2::framework::InputRecordWalker(ctx.inputs(), { dummyDS })) {
    // auto posReadout = ctx.inputs().getPos("readout");
    // auto nslots = ctx.inputs().getNofParts(posReadout);
    // for (decltype(nslots) islot = 0; islot < nslots; islot++) {
    //   const auto& ref = ctx.inputs().getByPos(posReadout, islot);
    const auto dh = o2::framework::DataRefUtils::getHeader<o2::header::DataHeader*>(ref);
    const auto payloadSize = o2::framework::DataRefUtils::getPayloadSize(ref);
    // if (dh->subSpecification == 0xDEADBEEF) {
    if (payloadSize == 0) {
      return true;
      //  }
    }
  }

  return false;
}

} // namespace o2::quality_control_modules::emcal
