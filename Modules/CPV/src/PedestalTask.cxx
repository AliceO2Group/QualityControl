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
/// \file   PedestalTask.cxx
/// \author Sergey Evdokimov
///

#include <TH1.h>
#include <TH2.h>
#include <TF1.h>
#include <TSpectrum.h>

#include "QualityControl/QcInfoLogger.h"
#include "CPV/PedestalTask.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/DataRef.h>
#include <Framework/DataRefUtils.h>
#include <DataFormatsCPV/TriggerRecord.h>
#include <DataFormatsCPV/Digit.h>
#include <DataFormatsCPV/Pedestals.h>
#include <CPVReconstruction/RawDecoder.h>

namespace o2::quality_control_modules::cpv
{

PedestalTask::PedestalTask()
{
  for (int i = 0; i < kNHist1D; i++) {
    mHist1D[i] = nullptr;
  }
  for (int i = 0; i < kNHist2D; i++) {
    mHist2D[i] = nullptr;
  }
  for (int i = 0; i < o2::cpv::Geometry::kNCHANNELS; i++) {
    mHistAmplitudes[i] = nullptr;
  }
}

PedestalTask::~PedestalTask()
{
  for (int i = 0; i < kNHist1D; i++) {
    if (mHist1D[i]) {
      mHist1D[i]->Delete();
      mHist1D[i] = nullptr;
    }
  }
  for (int i = 0; i < kNHist2D; i++) {
    if (mHist2D[i]) {
      mHist2D[i]->Delete();
      mHist2D[i] = nullptr;
    }
  }
  for (int i = 0; i < o2::cpv::Geometry::kNCHANNELS; i++) {
    if (mHistAmplitudes[i]) {
      mHistAmplitudes[i]->Delete();
      mHistAmplitudes[i] = nullptr;
    }
  }
}

void PedestalTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize PedestalTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("minNEventsToUpdatePedestals"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter : minNEventsToUpdatePedestals " << param->second << ENDM;
    mMinNEventsToUpdatePedestals = stoi(param->second);
    ILOG(Info, Devel) << "I set minNEventsToUpdatePedestals = " << mMinNEventsToUpdatePedestals << ENDM;
  } else {
    ILOG(Info, Devel) << "Default parameter : minNEventsToUpdatePedestals = " << mMinNEventsToUpdatePedestals << ENDM;
  }
  if (auto param = mCustomParameters.find("monitorPedestalCalibrator"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter : monitorPedestalCalibrator " << param->second << ENDM;
    mMonitorPedestalCalibrator = stoi(param->second);
    ILOG(Info, Devel) << "I set mMonitorPedestalCalibrator = " << mMonitorPedestalCalibrator << ENDM;
  } else {
    ILOG(Info, Devel) << "Default parameter : monitorPedestalCalibrator = " << mMonitorPedestalCalibrator << ENDM;
  }
  if (auto param = mCustomParameters.find("monitorDigits"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter : monitorDigits " << param->second << ENDM;
    mMonitorDigits = stoi(param->second);
    ILOG(Info, Devel) << "I set mMonitorDigits = " << mMonitorDigits << ENDM;
  } else {
    ILOG(Info, Devel) << "Default parameter : monitorDigits = " << mMonitorDigits << ENDM;
  }

  if (mMonitorPedestalCalibrator) {
    ILOG(Info, Devel) << "Results of pedestal calibrator sent to CCDB will be monitored" << ENDM;
  }
  if (mMonitorDigits) {
    ILOG(Info, Devel) << "Digits will be monitored. Look at *FromDigits MOs." << ENDM;
  }

  initHistograms();
  mNEventsTotal = 0;
  mNEventsFromLastFillHistogramsCall = 0;
}

void PedestalTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity() : Run Number of Activity is " << activity.mId << ENDM;
  resetHistograms();
  mNEventsTotal = 0;
  mNEventsFromLastFillHistogramsCall = 0;
  mRunNumber = activity.mId;
  for (int i = 0; i < kNHist1D; i++) {
    if (mHist1D[i]) {
      getObjectsManager()->addMetadata(mHist1D[i]->GetName(), "RunNumberFromTask", Form("%d", mRunNumber));
    }
  }
  for (int i = 0; i < kNHist2D; i++) {
    if (mHist2D[i]) {
      getObjectsManager()->addMetadata(mHist2D[i]->GetName(), "RunNumberFromTask", Form("%d", mRunNumber));
    }
  }
}

void PedestalTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
  // at at the startOfCycle all HistAmplitudes are not updated by definition
  if (mMonitorDigits) {
    for (int i = 0; i < o2::cpv::Geometry::kNCHANNELS; i++) {
      mIsUpdatedAmplitude[i] = false;
    }
  }
}

void PedestalTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  LOG(info) << "PedestalTask::monitorData()";
  // In this function you can access data inputs specified in the JSON config file, for example:
  //   "query": "random:ITS/RAWDATA/0"
  // which is correspondingly <binding>:<dataOrigin>/<dataDescription>/<subSpecification
  // One can also access conditions from CCDB, via separate API (see point 3)

  // Use Framework/DataRefUtils.h or Framework/InputRecord.h to access and unpack inputs (both are documented)
  // One can find additional examples at:
  // https://github.com/AliceO2Group/AliceO2/blob/dev/Framework/Core/README.md#using-inputs---the-inputrecord-api

  // Some examples:

  // 1. In a loop
  int nInputs = ctx.inputs().size();
  mHist1D[H1DNInputs]->Fill(nInputs);

  int nValidInputs = ctx.inputs().countValidInputs();
  mHist1D[H1DNValidInputs]->Fill(nValidInputs);

  bool hasRawErrors = false, hasDigits = false;
  bool hasPedestalsCLP = false, hasPedEffsCLP = false, hasFEEThrsCLP = false, hasDeadChnlsCLP = false, hasHighThrsCLP = false;
  for (auto&& input : o2::framework::InputRecordWalker(ctx.inputs())) {
    // get message header
    if (input.header != nullptr && input.payload != nullptr) {
      auto payloadSize = o2::framework::DataRefUtils::getPayloadSize(input);
      // get payload of a specific input, which is a char array.
      // const char* payload = input.payload;
      mHist1D[H1DInputPayloadSize]->Fill(payloadSize);
      const auto* header = o2::framework::DataRefUtils::getHeader<header::DataHeader*>(input);
      if (payloadSize) {
        if ((strcmp(header->dataOrigin.str, "CPV") == 0) && (strcmp(header->dataDescription.str, "DIGITS") == 0)) {
          // LOG(info) << "monitorData() : I found digits in inputs";
          hasDigits = true;
        }
        if ((strcmp(header->dataOrigin.str, "CLP") == 0) && (strcmp(header->dataDescription.str, "CPV_Pedestals") == 0)) {
          // LOG(info) << "monitorData() : I found CPV_Pedestals in inputs";
          hasPedestalsCLP = true;
        }
        if ((strcmp(header->dataOrigin.str, "CLP") == 0) && (strcmp(header->dataDescription.str, "CPV_FEEThrs") == 0)) {
          // LOG(info) << "monitorData() : I found CPV_FEEThrs in inputs";
          hasFEEThrsCLP = true;
        }
        if ((strcmp(header->dataOrigin.str, "CLP") == 0) && (strcmp(header->dataDescription.str, "CPV_DeadChnls") == 0)) {
          // LOG(info) << "monitorData() : I found CPV_DeadChnls in inputs";
          hasDeadChnlsCLP = true;
        }
        if ((strcmp(header->dataOrigin.str, "CLP") == 0) && (strcmp(header->dataDescription.str, "CPV_HighThrs") == 0)) {
          // LOG(info) << "monitorData() : I found CPV_HighThrs in inputs";
          hasHighThrsCLP = true;
        }
        if ((strcmp(header->dataOrigin.str, "CLP") == 0) && (strcmp(header->dataDescription.str, "CPV_PedEffs") == 0)) {
          // LOG(info) << "monitorData() : I found CPV_PedEffs in inputs";
          hasPedEffsCLP = true;
        }
        if ((strcmp(header->dataOrigin.str, "CPV") == 0) && (strcmp(header->dataDescription.str, "RAWHWERRORS") == 0)) {
          // LOG(info) << "monitorData() : I found rawhwerrors in inputs";
          hasRawErrors = true;
        }
      }
    }
  }

  // 2. Using get("<binding>")
  // raw errors
  if (hasRawErrors) {
    auto rawErrors = ctx.inputs().get<gsl::span<o2::cpv::RawDecoderError>>("rawerrors");
    for (const auto& rawError : rawErrors) {
      mHist1D[H1DRawErrors]->Fill(rawError.errortype);
    }
  }

  // digits monitoring
  if (hasDigits && mMonitorDigits) {
    auto digits = ctx.inputs().get<gsl::span<o2::cpv::Digit>>("digits");
    mHist1D[H1DNDigitsPerInput]->Fill(digits.size());
    auto digitsTR = ctx.inputs().get<gsl::span<o2::cpv::TriggerRecord>>("dtrigrec");
    // mNEventsTotal += digitsTR.size();//number of events in the current input
    for (const auto& trigRecord : digitsTR) {
      LOG(debug) << " monitorData() : trigger record #" << mNEventsTotal
                 << " contains " << trigRecord.getNumberOfObjects() << " objects.";
      if (trigRecord.getNumberOfObjects() > 0) { // at least 1 digit -> pedestal event
        mNEventsTotal++;
        mNEventsFromLastFillHistogramsCall++;
        for (int iDig = trigRecord.getFirstEntry(); iDig < trigRecord.getFirstEntry() + trigRecord.getNumberOfObjects(); iDig++) {
          mHist1D[H1DDigitIds]->Fill(digits[iDig].getAbsId());
          short relId[3];
          if (o2::cpv::Geometry::absToRelNumbering(digits[iDig].getAbsId(), relId)) {
            // reminder: relId[3]={Module, phi col, z row} where Module=2..4, phi col=0..127, z row=0..59
            mHist2D[H2DDigitMapM2 + relId[0] - 2]->Fill(relId[1], relId[2]);
            mHistAmplitudes[digits[iDig].getAbsId()]->Fill(digits[iDig].getAmplitude());
            mIsUpdatedAmplitude[digits[iDig].getAbsId()] = true;
          }
        }
      }
    }
  }

  // pedestal calibrator output monitoring
  if (mMonitorPedestalCalibrator) {
    // o2::cpv::Pedestals object
    if (hasPedestalsCLP) {
      auto peds = o2::framework::DataRefUtils::as<o2::framework::CCDBSerialized<o2::cpv::Pedestals>>(ctx.inputs().get<o2::framework::DataRef>("peds"));
      if (peds) {
        mNtimesCCDBPayloadFetched++;
        LOG(info) << "PedestalTask::monitorData() : Extracted o2::cpv::Pedestals from CLP payload";
        short relId[3];
        for (int ch = 0; ch < o2::cpv::Geometry::kNCHANNELS; ch++) {
          o2::cpv::Geometry::absToRelNumbering(ch, relId);
          mHist2D[H2DPedestalValueMapM2 + relId[0] - 2]->SetBinContent(relId[1] + 1, relId[2] + 1, peds->getPedestal(ch));
          mHist1D[H1DPedestalValueM2 + relId[0] - 2]->Fill(peds->getPedestal(ch));
          mHist2D[H2DPedestalSigmaMapM2 + relId[0] - 2]->SetBinContent(relId[1] + 1, relId[2] + 1, peds->getPedSigma(ch));
          mHist1D[H1DPedestalSigmaM2 + relId[0] - 2]->Fill(peds->getPedSigma(ch));
        }
      }
    }
    // FEE thresolds
    if (hasFEEThrsCLP) {
      auto feethrs = o2::framework::DataRefUtils::as<o2::framework::CCDBSerialized<std::vector<int>>>(ctx.inputs().get<o2::framework::DataRef>("feethrs"));
      if (feethrs) {
        LOG(info) << "PedestalTask::monitorData() : Extracted FEE thresholds std::vector<int> of size " << feethrs->size() << "from CLP payload";
        short relId[3];
        for (int ch = 0; ch < o2::cpv::Geometry::kNCHANNELS; ch++) {
          o2::cpv::Geometry::absToRelNumbering(ch, relId);
          mHist2D[H2DFEEThresholdsMapM2 + relId[0] - 2]->SetBinContent(relId[1] + 1, relId[2] + 1, float(feethrs->at(ch) & 0xffff));
        }
      }
    }
    // Dead channels
    if (hasDeadChnlsCLP) {
      auto deadchs = o2::framework::DataRefUtils::as<o2::framework::CCDBSerialized<std::vector<int>>>(ctx.inputs().get<o2::framework::DataRef>("deadchs"));
      if (deadchs) {
        LOG(info) << "PedestalTask::monitorData() : Extracted dead channels std::vector<int> of size " << deadchs->size() << "from CLP payload";
        short relId[3];
        for (int ch : *deadchs) {
          o2::cpv::Geometry::absToRelNumbering(ch, relId);
          mHist2D[H2DDeadChanelsMapM2 + relId[0] - 2]->SetBinContent(relId[1] + 1, relId[2] + 1, 1.);
        }
      }
    }
    // High threshold channels
    if (hasHighThrsCLP) {
      auto highthrs = o2::framework::DataRefUtils::as<o2::framework::CCDBSerialized<std::vector<int>>>(ctx.inputs().get<o2::framework::DataRef>("highthrs"));
      if (highthrs) {
        LOG(info) << "PedestalTask::monitorData() : Extracted high threshold channels std::vector<int> of size " << highthrs->size() << "from CLP payload";
        short relId[3];
        for (int ch : *highthrs) {
          o2::cpv::Geometry::absToRelNumbering(ch, relId);
        }
      }
    }
    // Efficiencies
    if (hasPedEffsCLP) {
      auto pedeffs = o2::framework::DataRefUtils::as<o2::framework::CCDBSerialized<std::vector<float>>>(ctx.inputs().get<o2::framework::DataRef>("pedeffs"));
      if (pedeffs) {
        LOG(info) << "PedestalTask::monitorData() : Extracted pedesta efficiencies std::vector<float> of size " << pedeffs->size() << "from CLP payload";
        short relId[3];
        for (int ch = 0; ch < o2::cpv::Geometry::kNCHANNELS; ch++) {
          o2::cpv::Geometry::absToRelNumbering(ch, relId);
          mHist2D[H2DPedestalEfficiencyMapM2 + relId[0] - 2]->SetBinContent(relId[1] + 1, relId[2] + 1, pedeffs->at(ch));
          mHist1D[H1DPedestalEfficiencyM2 + relId[0] - 2]->Fill(pedeffs->at(ch));
        }
      }
    }
    LOG(info) << "PedestalTask::monitorData() : I fetched o2::cpv::Pedestals CLP payload " << mNtimesCCDBPayloadFetched << " times.";
  }
}

