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
/// \file   DigitQcTask.cxx
/// \author Artur Furs afurs@cern.ch
/// modified by Sebastin Bysiak sbysiak@cern.ch

#include "FT0/DigitQcTask.h"
#include "TCanvas.h"
#include "TROOT.h"

#include "QualityControl/QcInfoLogger.h"
#include "DataFormatsFIT/Triggers.h"
#include "Framework/InputRecord.h"
#include "Framework/TimingInfo.h"
#include "DataFormatsFT0/LookUpTable.h"
#include "Common/Utils.h"

#include "FITCommon/HelperHist.h"
#include "FITCommon/HelperCommon.h"

namespace o2::quality_control_modules::ft0
{
using namespace o2::quality_control_modules::fit;
DigitQcTask::~DigitQcTask()
{
  delete mListHistGarbage;
}

void DigitQcTask::rebinFromConfig()
{
  /* Examples:
     "binning_SumAmpC": "100, 0, 100"
     "binning_BcOrbitMap_TrgOrA": "25, 0, 256, 10, 0, 3564"
   hashtag = all channel IDs (mSetAllowedChIDs), e.g.
     "binning_Amp_channel#": "5,-10,90"
   is equivalent to:
     "binning_Amp_channel0": "5,-10,90"
     "binning_Amp_channel1": "5,-10,90"
     "binning_Amp_channel2": "5,-10,90" ...
  */
  auto rebinHisto = [](std::string hName, std::string binning) {
    if (!gROOT->FindObject(hName.data())) {
      ILOG(Warning) << "config: histogram named \"" << hName << "\" not found" << ENDM;
      return;
    }
    std::vector<std::string> tokenizedBinning;
    boost::split(tokenizedBinning, binning, boost::is_any_of(","));
    if (tokenizedBinning.size() == 3) { // TH1
      ILOG(Debug) << "config: rebinning TH1 " << hName << " -> " << binning << ENDM;
      auto htmp = (TH1F*)gROOT->FindObject(hName.data());
      htmp->SetBins(std::atof(tokenizedBinning[0].c_str()), std::atof(tokenizedBinning[1].c_str()), std::atof(tokenizedBinning[2].c_str()));
    } else if (tokenizedBinning.size() == 6) { // TH2
      auto htmp = (TH2F*)gROOT->FindObject(hName.data());
      ILOG(Debug) << "config: rebinning TH2 " << hName << " -> " << binning << ENDM;
      htmp->SetBins(std::atof(tokenizedBinning[0].c_str()), std::atof(tokenizedBinning[1].c_str()), std::atof(tokenizedBinning[2].c_str()),
                    std::atof(tokenizedBinning[3].c_str()), std::atof(tokenizedBinning[4].c_str()), std::atof(tokenizedBinning[5].c_str()));
    } else {
      ILOG(Warning) << "config: invalid binning parameter: " << hName << " -> " << binning << ENDM;
    }
  };

  const std::string rebinKeyword = "binning";
  const char* channelIdPlaceholder = "#";
  try {
    for (auto& param : mCustomParameters.getAllDefaults()) {
      if (param.first.rfind(rebinKeyword, 0) != 0)
        continue;
      std::string hName = param.first.substr(rebinKeyword.length() + 1);
      std::string binning = param.second.c_str();
      if (hName.find(channelIdPlaceholder) != std::string::npos) {
        for (const auto& chID : mSetAllowedChIDs) {
          std::string hNameCur = hName.substr(0, hName.find(channelIdPlaceholder)) + std::to_string(chID) + hName.substr(hName.find(channelIdPlaceholder) + 1);
          rebinHisto(hNameCur, binning);
        }
      } else {
        rebinHisto(hName, binning);
      }
    }
  } catch (std::out_of_range& oor) {
    ILOG(Error) << "Cannot access the default custom parameters : " << oor.what() << ENDM;
  }
}
bool DigitQcTask::chIsVertexEvent(const o2::ft0::ChannelData chd)
{
  return (chd.getFlag(o2::ft0::ChannelData::kIsCFDinADCgate) &&
          !(chd.getFlag(o2::ft0::ChannelData::kIsTimeInfoNOTvalid) || chd.getFlag(o2::ft0::ChannelData::kIsTimeInfoLate) || chd.getFlag(o2::ft0::ChannelData::kIsTimeInfoLost)) &&
          std::abs(static_cast<Int_t>(chd.CFDTime)) < mTrgValidation.mTrgOrGate &&
          !chd.getFlag(o2::ft0::ChannelData::kIsAmpHigh));
}

void DigitQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize DigitQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  mStateLastIR2Ch = {};
  mTrgValidation.configure(mCustomParameters);
  mHistTime2Ch = helper::registerHist<TH2F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "TimePerChannel", "Time vs Channel;Channel;Time", sNCHANNELS_PM, 0, sNCHANNELS_PM, 4100, -2050, 2050);
  mHistAmp2Ch = helper::registerHist<TH2F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "AmpPerChannel", "Amplitude vs Channel;Channel;Amp", sNCHANNELS_PM, 0, sNCHANNELS_PM, 4200, -100, 4100);
  mHistBC = helper::registerHist<TH1F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "BC", "BC;BC;counts;", sBCperOrbit, 0, sBCperOrbit);
  mHistChDataBits = helper::registerHist<TH2F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "ChannelDataBits", "ChannelData bits per ChannelID;Channel;Bit", sNCHANNELS_PM, 0, sNCHANNELS_PM, mMapPMbits);

  // Trg plots
  mHistOrbitVsTrg = helper::registerHist<TH2F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "OrbitVsTriggers", "Orbit vs Triggers;Orbit;Trg", sOrbitsPerTF, 0, sOrbitsPerTF, mMapTechTrgBitsExtra);
  mHistBCvsTrg = helper::registerHist<TH2F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "BCvsTriggers", "BC vs Triggers;BC;Trg", sBCperOrbit, 0, sBCperOrbit, mMapTechTrgBitsExtra);
  mHistTriggersCorrelation = helper::registerHist<TH2F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "TriggersCorrelation", "Correlation of triggers from TCM", mMapTechTrgBitsExtra, mMapTechTrgBitsExtra);

  mHistOrbit2BC = helper::registerHist<TH2F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "OrbitPerBC", "BC-Orbit map;Orbit;BC;", sOrbitsPerTF, 0, sOrbitsPerTF, sBCperOrbit, 0, sBCperOrbit);
  mHistPmTcmNchA = helper::registerHist<TH2F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "PmTcmNumChannelsA", "Comparison of num. channels A from PM and TCM;Number of channels(TCM), side A;PM - TCM", sNCHANNELS_A + 2, 0, sNCHANNELS_A + 2, 2 * sNCHANNELS_A + 1, -int(sNCHANNELS_A) - 0.5, int(sNCHANNELS_A) + 0.5);
  mHistPmTcmSumAmpA = helper::registerHist<TH2F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "PmTcmSumAmpA", "Comparison of sum of amplitudes A from PM and TCM;Sum of amplitudes(TCM), side A;PM - TCM", 2e2, 0, 1e3, 2e3, -1e3 - 0.5, 1e3 - 0.5);
  mHistPmTcmAverageTimeA = helper::registerHist<TH2F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "PmTcmAverageTimeA", "Comparison of average time A from PM and TCM;Average time(TCM), side A;PM - TCM", 410, -2050, 2050, 820, -410 - 0.5, 410 - 0.5);
  mHistPmTcmNchC = helper::registerHist<TH2F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "PmTcmNumChannelsC", "Comparison of num. channels C from PM and TCM;Number of channels(TCM), side C;PM - TCM", sNCHANNELS_C + 2, 0, sNCHANNELS_C + 2, 2 * sNCHANNELS_C + 1, -int(sNCHANNELS_C) - 0.5, int(sNCHANNELS_C) + 0.5);
  mHistPmTcmSumAmpC = helper::registerHist<TH2F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "PmTcmSumAmpC", "Comparison of sum of amplitudes C from PM and TCM;Sum of amplitudes(TCM), side C;PM - TCM", 2e2, 0, 1e3, 2e3, -1e3 - 0.5, 1e3 - 0.5);
  mHistPmTcmAverageTimeC = helper::registerHist<TH2F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "PmTcmAverageTimeC", "Comparison of average time C from PM and TCM;Average time(TCM), side C;PM - TCM", 410, -2050, 2050, 820, -410 - 0.5, 410 - 0.5);

  mHistTriggersSoftwareVsTCM = helper::registerHist<TH2F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "TriggersSoftwareVsTCM", "Trigger validation", mMapTrgBits, mTrgValidation.sMapTrgValidation);

  mListHistGarbage = new TList();
  mListHistGarbage->SetOwner(kTRUE);

  std::map<std::string, uint8_t> mapFEE2hash;
  const auto& lut = o2::ft0::SingleLUT::Instance().getVecMetadataFEE();
  auto lutSorted = lut;
  std::sort(lutSorted.begin(), lutSorted.end(), [](const auto& first, const auto& second) { return first.mModuleName < second.mModuleName; });
  uint8_t binPos{ 0 };
  for (const auto& lutEntry : lutSorted) {
    const auto& moduleName = lutEntry.mModuleName;
    const auto& moduleType = lutEntry.mModuleType;
    const auto& strChID = lutEntry.mChannelID;
    const auto& pairIt = mapFEE2hash.insert({ moduleName, binPos });
    if (pairIt.second) {
      if (moduleName.find("PMA") != std::string::npos)
        mMapPMhash2isAside.insert({ binPos, true });
      else if (moduleName.find("PMC") != std::string::npos)
        mMapPMhash2isAside.insert({ binPos, false });
      binPos++;
    }
    if (std::regex_match(strChID, std::regex("[\\d]{1,3}"))) {
      int chID = std::stoi(strChID);
      if (chID < sNCHANNELS_PM) {
        mChID2PMhash[chID] = mapFEE2hash[moduleName];
      } else {
        LOG(error) << "Incorrect LUT entry: chID " << strChID << " | " << moduleName;
      }
    } else if (moduleType != "TCM") {
      LOG(error) << "Non-TCM module w/o numerical chID: chID " << strChID << " | " << moduleName;
    } else if (moduleType == "TCM") {
      mTCMhash = mapFEE2hash[moduleName];
    }
  }

  std::map<unsigned int, std::string> mapBin2ModuleName;
  std::transform(mapFEE2hash.begin(), mapFEE2hash.end(), std::inserter(mapBin2ModuleName, std::end(mapBin2ModuleName)), [](const auto& entry) { return std::pair<unsigned int, std::string>{ static_cast<unsigned int>(entry.second), entry.first }; });

  mHistBCvsFEEmodules = helper::registerHist<TH2F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "BCvsFEEmodules", "BC vs FEE module;BC;FEE", sBCperOrbit, 0, sBCperOrbit, mapBin2ModuleName);
  mHistOrbitVsFEEmodules = helper::registerHist<TH2F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "OrbitVsFEEmodules", "Orbit vs FEE module;Orbit;FEE", sOrbitsPerTF, 0, sOrbitsPerTF, mapBin2ModuleName);

  mHistTimeSum2Diff = helper::registerHist<TH2F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "timeSumVsDiff", "time A/C side: sum VS diff;(TOC-TOA)/2 [ns];(TOA+TOC)/2 [ns]", 2000, -52.08, 52.08, 2000, -52.08, 52.08); // range of 52.08 ns = 4000*13.02ps = 4000 channels
  mHistTimeSum2Diff->GetXaxis()->SetRangeUser(-5, 5);
  mHistTimeSum2Diff->GetYaxis()->SetRangeUser(-5, 5);
  mHistNchA = helper::registerHist<TH1F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "NumChannelsA", "Number of channels(TCM), side A;Nch", sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mHistNchC = helper::registerHist<TH1F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "NumChannelsC", "Number of channels(TCM), side C;Nch", sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mHistSumAmpA = helper::registerHist<TH1F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "SumAmpA", "Sum of amplitudes(TCM), side A;", 1e4, 0, 1e4);
  mHistSumAmpC = helper::registerHist<TH1F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "SumAmpC", "Sum of amplitudes(TCM), side C;", 1e4, 0, 1e4);
  mHistAverageTimeA = helper::registerHist<TH1F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "AverageTimeA", "Average time(TCM), side A", 4100, -2050, 2050);
  mHistAverageTimeC = helper::registerHist<TH1F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "AverageTimeC", "Average time(TCM), side C", 4100, -2050, 2050);
  mHistChannelID = helper::registerHist<TH1F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "StatChannelID", "ChannelID statistics;ChannelID", sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mHistCycleDuration = helper::registerHist<TH1D>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "CycleDuration", "Cycle Duration;;time [ns]", 1, 0, 2);
  mHistCycleDurationNTF = helper::registerHist<TH1D>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "CycleDurationNTF", "Cycle Duration;;time [TimeFrames]", 1, 0, 2);
  mHistCycleDurationRange = helper::registerHist<TH1D>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "CycleDurationRange", "Cycle Duration (total cycle range);;time [ns]", 1, 0, 2);

  std::vector<unsigned int> vecChannelIDs;
  if (auto param = mCustomParameters.find("ChannelIDs"); param != mCustomParameters.end()) {
    const auto chIDs = param->second;
    const std::string del = ",";
    vecChannelIDs = helper::parseParameters<unsigned int>(chIDs, del);
  }
  for (const auto& entry : vecChannelIDs) {
    mSetAllowedChIDs.insert(entry);
  }
  std::vector<unsigned int> vecChannelIDsAmpVsTime;
  if (auto param = mCustomParameters.find("ChannelIDsAmpVsTime"); param != mCustomParameters.end()) {
    const auto chIDs = param->second;
    const std::string del = ",";
    vecChannelIDsAmpVsTime = helper::parseParameters<unsigned int>(chIDs, del);
  }
  for (const auto& entry : vecChannelIDsAmpVsTime) {
    mSetAllowedChIDsAmpVsTime.insert(entry);
  }

  for (const auto& chID : mSetAllowedChIDs) {
    auto pairHistAmp = mMapHistAmp1D.insert({ chID, new TH1F(Form("Amp_channel%i", chID), Form("Amplitude, channel %i", chID), 4200, -100, 4100) });
    auto pairHistTime = mMapHistTime1D.insert({ chID, new TH1F(Form("Time_channel%i", chID), Form("Time, channel %i", chID), 4100, -2050, 2050) });
    auto pairHistBits = mMapHistPMbits.insert({ chID, new TH1F(Form("Bits_channel%i", chID), Form("Bits, channel %i", chID), mMapPMbits.size(), 0, mMapPMbits.size()) });
    for (const auto& entry : mMapPMbits) {
      pairHistBits.first->second->GetXaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());
    }
    if (pairHistAmp.second) {
      getObjectsManager()->startPublishing(pairHistAmp.first->second);
      mListHistGarbage->Add(pairHistAmp.first->second);
    }
    if (pairHistTime.second) {
      mListHistGarbage->Add(pairHistTime.first->second);
      getObjectsManager()->startPublishing(pairHistTime.first->second);
    }
    if (pairHistBits.second) {
      mListHistGarbage->Add(pairHistBits.first->second);
      getObjectsManager()->startPublishing(pairHistBits.first->second);
    }
  }
  for (const auto& chID : mSetAllowedChIDsAmpVsTime) {
    auto pairHistAmpVsTime = mMapHistAmpVsTime.insert({ chID, new TH2F(Form("Amp_vs_time_channel%i", chID), Form("Amplitude vs time, channel %i;Amp;Time", chID), 420, -100, 4100, 410, -2050, 2050) });
    if (pairHistAmpVsTime.second) {
      mListHistGarbage->Add(pairHistAmpVsTime.first->second);
      getObjectsManager()->startPublishing(pairHistAmpVsTime.first->second);
    }
  }

  rebinFromConfig(); // after all histos are created

  for (int i = 0; i < getObjectsManager()->getNumberPublishedObjects(); i++) {
    TH1* obj = dynamic_cast<TH1*>(getObjectsManager()->getMonitorObject(i)->getObject());
    obj->SetTitle((string("FT0 ") + obj->GetTitle()).c_str());
  }
  //
  mGoodPMbits_ChID = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "goodPMbits_ChID", 1 << o2::ft0::ChannelData::kIsCFDinADCgate);
  mBadPMbits_ChID = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "badPMbits_ChID", (1 << o2::ft0::ChannelData::kIsTimeInfoNOTvalid) | (1 << o2::ft0::ChannelData::kIsTimeInfoLate) | (1 << o2::ft0::ChannelData::kIsAmpHigh) | (1 << o2::ft0::ChannelData::kIsTimeInfoLost));
  mPMbitsToCheck_ChID = mBadPMbits_ChID | mGoodPMbits_ChID;
  mLowTimeGate_ChID = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "lowTimeGate_ChID", -192);
  mUpTimeGate_ChID = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "upTimeGate_ChID", 192);
  mHistChIDperBC = helper::registerHist<TH2F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "ChannelIDperBC", Form("FT0 ChannelID per BC, bad PM bit suppression %i, good PM checking %i, gate (%i,%i)", mBadPMbits_ChID, mGoodPMbits_ChID, mLowTimeGate_ChID, mUpTimeGate_ChID), sBCperOrbit, 0, sBCperOrbit, sNCHANNELS_PM, 0, sNCHANNELS_PM);

  // Timestamp
  mMetaAnchorOutput = o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "metaAnchorOutput", "CycleDurationNTF");
  mTimestampMetaField = o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "timestampMetaField", "timestampTF");
}

void DigitQcTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity" << activity.mId << ENDM;
  mHistTime2Ch->Reset();
  mHistAmp2Ch->Reset();
  mHistBC->Reset();
  mHistChDataBits->Reset();
  mHistTimeSum2Diff->Reset();
  mHistBCvsFEEmodules->Reset();
  mHistOrbitVsTrg->Reset();
  mHistOrbitVsFEEmodules->Reset();
  mHistTriggersCorrelation->Reset();
  mHistCycleDuration->Reset();
  mHistCycleDurationNTF->Reset();
  mHistCycleDurationRange->Reset();
  mHistBCvsTrg->Reset();
  mHistOrbit2BC->Reset();
  mHistNchA->Reset();
  mHistNchC->Reset();
  mHistSumAmpA->Reset();
  mHistSumAmpC->Reset();
  mHistAverageTimeA->Reset();
  mHistAverageTimeC->Reset();
  mHistChannelID->Reset();
  mHistPmTcmNchA->Reset();
  mHistPmTcmSumAmpA->Reset();
  mHistPmTcmAverageTimeA->Reset();
  mHistPmTcmNchC->Reset();
  mHistPmTcmSumAmpC->Reset();
  mHistPmTcmAverageTimeC->Reset();
  mHistTriggersSoftwareVsTCM->Reset();
  mHistChIDperBC->Reset();
  for (auto& entry : mMapHistAmp1D) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistTime1D) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistPMbits) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistAmpVsTime) {
    entry.second->Reset();
  }
}

void DigitQcTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
  mTimeMinNS = -1;
  mTimeMaxNS = 0.;
  mTimeCurNS = 0.;
  mTfCounter = 0;
  mTimeSum = 0.;
}

void DigitQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  mTFcreationTime = ctx.services().get<o2::framework::TimingInfo>().creation;
  mTfCounter++;
  auto channels = ctx.inputs().get<gsl::span<o2::ft0::ChannelData>>("channels");
  auto digits = ctx.inputs().get<gsl::span<o2::ft0::Digit>>("digits");
  if (digits.size() > 0) {
    // Considering that Digit container is already sorted by IR
    const o2::InteractionRecord& firstIR = digits[0].getIntRecord();
    const o2::InteractionRecord& lastIR = digits[digits.size() - 1].getIntRecord();
    auto timeMinNS = firstIR.bc2ns();
    auto timeMaxNS = lastIR.bc2ns();
    mTimeMinNS = std::min(mTimeMinNS, timeMinNS); // to be sure if somehow TFID is unordered
    mTimeMaxNS = std::max(mTimeMaxNS, timeMaxNS);
    mTimeSum += timeMaxNS - timeMinNS;
  }
  for (auto& digit : digits) {
    // Exclude all BCs, in which laser signals are expected (and trigger outputs are blocked)
    const auto& vecChData = digit.getBunchChannelData(channels);
    bool isTCM = true;
    if (digit.mTriggers.getTimeA() == o2::fit::Triggers::DEFAULT_TIME && digit.mTriggers.getTimeC() == o2::fit::Triggers::DEFAULT_TIME) {
      isTCM = false;
    }
    mHistOrbit2BC->Fill(digit.getIntRecord().orbit % sOrbitsPerTF, digit.getIntRecord().bc);
    mHistBC->Fill(digit.getBC());

    std::set<uint8_t> setFEEmodules{};

    int32_t pmSumAmplA = 0;
    int32_t pmSumAmplC = 0;
    uint8_t pmNChanA{ 0 };
    uint8_t pmNChanC{ 0 };
    int pmSumTimeA{ 0 };
    int pmSumTimeC{ 0 };
    int pmAverTimeA{ 0 };
    int pmAverTimeC{ 0 };

    std::map<uint8_t, int> mapPMhash2sumAmpl;
    for (const auto& entry : mMapPMhash2isAside) {
      mapPMhash2sumAmpl.insert({ entry.first, 0 });
    }
    for (const auto& chData : vecChData) {
      mHistTime2Ch->Fill(static_cast<Double_t>(chData.ChId), static_cast<Double_t>(chData.CFDTime));
      mHistAmp2Ch->Fill(static_cast<Double_t>(chData.ChId), static_cast<Double_t>(chData.QTCAmpl));
      mStateLastIR2Ch[chData.ChId] = digit.mIntRecord;
      mHistChannelID->Fill(chData.ChId);
      if (mSetAllowedChIDs.size() != 0 && mSetAllowedChIDs.find(static_cast<unsigned int>(chData.ChId)) != mSetAllowedChIDs.end()) {
        mMapHistAmp1D[chData.ChId]->Fill(chData.QTCAmpl);
        mMapHistTime1D[chData.ChId]->Fill(chData.CFDTime);
        for (const auto& entry : mMapPMbits) {
          if ((chData.ChainQTC & (1 << entry.first))) {
            mMapHistPMbits[chData.ChId]->Fill(entry.first);
          }
        }
      }
      if (mSetAllowedChIDsAmpVsTime.size() != 0 && mSetAllowedChIDsAmpVsTime.find(static_cast<unsigned int>(chData.ChId)) != mSetAllowedChIDsAmpVsTime.end()) {
        mMapHistAmpVsTime[chData.ChId]->Fill(chData.QTCAmpl, chData.CFDTime);
      }
      for (const auto& binPos : mHashedBitBinPos[chData.ChainQTC]) {
        mHistChDataBits->Fill(chData.ChId, binPos);
      }

      setFEEmodules.insert(mChID2PMhash[chData.ChId]);

      if (((chData.ChainQTC & mPMbitsToCheck_ChID) == mGoodPMbits_ChID) && std::abs(static_cast<Int_t>(chData.CFDTime)) < mTrgValidation.mTrgOrGate) {
        if (!mMapPMhash2isAside[mChID2PMhash[static_cast<uint8_t>(chData.ChId)]]) {
          pmSumTimeC += chData.CFDTime;
          pmNChanC++;
        } else if (mMapPMhash2isAside[mChID2PMhash[static_cast<uint8_t>(chData.ChId)]]) {
          pmSumTimeA += chData.CFDTime;
          pmNChanA++;
        }
        mHistChIDperBC->Fill(digit.getBC(), chData.ChId);
      }
      if (chData.getFlag(o2::ft0::ChannelData::kIsCFDinADCgate)) {
        mapPMhash2sumAmpl[mChID2PMhash[static_cast<uint8_t>(chData.ChId)]] += static_cast<Int_t>(chData.QTCAmpl);
      }
    }

    for (const auto& entry : mapPMhash2sumAmpl) {
      if (mMapPMhash2isAside[entry.first])
        pmSumAmplA += (entry.second >> 3);
      else
        pmSumAmplC += (entry.second >> 3);
    }

    if (isTCM) {
      if (pmNChanA > 1) {
        pmAverTimeA = std::floor((float)pmSumTimeA / pmNChanA);
      } else if (pmNChanA == 1) {
        pmAverTimeA = pmSumTimeA;
      } else {
        pmAverTimeA = 0;
      }
      if (pmNChanC > 1) {
        pmAverTimeC = std::floor((float)pmSumTimeC / pmNChanC);
      } else if (pmNChanC == 1) {
        pmAverTimeC = pmSumTimeC;
      } else {
        pmAverTimeC = 0;
      }
      setFEEmodules.insert(mTCMhash);

    } else {
      pmAverTimeA = o2::fit::Triggers::DEFAULT_TIME;
      pmAverTimeC = o2::fit::Triggers::DEFAULT_TIME;
    }
    auto vtxPos = (pmNChanA && pmNChanC) ? (pmAverTimeC - pmAverTimeA) / 2 : 0;

    for (const auto& feeHash : setFEEmodules) {
      mHistBCvsFEEmodules->Fill(static_cast<double>(digit.getIntRecord().bc), static_cast<double>(feeHash));
      mHistOrbitVsFEEmodules->Fill(static_cast<double>(digit.getIntRecord().orbit % sOrbitsPerTF), static_cast<double>(feeHash));
    }

    if (isTCM && digit.mTriggers.getDataIsValid() && !digit.mTriggers.getOutputsAreBlocked()) {
      if (digit.mTriggers.getNChanA() > 0) {
        mHistNchA->Fill(digit.mTriggers.getNChanA());
        mHistSumAmpA->Fill(digit.mTriggers.getAmplA());
        mHistAverageTimeA->Fill(digit.mTriggers.getTimeA());
      }
      if (digit.mTriggers.getNChanC() > 0) {
        mHistNchC->Fill(digit.mTriggers.getNChanC());
        mHistSumAmpC->Fill(digit.mTriggers.getAmplC());
        mHistAverageTimeC->Fill(digit.mTriggers.getTimeC());
      }
      mHistPmTcmNchA->Fill(digit.mTriggers.getNChanA(), pmNChanA - digit.mTriggers.getNChanA());
      mHistPmTcmSumAmpA->Fill(digit.mTriggers.getAmplA(), pmSumAmplA - digit.mTriggers.getAmplA());
      mHistPmTcmAverageTimeA->Fill(digit.mTriggers.getTimeA(), pmAverTimeA - digit.mTriggers.getTimeA());
      mHistPmTcmNchC->Fill(digit.mTriggers.getNChanC(), pmNChanC - digit.mTriggers.getNChanC());
      mHistPmTcmSumAmpC->Fill(digit.mTriggers.getAmplC(), pmSumAmplC - digit.mTriggers.getAmplC());
      mHistPmTcmAverageTimeC->Fill(digit.mTriggers.getTimeC(), pmAverTimeC - digit.mTriggers.getTimeC());

      mHistTimeSum2Diff->Fill((digit.mTriggers.getTimeC() - digit.mTriggers.getTimeA()) * sCFDChannel2NS / 2, (digit.mTriggers.getTimeC() + digit.mTriggers.getTimeA()) * sCFDChannel2NS / 2);
    }
    if (isTCM) {
      std::vector<unsigned int> vecTrgWords{};
      const uint64_t trgWordExt = digit.mTriggers.getExtendedTrgWordFT0();
      for (const auto& entry : mMapTechTrgBitsExtra) {
        const auto& trgBit = entry.first;
        if (((1 << trgBit) & trgWordExt) > 0) {
          mHistTriggersCorrelation->Fill(trgBit, trgBit);
          for (const auto& prevTrgBit : vecTrgWords) {
            mHistTriggersCorrelation->Fill(trgBit, prevTrgBit);
          }
          mHistBCvsTrg->Fill(digit.getIntRecord().bc, trgBit);
          mHistOrbitVsTrg->Fill(digit.getIntRecord().orbit % sOrbitsPerTF, trgBit);
          vecTrgWords.push_back(trgBit);
        }
      }
    }
    // trigger emulation
    DataTCM_t tcmEmu(pmSumAmplA, pmSumAmplC, pmSumTimeA, pmSumTimeC, pmNChanA, pmNChanC);
    mTrgValidation.emulateTriggers(tcmEmu); // emulate trigger
    const auto& trg = digit.mTriggers.getTriggersignals();
    for (int iTrgBit = 0; iTrgBit < mMapTrgBits.size(); iTrgBit++) {
      const auto status = mTrgValidation.getTrgValidationStatus(trg, tcmEmu.triggersignals, iTrgBit);
      mHistTriggersSoftwareVsTCM->Fill(iTrgBit, status);
    }
    // end of triggers re-computation
  }
}

void DigitQcTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
  // add TF creation time for further match with filling scheme in PP in case of offline running
  ILOG(Debug, Support) << "adding last TF creation time: " << mTFcreationTime << ENDM;
  getObjectsManager()->getMonitorObject(mMetaAnchorOutput)->addOrUpdateMetadata(mTimestampMetaField, std::to_string(mTFcreationTime));

  // one has to set num. of entries manually because
  // default TH1Reductor gets only mean,stddev and entries (no integral)
  mHistCycleDurationRange->SetBinContent(1., mTimeMaxNS - mTimeMinNS);
  mHistCycleDurationRange->SetEntries(mTimeMaxNS - mTimeMinNS);
  mHistCycleDurationNTF->SetBinContent(1., mTfCounter);
  mHistCycleDurationNTF->SetEntries(mTfCounter);
  mHistCycleDuration->SetBinContent(1., mTimeSum);
  mHistCycleDuration->SetEntries(mTimeSum);
  ILOG(Debug) << "Cycle duration: NTF=" << mTfCounter << ", range = " << (mTimeMaxNS - mTimeMinNS) / 1e6 / mTfCounter << " ms/TF, sum = " << mTimeSum / 1e6 / mTfCounter << " ms/TF" << ENDM;
}

void DigitQcTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void DigitQcTask::reset()
{
  // clean all the monitor objects here
  mHistTime2Ch->Reset();
  mHistAmp2Ch->Reset();
  mHistBC->Reset();
  mHistChDataBits->Reset();
  mHistTimeSum2Diff->Reset();
  mHistOrbit2BC->Reset();
  mHistNchA->Reset();
  mHistNchC->Reset();
  mHistSumAmpA->Reset();
  mHistSumAmpC->Reset();
  mHistAverageTimeA->Reset();
  mHistAverageTimeC->Reset();
  mHistChannelID->Reset();
  mHistTriggersCorrelation->Reset();
  mHistCycleDuration->Reset();
  mHistCycleDurationNTF->Reset();
  mHistCycleDurationRange->Reset();
  mHistBCvsTrg->Reset();
  mHistBCvsFEEmodules->Reset();
  mHistOrbitVsTrg->Reset();
  mHistOrbitVsFEEmodules->Reset();
  mHistPmTcmNchA->Reset();
  mHistPmTcmSumAmpA->Reset();
  mHistPmTcmAverageTimeA->Reset();
  mHistPmTcmNchC->Reset();
  mHistPmTcmSumAmpC->Reset();
  mHistPmTcmAverageTimeC->Reset();
  mHistTriggersSoftwareVsTCM->Reset();
  mHistChIDperBC->Reset();
  for (auto& entry : mMapHistAmp1D) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistTime1D) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistPMbits) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistAmpVsTime) {
    entry.second->Reset();
  }
}

} // namespace o2::quality_control_modules::ft0
