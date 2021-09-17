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
#include <climits>
#include <cfloat>

#include "QualityControl/QcInfoLogger.h"
#include "DetectorsRaw/RDHUtils.h"
#include "EMCAL/RawTask.h"
#include "Headers/RAWDataHeader.h"
#include "EMCALBase/Geometry.h"
#include "EMCALReconstruction/AltroDecoder.h"
#include "EMCALReconstruction/RawReaderMemory.h"
#include "EMCALReconstruction/RawHeaderStream.h"
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
  if (mPayloadSize) {
    delete mPayloadSize;
  }
  if (mPayloadSizePerDDL) {
    delete mPayloadSizePerDDL;
  }
  if (mPayloadSizeTFPerDDL) {
    delete mPayloadSizeTFPerDDL;
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
  for (auto h : mFECmaxCount) {
    delete h;
  }
  for (auto h : mFECmaxID) {
    delete h;
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

  for (auto& histos : mRMS) {
    delete histos.second;
  }

  for (auto& histos : mMEAN) {
    delete histos.second;
  }

  for (auto& histos : mMAX) {
    delete histos.second;
  }

  for (auto& histos : mMIN) {
    delete histos.second;
  }
  for (auto& histos : mRawAmplMin_tot) {
    delete histos.second;
  }
  for (auto& histos : mRawAmplMinEMCAL_tot) {
    delete histos.second;
  }
  for (auto& histos : mRawAmplMinDCAL_tot) {
    delete histos.second;
  }
  for (auto& histos : mRawAmplitudeEMCAL) {
    for (auto h : histos.second) {
      delete h;
    }
  }
  for (auto& histos : mMINRawAmplitudeEMCAL) {
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

  // initialize geometry
  if (!mGeometry)
    mGeometry = o2::emcal::Geometry::GetInstanceFromRunNumber(300000);

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
  mPayloadSizePerDDL = new TH2F("PayloadSizePerDDL", "PayloadSizePerDDL", 40, 0, 40, 200, 0, 20);
  mPayloadSizePerDDL->GetXaxis()->SetTitle("ddl");
  mPayloadSizePerDDL->GetYaxis()->SetTitle("Payload Size / Event (kB)");
  getObjectsManager()->startPublishing(mPayloadSizePerDDL);

  mPayloadSizeTFPerDDL = new TH2F("PayloadSizeTFPerDDL", "PayloadSizeTFPerDDL", 40, 0, 40, 100, 0, 100);
  mPayloadSizeTFPerDDL->GetXaxis()->SetTitle("ddl");
  mPayloadSizeTFPerDDL->GetYaxis()->SetTitle("Payload Size / TF (kB)");
  getObjectsManager()->startPublishing(mPayloadSizeTFPerDDL);

  mPayloadSize = new TH1F("PayloadSize", "PayloadSize", 20, 0, 60000000); //
  mPayloadSize->GetXaxis()->SetTitle("bytes");
  mPayloadSize->GetYaxis()->SetTitle("Counts");
  getObjectsManager()->startPublishing(mPayloadSize);

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

  mNbunchPerChan = new TH1F("NumberBunchPerChannel", "NumberBunchPerChannel", 4, -0.5, 3.5);
  mNbunchPerChan->GetXaxis()->SetTitle("# bunches per channels");
  getObjectsManager()->startPublishing(mNbunchPerChan);

  mNofADCsamples = new TH1F("NumberOfADCPerChannel", "NumberOfADCPerChannel", 15, -0.5, 14.5);
  mNofADCsamples->GetXaxis()->SetTitle("# of ADC sample per channels");
  getObjectsManager()->startPublishing(mNofADCsamples);

  mADCsize = new TH1F("ADCsizePerBunch", "ADCsizePerBunch", 15, -0.5, 14.5);
  mADCsize->GetXaxis()->SetTitle("ADC size per bunch");
  getObjectsManager()->startPublishing(mADCsize);

  mFECmaxCountperSM = new TH2F("NumberOfChWithInput_perSM", "NumberOfChWithInput_perSM", 20, 0, 20, 40, 0, 40);
  mFECmaxCountperSM->GetXaxis()->SetTitle("SM");
  mFECmaxCountperSM->GetYaxis()->SetTitle("max channel count");
  getObjectsManager()->startPublishing(mFECmaxCountperSM);

  mFECmaxIDperSM = new TH2F("FECidMaxChWithInput_perSM", "FECidMaxChWithInput_perSM", 20, 0, 20, 40, 0, 40);
  mFECmaxIDperSM->GetXaxis()->SetTitle("SM");
  mFECmaxIDperSM->GetYaxis()->SetTitle("FEC id");
  getObjectsManager()->startPublishing(mFECmaxIDperSM);

  //histos per SM
  for (auto ism = 0; ism < 20; ism++) {
    mFECmaxCount[ism] = new TH1F(Form("NumberOfChWithInputSM_%d", ism), Form("Number of Channels with input for SM %d", ism), 40, -0.5, 39.5);
    mFECmaxCount[ism]->GetXaxis()->SetTitle("max FEC count");
    mFECmaxCount[ism]->GetYaxis()->SetTitle("maximum occupancy");
    getObjectsManager()->startPublishing(mFECmaxCount[ism]);

    mFECmaxID[ism] = new TH1F(Form("IDFECMaxChWithInputSM_%d", ism), Form("ID FEC Max Number of Channels with input %d", ism), 40, -0.5, 39.5);
    mFECmaxID[ism]->GetXaxis()->SetTitle("FEC id");
    mFECmaxID[ism]->GetYaxis()->SetTitle("maximum occupancy");
    getObjectsManager()->startPublishing(mFECmaxID[ism]);
  }

  //histos per SM and Trigger
  EventType triggers[2] = { EventType::CAL_EVENT, EventType::PHYS_EVENT };
  TString histoStr[2] = { "CAL", "PHYS" };
  for (auto trg = 0; trg < 2; trg++) {

    TProfile2D* histosRawAmplRms; //Filling EMCAL/DCAL
    TProfile2D* histosRawAmplMean;
    TProfile2D* histosRawAmplMax;
    TProfile2D* histosRawAmplMin;
    //EMCAL+DCAL histo
    histosRawAmplRms = new TProfile2D(Form("RMSADC_EMCAL_%s", histoStr[trg].Data()), Form("RMSADC_EMCAL_%s", histoStr[trg].Data()), 96, 0, 95, 208, 0, 207);
    histosRawAmplRms->GetXaxis()->SetTitle("col");
    histosRawAmplRms->GetYaxis()->SetTitle("row");
    getObjectsManager()->startPublishing(histosRawAmplRms);

    histosRawAmplMean = new TProfile2D(Form("MeanADC_EMCAL_%s", histoStr[trg].Data()), Form("MeanADC_EMCAL_%s", histoStr[trg].Data()), 96, 0, 95, 208, 0, 207);
    histosRawAmplMean->GetXaxis()->SetTitle("col");
    histosRawAmplMean->GetYaxis()->SetTitle("row");
    getObjectsManager()->startPublishing(histosRawAmplMean);

    histosRawAmplMax = new TProfile2D(Form("MaxADC_EMCAL_%s", histoStr[trg].Data()), Form("MaxADC_EMCAL_%s", histoStr[trg].Data()), 96, 0, 95, 208, 0, 207);
    histosRawAmplMax->GetXaxis()->SetTitle("col");
    histosRawAmplMax->GetYaxis()->SetTitle("row");
    getObjectsManager()->startPublishing(histosRawAmplMax);

    histosRawAmplMin = new TProfile2D(Form("MinADC_EMCAL_%s", histoStr[trg].Data()), Form("MinADC_EMCAL_%s", histoStr[trg].Data()), 96, 0, 95, 208, 0, 207);
    histosRawAmplMin->GetXaxis()->SetTitle("col");
    histosRawAmplMin->GetYaxis()->SetTitle("raw");
    getObjectsManager()->startPublishing(histosRawAmplMin);

    TH1D* histosRawMintot;
    TH1D* histosRawMinEMCALtot;
    TH1D* histosRawMinDCALtot;

    histosRawMintot = new TH1D(Form("mRawAmplMin_distr_%s", histoStr[trg].Data()), Form("mRawAmplMin_distr_%s", histoStr[trg].Data()), 100, 0., 100.);
    histosRawMintot->GetXaxis()->SetTitle("Raw Amplitude EMCAL,DCAL");
    histosRawMintot->GetYaxis()->SetTitle("Counts");
    getObjectsManager()->startPublishing(histosRawMintot);

    histosRawMinEMCALtot = new TH1D(Form("mRawAmplMinEMCAL_distr_%s", histoStr[trg].Data()), Form("mRawAmplMinEMCAL_distr_%s", histoStr[trg].Data()), 100, 0., 100.);
    histosRawMinEMCALtot->GetXaxis()->SetTitle("Raw Amplitude");
    histosRawMinEMCALtot->GetYaxis()->SetTitle("Counts");
    getObjectsManager()->startPublishing(histosRawMinEMCALtot);

    histosRawMinDCALtot = new TH1D(Form("mRawAmplMinDCAL_distr_%s", histoStr[trg].Data()), Form("mRawAmplMinDCAL_distr_%s", histoStr[trg].Data()), 100, 0., 100.);
    histosRawMinDCALtot->GetXaxis()->SetTitle("Raw Amplitude");
    histosRawMinDCALtot->GetYaxis()->SetTitle("Counts");
    getObjectsManager()->startPublishing(histosRawMinDCALtot);

    std::array<TH1*, 20> histosRawAmplEMCALSM;
    std::array<TH1*, 20> histosMINRawAmplEMCALSM;
    std::array<TH1*, 20> histosRawAmplMaxEMCALSM;
    std::array<TH1*, 20> histosRawAmplMinEMCALSM;
    std::array<TProfile2D*, 20> histosRawAmplRmsSM;
    std::array<TProfile2D*, 20> histosRawAmplMeanSM;
    std::array<TProfile2D*, 20> histosRawAmplMaxSM;
    std::array<TProfile2D*, 20> histosRawAmplMinSM;

    for (auto ism = 0; ism < 20; ism++) {

      histosRawAmplEMCALSM[ism] = new TH1F(Form("RawAmplitudeEMCAL_sm%d_%s", ism, histoStr[trg].Data()), Form(" RawAmplitudeEMCAL%d, %s", ism, histoStr[trg].Data()), 100, 0., 100.);
      histosRawAmplEMCALSM[ism]->GetXaxis()->SetTitle("Raw Amplitude");
      histosRawAmplEMCALSM[ism]->GetYaxis()->SetTitle("Counts");
      getObjectsManager()->startPublishing(histosRawAmplEMCALSM[ism]);

      histosMINRawAmplEMCALSM[ism] = new TH1F(Form("RawMINAmplitudeEMCAL_sm%d_%s", ism, histoStr[trg].Data()), Form(" RawMINAmplitudeEMCAL%d, %s", ism, histoStr[trg].Data()), 100, 0., 100.);
      histosMINRawAmplEMCALSM[ism]->GetXaxis()->SetTitle("Raw MIN Amplitude");
      histosMINRawAmplEMCALSM[ism]->GetYaxis()->SetTitle("Counts");
      getObjectsManager()->startPublishing(histosMINRawAmplEMCALSM[ism]);

      histosRawAmplMaxEMCALSM[ism] = new TH1F(Form("RawAmplMaxEMCAL_sm%d_%s", ism, histoStr[trg].Data()), Form(" RawAmplMaxEMCAL_sm%d_%s", ism, histoStr[trg].Data()), 500, 0., 500.);
      histosRawAmplMaxEMCALSM[ism]->GetXaxis()->SetTitle("Max Raw Amplitude [ADC]");
      histosRawAmplMaxEMCALSM[ism]->GetYaxis()->SetTitle("Counts");
      getObjectsManager()->startPublishing(histosRawAmplMaxEMCALSM[ism]);

      histosRawAmplMinEMCALSM[ism] = new TH1F(Form("RawAmplMinEMCAL_sm%d_%s", ism, histoStr[trg].Data()), Form("RawAmplMinEMCAL_sm%d_%s", ism, histoStr[trg].Data()), 100, 0., 100.);
      histosRawAmplMinEMCALSM[ism]->GetXaxis()->SetTitle("Min Raw Amplitude");
      histosRawAmplMinEMCALSM[ism]->GetYaxis()->SetTitle("Counts");
      getObjectsManager()->startPublishing(histosRawAmplMinEMCALSM[ism]);

      histosRawAmplRmsSM[ism] = new TProfile2D(Form("RMSADCperSM%d_%s", ism, histoStr[trg].Data()), Form("RMSperSM%d_%s", ism, histoStr[trg].Data()), 48, 0, 48, 24, 0, 24);
      histosRawAmplRmsSM[ism]->GetXaxis()->SetTitle("col");
      histosRawAmplRmsSM[ism]->GetYaxis()->SetTitle("row");
      getObjectsManager()->startPublishing(histosRawAmplRmsSM[ism]);

      histosRawAmplMeanSM[ism] = new TProfile2D(Form("MeanADCperSM%d_%s", ism, histoStr[trg].Data()), Form("MeanADCperSM%d_%s", ism, histoStr[trg].Data()), 48, 0, 48, 24, 0, 24);
      histosRawAmplMeanSM[ism]->GetXaxis()->SetTitle("col");
      histosRawAmplMeanSM[ism]->GetYaxis()->SetTitle("row");
      getObjectsManager()->startPublishing(histosRawAmplMeanSM[ism]);

      histosRawAmplMaxSM[ism] = new TProfile2D(Form("MaxADCperSM%d_%s", ism, histoStr[trg].Data()), Form("MaxADCperSM%d_%s", ism, histoStr[trg].Data()), 48, 0, 47, 24, 0, 23);
      histosRawAmplMaxSM[ism]->GetXaxis()->SetTitle("col");
      histosRawAmplMaxSM[ism]->GetYaxis()->SetTitle("row");
      getObjectsManager()->startPublishing(histosRawAmplMaxSM[ism]);

      histosRawAmplMinSM[ism] = new TProfile2D(Form("MinADCperSM%d_%s", ism, histoStr[trg].Data()), Form("MinADCperSM%d_%s", ism, histoStr[trg].Data()), 48, 0, 47, 24, 0, 23);
      histosRawAmplMinSM[ism]->GetXaxis()->SetTitle("col");
      histosRawAmplMinSM[ism]->GetYaxis()->SetTitle("raw");
      getObjectsManager()->startPublishing(histosRawAmplMinSM[ism]);
    } //loop SM
    mRawAmplitudeEMCAL[triggers[trg]] = histosRawAmplEMCALSM;
    mMINRawAmplitudeEMCAL[triggers[trg]] = histosMINRawAmplEMCALSM;
    mRawAmplMaxEMCAL[triggers[trg]] = histosRawAmplMaxEMCALSM;
    mRawAmplMinEMCAL[triggers[trg]] = histosRawAmplMinEMCALSM;

    mRMSperSM[triggers[trg]] = histosRawAmplRmsSM;
    mMEANperSM[triggers[trg]] = histosRawAmplMeanSM;
    mMAXperSM[triggers[trg]] = histosRawAmplMaxSM;
    mMINperSM[triggers[trg]] = histosRawAmplMinSM;

    mRMS[triggers[trg]] = histosRawAmplRms;
    mMEAN[triggers[trg]] = histosRawAmplMean;
    mMAX[triggers[trg]] = histosRawAmplMax;
    mMIN[triggers[trg]] = histosRawAmplMin;

    mRawAmplMin_tot[triggers[trg]] = histosRawMintot;
    mRawAmplMinEMCAL_tot[triggers[trg]] = histosRawMinEMCALtot;
    mRawAmplMinDCAL_tot[triggers[trg]] = histosRawMinDCALtot;

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

  // The type DataOrigin allows only conversion of char arrays with size 4, not char *, therefore
  // the origin string has to be converted manually to the char array and checked for length.
  if (mDataOrigin.size() > 4) {
    QcInfoLogger::GetInstance() << QcInfoLogger::Error << "No valid data origin" << mDataOrigin << ", cannot process" << QcInfoLogger::endm;
    return;
  }
  char dataOrigin[4];
  strcpy(dataOrigin, mDataOrigin.data());

  if (isLostTimeframe(ctx))
    return;

  Int_t nPagesMessage = 0, nSuperpagesMessage = 0;
  QcInfoLogger::GetInstance() << QcInfoLogger::Debug << " Processing message " << mNumberOfMessages << AliceO2::InfoLogger::InfoLogger::endm;
  mNumberOfMessages++;
  mMessageCounter->Fill(1); //for expert

  const int NUMBERSM = 20;
  const int NFEESM = 40; //number of fee per sm

  std::unordered_map<RawEventType, std::array<int, 20>, RawEventTypeHash> maxADCSM, minADCSM;
  std::unordered_map<RawEventType, std::array<std::array<int, NFEESM>, NUMBERSM>, RawEventTypeHash> fecMaxPayload;

  // Accept only descriptor RAWDATA, discard FLP/SUBTIMEFRAME
  auto posReadout = ctx.inputs().getPos("readout");
  auto nslots = ctx.inputs().getNofParts(posReadout);
  for (decltype(nslots) islot = 0; islot < nslots; islot++) {
    auto rawData = ctx.inputs().getByPos(posReadout, islot);
    // get message header
    if (rawData.header != nullptr && rawData.payload != nullptr) {
      const auto* header = header::get<header::DataHeader*>(rawData.header);
      // get payload of a specific input, which is a char array.
      QcInfoLogger::GetInstance() << QcInfoLogger::Debug << "Processing superpage " << mNumberOfSuperpages << AliceO2::InfoLogger::InfoLogger::endm;
      mNumberOfSuperpages++;
      nSuperpagesMessage++;
      mSuperpageCounter->Fill(1); //for expert
      QcInfoLogger::GetInstance() << QcInfoLogger::Debug << " EMCAL Reading Payload size: " << header->payloadSize << " for " << header->dataOrigin << AliceO2::InfoLogger::InfoLogger::endm;

      //fill the histogram with payload sizes
      mPayloadSize->Fill(header->payloadSize);         //for expert
      mTotalDataVolume->Fill(1., header->payloadSize); //for expert

      // Skip SOX headers
      auto rdhblock = reinterpret_cast<const o2::header::RDHAny*>(rawData.payload); //
      if (o2::raw::RDHUtils::getHeaderSize(rdhblock) == static_cast<int>(header->payloadSize)) {
        continue;
      }
      mPayloadSizeTFPerDDL->Fill(o2::raw::RDHUtils::getFEEID(rdhblock), header->payloadSize / 1024.); //PayLoad size per TimeFrame for shifter

      // try decoding payload
      o2::emcal::RawReaderMemory rawreader(gsl::span(rawData.payload, header->payloadSize));

      while (rawreader.hasNext()) {

        QcInfoLogger::GetInstance() << QcInfoLogger::Debug << " Processing page " << mNumberOfPages << AliceO2::InfoLogger::InfoLogger::endm;
        mNumberOfPages++;
        nPagesMessage++;
        mPageCounter->Fill(1); //expert
        rawreader.next();
        auto payLoadSize = rawreader.getPayloadSize(); //payloadsize in byte;

        auto headerR = rawreader.getRawHeader();
        auto feeID = o2::raw::RDHUtils::getFEEID(headerR);

        if (feeID > 40)
          continue; //skip STU ddl

        o2::InteractionRecord triggerIR{ o2::raw::RDHUtils::getTriggerBC(headerR), o2::raw::RDHUtils::getTriggerOrbit(headerR) };
        RawEventType evIndex{ triggerIR, o2::raw::RDHUtils::getTriggerType(headerR) };

        mPayloadSizePerDDL->Fill(feeID, payLoadSize / 1024.); //for shifter

        //trigger type
        auto triggertype = o2::raw::RDHUtils::getTriggerType(headerR);
        bool isPhysTrigger = triggertype & o2::trigger::PhT, isCalibTrigger = triggertype & o2::trigger::Cal;

        if (!(isPhysTrigger || isCalibTrigger)) {
          QcInfoLogger::GetInstance() << QcInfoLogger::Error << " Unmonitored trigger class requested " << AliceO2::InfoLogger::InfoLogger::endm;
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
          mErrorTypeAltro->Fill(feeID, errornum); //for shifter
          continue;
        }
        int supermoduleID = feeID / 2; //SM id
        auto& mapping = mMappings->getMappingForDDL(feeID);

        auto fecIndex = 0;
        auto branchIndex = 0;
        auto fecID = 0;

        for (auto& chan : decoder.getChannels()) {
          // Row and column in online format, must be remapped to offline indexing,
          // otherwise it leads to invalid cell IDs
          auto colOnline = mapping.getColumn(chan.getHardwareAddress());
          auto rowOnline = mapping.getRow(chan.getHardwareAddress());
          auto [row, col] = mGeometry->ShiftOnlineToOfflineCellIndexes(supermoduleID, rowOnline, colOnline);
          auto [phimod, etamod, mod] = mGeometry->GetModuleIndexesFromCellIndexesInSModule(supermoduleID, row, col);
          //tower absolute ID
          auto cellID = mGeometry->GetAbsCellId(supermoduleID, mod, phimod, etamod);
          //position in the EMCAL
          auto [globRow, globCol] = mGeometry->GlobalRowColFromIndex(cellID);

          //exclude LED Mon, TRU
          auto chType = mapping.getChannelType(chan.getHardwareAddress());
          if (chType == CHTYP::LEDMON || chType == CHTYP::TRU)
            continue;

          fecIndex = chan.getFECIndex();
          branchIndex = chan.getBranchIndex();
          fecID = mMappings->getFEEForChannelInDDL(feeID, fecIndex, branchIndex);
          fecMaxChannelsEvent->second[supermoduleID][fecID]++;

          Short_t maxADC = 0;
          Short_t minADC = SHRT_MAX;
          Double_t meanADC = 0;
          Double_t rmsADC = 0;
          EventType evtype = isPhysTrigger ? EventType::PHYS_EVENT : EventType::CAL_EVENT;

          mNbunchPerChan->Fill(chan.getBunches().size()); //(1 histo for EMCAL-526).//1, if high rate --> pile up.

          int numberOfADCsamples = 0;
          for (auto& bunch : chan.getBunches()) {
            auto adcs = bunch.getADC();
            numberOfADCsamples += adcs.size();
            mADCsize->Fill(adcs.size());

            auto maxADCbunch = *max_element(adcs.begin(), adcs.end());
            if (maxADCbunch > maxADC)
              maxADC = maxADCbunch;
            mRawAmplMaxEMCAL[evtype][supermoduleID]->Fill(maxADCbunch); //max for each cell --> for for expert only

            if (maxADCSMEvent == maxADCSM.end()) { //max for each event
              std::array<int, NUMBERSM> maxadc;
              memset(maxadc.data(), 0, maxadc.size());
              maxADCSMEvent = (maxADCSM.insert({ evIndex, maxadc })).first;
            }

            auto minADCbunch = *min_element(adcs.begin(), adcs.end());
            if (minADCbunch < minADC)
              minADC = minADCbunch;
            mRawAmplMinEMCAL[evtype][supermoduleID]->Fill(minADCbunch); // min for each cell --> for for expert only
            mRawAmplMin_tot[evtype]->Fill(minADCbunch);                 //shifter
            if (supermoduleID < 12)
              mRawAmplMinEMCAL_tot[evtype]->Fill(minADCbunch); //shifter (not for pilot beam)
            else
              mRawAmplMinDCAL_tot[evtype]->Fill(minADCbunch); //shifter (not for pilot beam)

            meanADC = TMath::Mean(adcs.begin(), adcs.end());
            rmsADC = TMath::RMS(adcs.begin(), adcs.end());

            mRMS[evtype]->Fill(globCol, globRow, rmsADC);             //for  shifter
            mRMSperSM[evtype][supermoduleID]->Fill(col, row, rmsADC); // no shifter

            mMEAN[evtype]->Fill(globCol, globRow, meanADC);             //for shifter
            mMEANperSM[evtype][supermoduleID]->Fill(col, row, meanADC); // no shifter
          }
          mNofADCsamples->Fill(numberOfADCsamples); // number of bunches per channel

          if (maxADC > maxADCSMEvent->second[supermoduleID])
            maxADCSMEvent->second[supermoduleID] = maxADC;

          mMAXperSM[evtype][supermoduleID]->Fill(col, row, maxADC); //max col,row, per SM
          mMAX[evtype]->Fill(globCol, globRow, maxADC);             //for shifter

          if (minADC < minADCSMEvent->second[supermoduleID])
            minADCSMEvent->second[supermoduleID] = minADC;
          mMINperSM[evtype][supermoduleID]->Fill(col, row, minADC); //min col,row, per SM
          mMIN[evtype]->Fill(globCol, globRow, minADC);             //for shifter
        }                                                           //channels
      }                                                             //new page
    }                                                               //header
  }                                                                 //inputs
  mNumberOfPagesPerMessage->Fill(nPagesMessage);                    // for experts
  mNumberOfSuperpagesPerMessage->Fill(nSuperpagesMessage);

  // Fill histograms with cached values
  for (auto maxfec : fecMaxPayload) {
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
        continue;                           // Reject links on different FLP
      mFECmaxID[ism]->Fill(maxfecID);       // histo to monitor the ID of FEC with max count for shifter
      mFECmaxCount[ism]->Fill(maxfecCount); // histo to monitor the count //for shifter

      mFECmaxIDperSM->Fill(ism, maxfecID);       //filled as a funcion of SM (shifter)
      mFECmaxCountperSM->Fill(ism, maxfecCount); //filled as a function of SM (shifter)
    }
  }
  for (auto maxadc : maxADCSM) {
    auto triggertype = maxadc.first.mTrigger;
    bool isPhysTrigger = triggertype & o2::trigger::PhT;
    EventType evtype = isPhysTrigger ? EventType::PHYS_EVENT : EventType::CAL_EVENT;
    for (int ism = 0; ism < NUMBERSM; ism++) {
      mRawAmplitudeEMCAL[evtype][ism]->Fill(maxadc.second[ism]); //max in the event for shifter
    }
  }

  for (auto minadc : minADCSM) {
    auto triggertype = minadc.first.mTrigger;
    bool isPhysTrigger = triggertype & o2::trigger::PhT;
    EventType evtype = isPhysTrigger ? EventType::PHYS_EVENT : EventType::CAL_EVENT;
    for (int ism = 0; ism < NUMBERSM; ism++) {
      mMINRawAmplitudeEMCAL[evtype][ism]->Fill(minadc.second[ism]); //max in the event (not for shifter)
    }
  }
  // Same for other cached values
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
  EventType triggers[2] = { EventType::CAL_EVENT, EventType::PHYS_EVENT };

  for (const auto& trg : triggers) {
    mRMS[trg]->Reset();
    mMEAN[trg]->Reset();
    mMAX[trg]->Reset();
    mMIN[trg]->Reset();
    for (Int_t ism = 0; ism < 20; ism++) {
      mRawAmplitudeEMCAL[trg][ism]->Reset();
      mMINRawAmplitudeEMCAL[trg][ism]->Reset();
      mRawAmplMaxEMCAL[trg][ism]->Reset();
      mRawAmplMinEMCAL[trg][ism]->Reset();
      mRMSperSM[trg][ism]->Reset();
      mMEANperSM[trg][ism]->Reset();
      mMAXperSM[trg][ism]->Reset();
      mMINperSM[trg][ism]->Reset();
    }
  }
  mPayloadSizePerDDL->Reset();
  mPayloadSizeTFPerDDL->Reset();
  mPayloadSize->Reset();
  mErrorTypeAltro->Reset();
  mNbunchPerChan->Reset();
  mNofADCsamples->Reset();
  mADCsize->Reset();
  mFECmaxIDperSM->Reset();
  mFECmaxCountperSM->Reset();
}

bool RawTask::isLostTimeframe(framework::ProcessingContext& ctx) const
{
  constexpr auto originEMC = header::gDataOriginEMC;
  o2::framework::InputSpec dummy{ "dummy",
                                  framework::ConcreteDataMatcher{ originEMC,
                                                                  header::gDataDescriptionRawData,
                                                                  0xDEADBEEF } };
  for (const auto& ref : o2::framework::InputRecordWalker(ctx.inputs(), { dummy })) {
    //auto posReadout = ctx.inputs().getPos("readout");
    //auto nslots = ctx.inputs().getNofParts(posReadout);
    //for (decltype(nslots) islot = 0; islot < nslots; islot++) {
    //  const auto& ref = ctx.inputs().getByPos(posReadout, islot);
    const auto dh = o2::framework::DataRefUtils::getHeader<o2::header::DataHeader*>(ref);
    // if (dh->subSpecification == 0xDEADBEEF) {
    if (dh->payloadSize == 0) {
      return true;
      //  }
    }
  }
  return false;
}

} // namespace o2::quality_control_modules::emcal