void PedestalTask::endOfCycle()
{
  ILOG(Info, Devel) << "PedestalTask::endOfCycle()" << ENDM;
  if (mMonitorDigits) {
    // fit histograms if have sufficient increment of event number
    if (mNEventsFromLastFillHistogramsCall >= mMinNEventsToUpdatePedestals) {
      ILOG(Info, Devel) << "I call fillDigitsHistograms()" << ENDM;
      fillDigitsHistograms();
      mNEventsFromLastFillHistogramsCall = 0;
    } else {
      ILOG(Info, Devel) << "Not enough events (" << mNEventsFromLastFillHistogramsCall << ") to call fillDigitsHistograms(). Min "
                        << mMinNEventsToUpdatePedestals << " needed." << ENDM;
    }
  }
}

void PedestalTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
  if (!mMonitorDigits) {
    // do a final fill of histograms (if needed)
    if (mNEventsFromLastFillHistogramsCall) {
      ILOG(Info, Devel) << "Final call of fillDigitsHistograms() " << ENDM;
      fillDigitsHistograms();
    }
  }
}

void PedestalTask::reset()
{
  // clean all the monitor objects here
  ILOG(Debug, Devel) << "Resetting PedestalTask" << ENDM;
  resetHistograms();
  mNEventsTotal = 0;
  mNEventsFromLastFillHistogramsCall = 0;
}

void PedestalTask::initHistograms()
{
  ILOG(Info, Devel) << "initing histograms" << ENDM;

  if (!mHist1D[H1DRawErrors]) {
    mHist1D[H1DRawErrors] =
      new TH1F("RawErrors", "Raw Errors", 20, 0., 20.);
    getObjectsManager()->startPublishing(mHist1D[H1DRawErrors]);
  } else {
    mHist1D[H1DRawErrors]->Reset();
  }

  if (mMonitorDigits) {
    // create monitoring histograms (or reset, if they already exist)
    for (int i = 0; i < o2::cpv::Geometry::kNCHANNELS; i++) {
      if (!mHistAmplitudes[i]) {
        mHistAmplitudes[i] =
          new TH1F(Form("HistAmplitude%d", i), Form("HistAmplitude%d", i), 4096, 0., 4096.);
        // publish some of them
        if (i % 1000 == 0) {
          getObjectsManager()->startPublishing(mHistAmplitudes[i]);
        }
      } else {
        mHistAmplitudes[i]->Reset();
      }
      mIsUpdatedAmplitude[i] = false;
    }
  }
  // 1D Histos
  if (!mHist1D[H1DInputPayloadSize]) {
    mHist1D[H1DInputPayloadSize] =
      new TH1F("InputPayloadSize", "Input Payload Size", 30000, 0, 30000000);
    getObjectsManager()->startPublishing(mHist1D[H1DInputPayloadSize]);
  } else {
    mHist1D[H1DInputPayloadSize]->Reset();
  }

  if (!mHist1D[H1DNInputs]) {
    mHist1D[H1DNInputs] = new TH1F("NInputs", "Number of inputs", 20, -0.5, 19.5);
    getObjectsManager()->startPublishing(mHist1D[H1DNInputs]);
  } else {
    mHist1D[H1DNInputs]->Reset();
  }

  if (!mHist1D[H1DNValidInputs]) {
    mHist1D[H1DNValidInputs] =
      new TH1F("NValidInputs", "Number of valid inputs", 20, -0.5, 19.5);
    getObjectsManager()->startPublishing(mHist1D[H1DNValidInputs]);
  } else {
    mHist1D[H1DNValidInputs]->Reset();
  }

  if (mMonitorDigits) {
    if (!mHist1D[H1DNDigitsPerInput]) {
      mHist1D[H1DNDigitsPerInput] =
        new TH1F("NDigitsPerInput", "Number of digits per input", 30000, 0, 300000);
      if (mMonitorDigits) {
        getObjectsManager()->startPublishing(mHist1D[H1DNDigitsPerInput]);
      }
    } else {
      mHist1D[H1DNDigitsPerInput]->Reset();
    }

    if (!mHist1D[H1DDigitIds]) {
      mHist1D[H1DDigitIds] = new TH1F("DigitIds", "Digit Ids", 30000, -0.5, 29999.5);
      if (mMonitorDigits) {
        getObjectsManager()->startPublishing(mHist1D[H1DDigitIds]);
      }
    } else {
      mHist1D[H1DDigitIds]->Reset();
    }
  }

  for (int mod = 0; mod < 3; mod++) {
    if (mMonitorPedestalCalibrator) { // MOs to appear in pedestal calibrator monitoring mode
      if (!mHist1D[H1DPedestalValueM2 + mod]) {
        mHist1D[H1DPedestalValueM2 + mod] =
          new TH1F(
            Form("PedestalValueM%d", mod + 2),
            Form("Pedestal value distribution M%d", mod + 2),
            512, 0, 512);
        mHist1D[H1DPedestalValueM2 + mod]->GetXaxis()->SetTitle("Pedestal value");
        getObjectsManager()->startPublishing(mHist1D[H1DPedestalValueM2 + mod]);
      } else {
        mHist1D[H1DPedestalValueM2 + mod]->Reset();
      }

      if (!mHist1D[H1DPedestalSigmaM2 + mod]) {
        mHist1D[H1DPedestalSigmaM2 + mod] =
          new TH1F(
            Form("PedestalSigmaM%d", mod + 2),
            Form("Pedestal sigma distribution M%d", mod + 2),
            200, 0, 20);
        mHist1D[H1DPedestalSigmaM2 + mod]->GetXaxis()->SetTitle("Pedestal sigma");
        getObjectsManager()->startPublishing(mHist1D[H1DPedestalSigmaM2 + mod]);
      } else {
        mHist1D[H1DPedestalSigmaM2 + mod]->Reset();
      }

      if (!mHist1D[H1DPedestalEfficiencyM2 + mod]) {
        mHist1D[H1DPedestalEfficiencyM2 + mod] =
          new TH1F(
            Form("PedestalEfficiencyM%d", mod + 2),
            Form("Pedestal efficiency distribution M%d", mod + 2),
            500, 0., 5.);
        mHist1D[H1DPedestalEfficiencyM2 + mod]->GetXaxis()->SetTitle("Pedestal efficiency");
        getObjectsManager()->startPublishing(mHist1D[H1DPedestalEfficiencyM2 + mod]);
      } else {
        mHist1D[H1DPedestalEfficiencyM2 + mod]->Reset();
      }
    }
    if (mMonitorDigits) { // MOs to appear in digits monitoring mode
      if (!mHist1D[H1DPedestalValueInDigitsM2 + mod]) {
        mHist1D[H1DPedestalValueInDigitsM2 + mod] =
          new TH1F(
            Form("PedestalValueInDigitsM%d", mod + 2),
            Form("Pedestal value distribution M%d", mod + 2),
            512, 0, 512);
        mHist1D[H1DPedestalValueInDigitsM2 + mod]->GetXaxis()->SetTitle("Pedestal value");
        getObjectsManager()->startPublishing(mHist1D[H1DPedestalValueInDigitsM2 + mod]);
      } else {
        mHist1D[H1DPedestalValueInDigitsM2 + mod]->Reset();
      }

      if (!mHist1D[H1DPedestalSigmaInDigitsM2 + mod]) {
        mHist1D[H1DPedestalSigmaInDigitsM2 + mod] =
          new TH1F(
            Form("PedestalSigmaInDigitsM%d", mod + 2),
            Form("Pedestal sigma distribution M%d", mod + 2),
            200, 0, 20);
        mHist1D[H1DPedestalSigmaInDigitsM2 + mod]->GetXaxis()->SetTitle("Pedestal sigma");
        getObjectsManager()->startPublishing(mHist1D[H1DPedestalSigmaInDigitsM2 + mod]);
      } else {
        mHist1D[H1DPedestalSigmaInDigitsM2 + mod]->Reset();
      }

      if (!mHist1D[H1DPedestalEfficiencyInDigitsM2 + mod]) {
        mHist1D[H1DPedestalEfficiencyInDigitsM2 + mod] =
          new TH1F(
            Form("PedestalEfficiencyInDigitsM%d", mod + 2),
            Form("Pedestal efficiency distribution M%d", mod + 2),
            500, 0., 5.);
        mHist1D[H1DPedestalEfficiencyInDigitsM2 + mod]->GetXaxis()->SetTitle("Pedestal efficiency");
        getObjectsManager()->startPublishing(mHist1D[H1DPedestalEfficiencyInDigitsM2 + mod]);
      } else {
        mHist1D[H1DPedestalEfficiencyInDigitsM2 + mod]->Reset();
      }
    }
  }

  // 2D Histos
  if (!mHist2D[H2DErrorType]) {
    mHist2D[H2DErrorType] = new TH2F("ErrorType", "ErrorType", 50, 0, 50, 5, 0, 5);
    mHist2D[H2DErrorType]->SetStats(0);
    getObjectsManager()->startPublishing(mHist2D[H2DErrorType]);
  } else {
    mHist2D[H2DErrorType]->Reset();
  }

  // per-module histos
  int nPadsX = o2::cpv::Geometry::kNumberOfCPVPadsPhi;
  int nPadsZ = o2::cpv::Geometry::kNumberOfCPVPadsZ;

  for (int mod = 0; mod < 3; mod++) {
    if (mMonitorPedestalCalibrator) { // MOs to appear in pedestal calibrator monitoring mode
      if (!mHist2D[H2DPedestalValueMapM2 + mod]) {
        mHist2D[H2DPedestalValueMapM2 + mod] =
          new TH2F(
            Form("PedestalValueMapM%d", 2 + mod),
            Form("PedestalValue Map in M%d", mod + 2),
            nPadsX, -0.5, nPadsX - 0.5,
            nPadsZ, -0.5, nPadsZ - 0.5);
        mHist2D[H2DPedestalValueMapM2 + mod]->GetXaxis()->SetTitle("x, pad");
        mHist2D[H2DPedestalValueMapM2 + mod]->GetYaxis()->SetTitle("z, pad");
        mHist2D[H2DPedestalValueMapM2 + mod]->SetStats(0);
        getObjectsManager()->startPublishing(mHist2D[H2DPedestalValueMapM2 + mod]);
      } else {
        mHist2D[H2DPedestalValueMapM2 + mod]->Reset();
      }

      if (!mHist2D[H2DPedestalSigmaMapM2 + mod]) {
        mHist2D[H2DPedestalSigmaMapM2 + mod] =
          new TH2F(
            Form("PedestalSigmaMapM%d", 2 + mod),
            Form("PedestalSigma Map in M%d", mod + 2),
            nPadsX, -0.5, nPadsX - 0.5,
            nPadsZ, -0.5, nPadsZ - 0.5);
        mHist2D[H2DPedestalSigmaMapM2 + mod]->GetXaxis()->SetTitle("x, pad");
        mHist2D[H2DPedestalSigmaMapM2 + mod]->GetYaxis()->SetTitle("z, pad");
        mHist2D[H2DPedestalSigmaMapM2 + mod]->SetStats(0);
        getObjectsManager()->startPublishing(mHist2D[H2DPedestalSigmaMapM2 + mod]);
      } else {
        mHist2D[H2DPedestalSigmaMapM2 + mod]->Reset();
      }

      if (!mHist2D[H2DPedestalEfficiencyMapM2 + mod]) {
        mHist2D[H2DPedestalEfficiencyMapM2 + mod] =
          new TH2F(
            Form("PedestalEfficiencyMapM%d", 2 + mod),
            Form("Pedestal Efficiency Map in M%d", mod + 2),
            nPadsX, -0.5, nPadsX - 0.5,
            nPadsZ, -0.5, nPadsZ - 0.5);
        mHist2D[H2DPedestalEfficiencyMapM2 + mod]->GetXaxis()->SetTitle("x, pad");
        mHist2D[H2DPedestalEfficiencyMapM2 + mod]->GetYaxis()->SetTitle("z, pad");
        mHist2D[H2DPedestalEfficiencyMapM2 + mod]->SetStats(0);
        getObjectsManager()->startPublishing(mHist2D[H2DPedestalEfficiencyMapM2 + mod]);
      } else {
        mHist2D[H2DPedestalEfficiencyMapM2 + mod]->Reset();
      }
      if (!mHist2D[H2DFEEThresholdsMapM2 + mod]) {
        mHist2D[H2DFEEThresholdsMapM2 + mod] =
          new TH2F(
            Form("FEEThresholdsMapM%d", 2 + mod),
            Form("FEE Thresholds Map in M%d", mod + 2),
            nPadsX, -0.5, nPadsX - 0.5,
            nPadsZ, -0.5, nPadsZ - 0.5);
        mHist2D[H2DFEEThresholdsMapM2 + mod]->GetXaxis()->SetTitle("x, pad");
        mHist2D[H2DFEEThresholdsMapM2 + mod]->GetYaxis()->SetTitle("z, pad");
        mHist2D[H2DFEEThresholdsMapM2 + mod]->SetStats(0);
        getObjectsManager()->startPublishing(mHist2D[H2DFEEThresholdsMapM2 + mod]);
      } else {
        mHist2D[H2DFEEThresholdsMapM2 + mod]->Reset();
      }
      if (!mHist2D[H2DDeadChanelsMapM2 + mod]) {
        mHist2D[H2DDeadChanelsMapM2 + mod] =
          new TH2F(
            Form("DeadChanelsMapM%d", 2 + mod),
            Form("Daed channels map Map in M%d", mod + 2),
            nPadsX, -0.5, nPadsX - 0.5,
            nPadsZ, -0.5, nPadsZ - 0.5);
        mHist2D[H2DDeadChanelsMapM2 + mod]->GetXaxis()->SetTitle("x, pad");
        mHist2D[H2DDeadChanelsMapM2 + mod]->GetYaxis()->SetTitle("z, pad");
        mHist2D[H2DDeadChanelsMapM2 + mod]->SetStats(0);
        getObjectsManager()->startPublishing(mHist2D[H2DDeadChanelsMapM2 + mod]);
      } else {
        mHist2D[H2DDeadChanelsMapM2 + mod]->Reset();
      }
      if (!mHist2D[H2DHighThresholdMapM2 + mod]) {
        mHist2D[H2DHighThresholdMapM2 + mod] =
          new TH2F(
            Form("HighThresholdMapM%d", 2 + mod),
            Form("High threshold Map in M%d", mod + 2),
            nPadsX, -0.5, nPadsX - 0.5,
            nPadsZ, -0.5, nPadsZ - 0.5);
        mHist2D[H2DHighThresholdMapM2 + mod]->GetXaxis()->SetTitle("x, pad");
        mHist2D[H2DHighThresholdMapM2 + mod]->GetYaxis()->SetTitle("z, pad");
        mHist2D[H2DHighThresholdMapM2 + mod]->SetStats(0);
        getObjectsManager()->startPublishing(mHist2D[H2DHighThresholdMapM2 + mod]);
      } else {
        mHist2D[H2DHighThresholdMapM2 + mod]->Reset();
      }
    }

    if (mMonitorDigits) { // MOs to appear in digits monitoring mode
      if (!mHist2D[H2DDigitMapM2 + mod]) {
        mHist2D[H2DDigitMapM2 + mod] =
          new TH2F(
            Form("DigitMapM%d", 2 + mod),
            Form("Digit Map in M%d", mod + 2),
            nPadsX, -0.5, nPadsX - 0.5,
            nPadsZ, -0.5, nPadsZ - 0.5);
        mHist2D[H2DDigitMapM2 + mod]->GetXaxis()->SetTitle("x, pad");
        mHist2D[H2DDigitMapM2 + mod]->GetYaxis()->SetTitle("z, pad");
        mHist2D[H2DDigitMapM2 + mod]->SetStats(0);
        getObjectsManager()->startPublishing(mHist2D[H2DDigitMapM2 + mod]);
      } else {
        mHist2D[H2DDigitMapM2 + mod]->Reset();
      }
      if (!mHist2D[H2DPedestalValueMapInDigitsM2 + mod]) {
        mHist2D[H2DPedestalValueMapInDigitsM2 + mod] =
          new TH2F(
            Form("PedestalValueMapInDigitsM%d", 2 + mod),
            Form("PedestalValue Map in M%d", mod + 2),
            nPadsX, -0.5, nPadsX - 0.5,
            nPadsZ, -0.5, nPadsZ - 0.5);
        mHist2D[H2DPedestalValueMapInDigitsM2 + mod]->GetXaxis()->SetTitle("x, pad");
        mHist2D[H2DPedestalValueMapInDigitsM2 + mod]->GetYaxis()->SetTitle("z, pad");
        mHist2D[H2DPedestalValueMapInDigitsM2 + mod]->SetStats(0);
        getObjectsManager()->startPublishing(mHist2D[H2DPedestalValueMapInDigitsM2 + mod]);
      } else {
        mHist2D[H2DPedestalValueMapInDigitsM2 + mod]->Reset();
      }

      if (!mHist2D[H2DPedestalSigmaMapInDigitsM2 + mod]) {
        mHist2D[H2DPedestalSigmaMapInDigitsM2 + mod] =
          new TH2F(
            Form("PedestalSigmaMapInDigitsM%d", 2 + mod),
            Form("PedestalSigma Map in M%d", mod + 2),
            nPadsX, -0.5, nPadsX - 0.5,
            nPadsZ, -0.5, nPadsZ - 0.5);
        mHist2D[H2DPedestalSigmaMapInDigitsM2 + mod]->GetXaxis()->SetTitle("x, pad");
        mHist2D[H2DPedestalSigmaMapInDigitsM2 + mod]->GetYaxis()->SetTitle("z, pad");
        mHist2D[H2DPedestalSigmaMapInDigitsM2 + mod]->SetStats(0);
        getObjectsManager()->startPublishing(mHist2D[H2DPedestalSigmaMapInDigitsM2 + mod]);
      } else {
        mHist2D[H2DPedestalSigmaMapInDigitsM2 + mod]->Reset();
      }

      if (!mHist2D[H2DPedestalEfficiencyMapInDigitsM2 + mod]) {
        mHist2D[H2DPedestalEfficiencyMapInDigitsM2 + mod] =
          new TH2F(
            Form("PedestalEfficiencyMapInDigitsM%d", 2 + mod),
            Form("Pedestal Efficiency Map in M%d", mod + 2),
            nPadsX, -0.5, nPadsX - 0.5,
            nPadsZ, -0.5, nPadsZ - 0.5);
        mHist2D[H2DPedestalEfficiencyMapInDigitsM2 + mod]->GetXaxis()->SetTitle("x, pad");
        mHist2D[H2DPedestalEfficiencyMapInDigitsM2 + mod]->GetYaxis()->SetTitle("z, pad");
        mHist2D[H2DPedestalEfficiencyMapInDigitsM2 + mod]->SetStats(0);
        getObjectsManager()->startPublishing(mHist2D[H2DPedestalEfficiencyMapInDigitsM2 + mod]);
      } else {
        mHist2D[H2DPedestalEfficiencyMapInDigitsM2 + mod]->Reset();
      }

      if (!mHist2D[H2DPedestalNPeaksMapInDigitsM2 + mod]) {
        mHist2D[H2DPedestalNPeaksMapInDigitsM2 + mod] =
          new TH2F(
            Form("PedestalNPeaksMapInDigitsM%d", 2 + mod),
            Form("Number of pedestal peaks Map in M%d", mod + 2),
            nPadsX, -0.5, nPadsX - 0.5, nPadsZ, -0.5, nPadsZ - 0.5);
        mHist2D[H2DPedestalNPeaksMapInDigitsM2 + mod]->GetXaxis()->SetTitle("x, pad");
        mHist2D[H2DPedestalNPeaksMapInDigitsM2 + mod]->GetYaxis()->SetTitle("z, pad");
        mHist2D[H2DPedestalNPeaksMapInDigitsM2 + mod]->SetStats(0);
        getObjectsManager()->startPublishing(mHist2D[H2DPedestalNPeaksMapInDigitsM2 + mod]);
      } else {
        mHist2D[H2DPedestalNPeaksMapInDigitsM2 + mod]->Reset();
      }
    }
  }
}

void PedestalTask::fillDigitsHistograms()
{
  LOG(info) << "fillDigitsHistograms()";

  if (!mMonitorDigits) {
    LOG(info) << "fillDigitsHistograms(): monitor digits mode is off. So do nothing.";
    return;
  } else {
    LOG(info) << "fillDigitsHistograms(): starting analyzing digit data ";
  }
  // count pedestals and update MOs
  float pedestalValue, pedestalSigma, pedestalEfficiency;
  short relId[3];
  TF1* functionGaus = new TF1("functionGaus", "gaus", 0., 4095.);
  TSpectrum* peakSearcher = new TSpectrum(5); // find up to 5 pedestal peaks
  int numberOfPeaks;                          // number of pedestal peaks in channel. Normaly it's 1, otherwise channel is bad
  double *xPeaks, yPeaks[5];                  // arrays of x-position of the peaks and their heights

  // first, reset pedestal histograms
  for (int mod = 0; mod < 3; mod++) {
    mHist2D[H2DPedestalNPeaksMapInDigitsM2 + mod]->Reset();
    mHist2D[H2DPedestalValueMapInDigitsM2 + mod]->Reset();
    mHist2D[H2DPedestalSigmaMapInDigitsM2 + mod]->Reset();
    mHist2D[H2DPedestalEfficiencyMapInDigitsM2 + mod]->Reset();
    mHist1D[H1DPedestalValueInDigitsM2 + mod]->Reset();
    mHist1D[H1DPedestalSigmaInDigitsM2 + mod]->Reset();
    mHist1D[H1DPedestalEfficiencyInDigitsM2 + mod]->Reset();
  }

  // then fill them with actual values
  for (int channel = 0; channel < o2::cpv::Geometry::kNCHANNELS; channel++) {
    if (!mHistAmplitudes[channel]) {
      ILOG(Error, Devel) << "fillDigitsHistograms() : histo mHistAmplitudes[" << channel
                         << "] does not exist! Something is going wrong." << ENDM;
      continue;
    }
    if (!mIsUpdatedAmplitude[channel])
      continue; // no data in channel, skipping it

    if (channel % 1000 == 0) {
      ILOG(Info, Devel) << "fillDigitsHistograms(): Start to search peaks in channel " << channel << ENDM;
    }

    numberOfPeaks = peakSearcher->Search(mHistAmplitudes[channel], 10., "nobackground", 0.2);
    xPeaks = peakSearcher->GetPositionX();

    if (numberOfPeaks == 1) { // only 1 peak, fit spectrum with gaus
      yPeaks[0] = mHistAmplitudes[channel]
                    ->GetBinContent(mHistAmplitudes[channel]->GetXaxis()->FindBin(xPeaks[0]));
      functionGaus->SetParameters(yPeaks[0], xPeaks[0], 2.);
      mHistAmplitudes[channel]->Fit(functionGaus, "WWQ", "", xPeaks[0] - 20., xPeaks[0] + 20.);
      pedestalValue = functionGaus->GetParameter(1);
      pedestalSigma = functionGaus->GetParameter(2);
      // stop publish this amplitude as it's OK (that fails due to some unclear reason)
      // if (getObjectsManager()->isBeingPublished(mHistAmplitudes[channel]->GetName())){
      // getObjectsManager()->stopPublishing(mHistAmplitudes[channel]->GetName());
      // }
    } else {
      if (numberOfPeaks > 1) { // >1 peaks, no fit. Just use mean and stddev as ped value & sigma
        pedestalValue = mHistAmplitudes[channel]->GetMean();
        if (pedestalValue > 0)
          pedestalValue = -pedestalValue; // let it be negative so we can know it's bad later
        pedestalSigma = mHistAmplitudes[channel]->GetStdDev();
        // let's publish this bad amplitude
        if (!getObjectsManager()->isBeingPublished(mHistAmplitudes[channel]->GetName())) {
          getObjectsManager()->startPublishing(mHistAmplitudes[channel]);
        }
      } else { // numberOfPeaks < 1 - what is it?
        // no peaks found((( OK let's show the spectrum to the world...
        if (!getObjectsManager()->isBeingPublished(mHistAmplitudes[channel]->GetName())) {
          getObjectsManager()->startPublishing(mHistAmplitudes[channel]);
        }
        continue;
      }
    }

    pedestalEfficiency = mHistAmplitudes[channel]->GetEntries() / mNEventsTotal;
    o2::cpv::Geometry::absToRelNumbering(channel, relId);
    mHist2D[H2DPedestalValueMapInDigitsM2 + relId[0] - 2]
      ->SetBinContent(relId[1] + 1, relId[2] + 1, pedestalValue);
    mHist2D[H2DPedestalSigmaMapInDigitsM2 + relId[0] - 2]
      ->SetBinContent(relId[1] + 1, relId[2] + 1, pedestalSigma);
    mHist2D[H2DPedestalEfficiencyMapInDigitsM2 + relId[0] - 2]
      ->SetBinContent(relId[1] + 1, relId[2] + 1, pedestalEfficiency);
    mHist2D[H2DPedestalNPeaksMapInDigitsM2 + relId[0] - 2]
      ->SetBinContent(relId[1] + 1, relId[2] + 1, numberOfPeaks);

    mHist1D[H1DPedestalValueInDigitsM2 + relId[0] - 2]->Fill(pedestalValue);
    mHist1D[H1DPedestalSigmaInDigitsM2 + relId[0] - 2]->Fill(pedestalSigma);
    mHist1D[H1DPedestalEfficiencyInDigitsM2 + relId[0] - 2]->Fill(pedestalEfficiency);
  }

  // show some info to developer
  ILOG(Info, Devel) << "fillDigitsHistograms() : at this time, N events = " << mNEventsTotal << ENDM;
}

void PedestalTask::resetHistograms()
{
  // clean all histograms
  ILOG(Debug, Devel) << "Resetting amplitude histograms" << ENDM;
  for (int i = 0; i < o2::cpv::Geometry::kNCHANNELS; i++) {
    if (mHistAmplitudes[i]) {
      mHistAmplitudes[i]->Reset();
    }
    mIsUpdatedAmplitude[i] = false;
  }

  ILOG(Debug, Devel) << "Resetting the 1D Histograms" << ENDM;
  for (int iHist1D = 0; iHist1D < kNHist1D; iHist1D++) {
    if (mHist1D[iHist1D]) {
      mHist1D[iHist1D]->Reset();
    }
  }

  ILOG(Debug, Devel) << "Resetting the 2D Histograms" << ENDM;
  for (int iHist2D = 0; iHist2D < kNHist2D; iHist2D++) {
    if (mHist2D[iHist2D]) {
      mHist2D[iHist2D]->Reset();
    }
  }
}

} // namespace o2::quality_control_modules::cpv
