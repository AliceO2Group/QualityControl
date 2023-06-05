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
/// LATEST modification for FDD on 25.04.2023 (akhuntia@cern.ch)

#include "FDD/DigitQcTask.h"
#include "TCanvas.h"
#include "TROOT.h"

#include "QualityControl/QcInfoLogger.h"
#include "DataFormatsFIT/Triggers.h"
#include "Framework/InputRecord.h"
#include "Framework/InputRecordWalker.h"
#include "Framework/TimingInfo.h"
#include "DataFormatsFDD/LookUpTable.h"
#include "DataFormatsFDD/ChannelData.h"
#include "DataFormatsFDD/Digit.h"

namespace o2::quality_control_modules::fdd
{

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
}

unsigned int DigitQcTask::getModeParameter(std::string paramName, unsigned int defaultVal, std::map<unsigned int, std::string> choices)
{
  if (auto param = mCustomParameters.find(paramName); param != mCustomParameters.end()) {
    // if parameter was provided check which option was chosen
    for (const auto& choice : choices) {
      if (param->second == choice.second) {
        ILOG(Debug, Support) << "setting \"" << paramName << "\" to: \"" << choice.second << "\"" << ENDM;
        return choice.first;
      }
    }
    // param value not allowed - use default but with warning
    std::string allowedValues;
    for (const auto& choice : choices) {
      allowedValues += "\"";
      allowedValues += choice.second;
      allowedValues += "\", ";
    }
    ILOG(Warning, Support) << "Provided value (\"" << param->second << "\") for parameter \"" << paramName << "\" is not allowed. Allowed values are: " << allowedValues << " setting \"" << paramName << "\" to default value: \"" << choices[defaultVal] << "\"" << ENDM;
    return defaultVal;
  } else {
    // param not provided - use default
    ILOG(Debug, Support) << "Setting \"" << paramName << "\" to default value: \"" << choices[defaultVal] << "\"" << ENDM;
    return defaultVal;
  }
}

int DigitQcTask::getNumericalParameter(std::string paramName, int defaultVal)
{
  if (auto param = mCustomParameters.find(paramName); param != mCustomParameters.end()) {
    float val = stoi(param->second);
    ILOG(Debug, Support) << "Setting \"" << paramName << "\" to: " << val << ENDM;
    return val;
  } else {
    ILOG(Debug, Support) << "Setting \"" << paramName << "\" to default value: " << defaultVal << ENDM;
    return defaultVal;
  }
}

bool DigitQcTask::chIsVertexEvent(const o2::fdd::ChannelData chd)
{
  return (chd.getFlag(o2::fdd::ChannelData::kIsCFDinADCgate) &&
          !(chd.getFlag(o2::fdd::ChannelData::kIsTimeInfoNOTvalid) || chd.getFlag(o2::fdd::ChannelData::kIsTimeInfoLate) || chd.getFlag(o2::fdd::ChannelData::kIsTimeInfoLost)) &&
          std::abs(static_cast<Int_t>(chd.mTime)) < mTrgOrGate &&
          static_cast<Int_t>(chd.mChargeADC) > mTrgChargeLevelLow &&
          static_cast<Int_t>(chd.mChargeADC) < mTrgChargeLevelHigh);
}

void DigitQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize DigitQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  mStateLastIR2Ch = {};
  mMapChTrgNames.insert({ o2::fdd::ChannelData::EEventDataBit::kNumberADC, "NumberADC" });
  mMapChTrgNames.insert({ o2::fdd::ChannelData::EEventDataBit::kIsDoubleEvent, "IsDoubleEvent" });
  mMapChTrgNames.insert({ o2::fdd::ChannelData::EEventDataBit::kIsTimeInfoNOTvalid, "IsTimeInfoNOTvalid" });
  mMapChTrgNames.insert({ o2::fdd::ChannelData::EEventDataBit::kIsCFDinADCgate, "IsCFDinADCgate" });
  mMapChTrgNames.insert({ o2::fdd::ChannelData::EEventDataBit::kIsTimeInfoLate, "IsTimeInfoLate" });
  mMapChTrgNames.insert({ o2::fdd::ChannelData::EEventDataBit::kIsAmpHigh, "IsAmpHigh" });
  mMapChTrgNames.insert({ o2::fdd::ChannelData::EEventDataBit::kIsEventInTVDC, "IsEventInTVDC" });
  mMapChTrgNames.insert({ o2::fdd::ChannelData::EEventDataBit::kIsTimeInfoLost, "IsTimeInfoLost" });

  mMapDigitTrgNames.insert({ o2::fit::Triggers::bitA, "OrA" });
  mMapDigitTrgNames.insert({ o2::fit::Triggers::bitC, "OrC" });
  mMapDigitTrgNames.insert({ o2::fit::Triggers::bitVertex, "Vertex" });
  mMapDigitTrgNames.insert({ o2::fit::Triggers::bitCen, "Central" });
  mMapDigitTrgNames.insert({ o2::fit::Triggers::bitSCen, "SemiCentral" });
  mMapDigitTrgNames.insert({ o2::fit::Triggers::bitLaser, "Laser" });
  mMapDigitTrgNames.insert({ o2::fit::Triggers::bitOutputsAreBlocked, "OutputsAreBlocked" });
  mMapDigitTrgNames.insert({ o2::fit::Triggers::bitDataIsValid, "DataIsValid" });

  mMapTrgSoftware.insert({ o2::fit::Triggers::bitA, false });
  mMapTrgSoftware.insert({ o2::fit::Triggers::bitC, false });
  mMapTrgSoftware.insert({ o2::fit::Triggers::bitVertex, false });
  mMapTrgSoftware.insert({ o2::fit::Triggers::bitCen, false });
  mMapTrgSoftware.insert({ o2::fit::Triggers::bitSCen, false });

  mTrgModeThresholdVar = getModeParameter("trgModeThresholdVar",
                                          TrgModeThresholdVar::kAmpl,
                                          { { TrgModeThresholdVar::kAmpl, "Ampl" },
                                            { TrgModeThresholdVar::kNchannels, "Nchannels" } });
  mTrgModeSide = getModeParameter("trgModeSide",
                                  TrgModeSide::kAplusC,
                                  { { TrgModeSide::kAplusC, "A+C" },
                                    { TrgModeSide::kAandC, "A&C" },
                                    { TrgModeSide::kA, "A" },
                                    { TrgModeSide::kC, "C" } });
  mTrgOrGate = getNumericalParameter("trgOrGate", 153);
  mTrgChargeLevelLow = getNumericalParameter("trgChargeLevelLow", 0);
  mTrgChargeLevelHigh = getNumericalParameter("trgChargeLevelHigh", 4095);
  mTrgThresholdTimeLow = getNumericalParameter("trgThresholdTimeLow", -192);
  mTrgThresholdTimeHigh = getNumericalParameter("trgThresholdTimeHigh", 192);
  mBinMinADCSaturationCheck = getNumericalParameter("BinMinADCSaturationCheck", 1);
  mBinMaxADCSaturationCheck = getNumericalParameter("BinMaxADCSaturationCheck", 3600);
  mMinTimeGate = getNumericalParameter("minGateTimeForRatioHistogram", -192);
  mMaxTimeGate = getNumericalParameter("maxGateTimeForRatioHistogram", 192);
  if (mTrgModeSide == TrgModeSide::kAplusC) {
    mTrgThresholdCenA = getNumericalParameter("trgThresholdCenA", 20);
    mTrgThresholdSCenA = getNumericalParameter("trgThresholdSCenA", 10);
  } else if (mTrgModeSide == TrgModeSide::kAandC) {
    mTrgThresholdCenA = getNumericalParameter("trgThresholdCenA", 20);
    mTrgThresholdCenC = getNumericalParameter("trgThresholdCenC", 20);
    mTrgThresholdSCenA = getNumericalParameter("trgThresholdSCenA", 10);
    mTrgThresholdSCenC = getNumericalParameter("trgThresholdSCenC", 10);
  } else if (mTrgModeSide == TrgModeSide::kA) {
    mTrgThresholdCenA = getNumericalParameter("trgThresholdCenA", 20);
    mTrgThresholdSCenA = getNumericalParameter("trgThresholdSCenA", 10);
  } else if (mTrgModeSide == TrgModeSide::kC) {
    mTrgThresholdCenC = getNumericalParameter("trgThresholdCenC", 20);
    mTrgThresholdSCenC = getNumericalParameter("trgThresholdSCenC", 10);
  }

  mHistTime2Ch = std::make_unique<TH2F>("TimePerChannel", "Time vs Channel;Channel;Time", sNCHANNELS_PM, 0, sNCHANNELS_PM, 4100, -2050, 2050);
  mHistTime2Ch->SetOption("colz");
  mHistAmp2Ch = std::make_unique<TH2F>("AmpPerChannel", "Amplitude vs Channel;Channel;Amp", sNCHANNELS_PM, 0, sNCHANNELS_PM, 4200, -100, 4100);
  mHistAmp2Ch->SetOption("colz");
  mHistBC = std::make_unique<TH1F>("BC", "BC;BC;counts;", sBCperOrbit, 0, sBCperOrbit);
  mHistChDataBits = std::make_unique<TH2F>("ChannelDataBits", "ChannelData bits per ChannelID;Channel;Bit", sNCHANNELS_PM, 0, sNCHANNELS_PM, mMapChTrgNames.size(), 0, mMapChTrgNames.size());
  mHistChDataBits->SetOption("colz");
  for (const auto& entry : mMapChTrgNames) {
    mHistChDataBits->GetYaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());
  }
  mHistOrbitVsTrg = std::make_unique<TH2F>("OrbitVsTriggers", "Orbit vs Triggers;Orbit;Trg", sOrbitsPerTF, 0, sOrbitsPerTF, mMapDigitTrgNames.size(), 0, mMapDigitTrgNames.size());
  mHistOrbitVsTrg->SetOption("colz");
  mHistOrbit2BC = std::make_unique<TH2F>("OrbitPerBC", "BC-Orbit map;Orbit;BC;", sOrbitsPerTF, 0, sOrbitsPerTF, sBCperOrbit, 0, sBCperOrbit);
  mHistOrbit2BC->SetOption("colz");
  mHistEventDensity2Ch = std::make_unique<TH2F>("EventDensityPerChannel", "Event density(in BC) per Channel;Channel;BC;", sNCHANNELS_PM, 0, sNCHANNELS_PM, 10000, 0, 1e5);
  mHistEventDensity2Ch->SetOption("colz");
  mHistTriggersCorrelation = std::make_unique<TH2F>("TriggersCorrelation", "Correlation of triggers from TCM", mMapDigitTrgNames.size(), 0, mMapDigitTrgNames.size(), mMapDigitTrgNames.size(), 0, mMapDigitTrgNames.size());
  mHistTriggersCorrelation->SetOption("colz");
  mHistBCvsTrg = std::make_unique<TH2F>("BCvsTriggers", "BC vs Triggers;BC;Trg", sBCperOrbit, 0, sBCperOrbit, mMapDigitTrgNames.size(), 0, mMapDigitTrgNames.size());
  mHistBCvsTrg->SetOption("colz");
  mHistPmTcmNchA = std::make_unique<TH2F>("PmTcmNumChannelsA", "Comparison of num. channels A from PM and TCM;Number of channels(TCM), side A;PM - TCM", sNCHANNELS_A + 2, 0, sNCHANNELS_A + 2, 2 * sNCHANNELS_A + 1, -int(sNCHANNELS_A) - 0.5, int(sNCHANNELS_A) + 0.5);
  mHistPmTcmSumAmpA = std::make_unique<TH2F>("PmTcmSumAmpA", "Comparison of sum of amplitudes A from PM and TCM;Sum of amplitudes(TCM), side A;PM - TCM", 2e2, 0, 1e3, 2e3, -1e3 - 0.5, 1e3 - 0.5);
  mHistPmTcmAverageTimeA = std::make_unique<TH2F>("PmTcmAverageTimeA", "Comparison of average time A from PM and TCM;Average time(TCM), side A;PM - TCM", 410, -2050, 2050, 820, -410 - 0.5, 410 - 0.5);
  mHistPmTcmNchC = std::make_unique<TH2F>("PmTcmNumChannelsC", "Comparison of num. channels C from PM and TCM;Number of channels(TCM), side C;PM - TCM", sNCHANNELS_C + 2, 0, sNCHANNELS_C + 2, 2 * sNCHANNELS_C + 1, -int(sNCHANNELS_C) - 0.5, int(sNCHANNELS_C) + 0.5);
  mHistPmTcmSumAmpC = std::make_unique<TH2F>("PmTcmSumAmpC", "Comparison of sum of amplitudes C from PM and TCM;Sum of amplitudes(TCM), side C;PM - TCM", 2e2, 0, 1e3, 2e3, -1e3 - 0.5, 1e3 - 0.5);
  mHistPmTcmAverageTimeC = std::make_unique<TH2F>("PmTcmAverageTimeC", "Comparison of average time C from PM and TCM;Average time(TCM), side C;PM - TCM", 410, -2050, 2050, 820, -410 - 0.5, 410 - 0.5);
  mHistTriggersSw = std::make_unique<TH1F>("TriggersSoftware", "Triggers from software", mMapDigitTrgNames.size(), 0, mMapDigitTrgNames.size());
  mHistTriggersSoftwareVsTCM = std::make_unique<TH2F>("TriggersSoftwareVsTCM", "Comparison of triggers from software and TCM;;Trigger name", mMapDigitTrgNames.size(), 0, mMapDigitTrgNames.size(), 4, 0, 4);
  mHistTriggersSoftwareVsTCM->SetOption("colz");
  mHistTriggersSoftwareVsTCM->SetStats(0);
  for (const auto& entry : mMapDigitTrgNames) {
    mHistOrbitVsTrg->GetYaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());
    mHistTriggersCorrelation->GetXaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());
    mHistTriggersCorrelation->GetYaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());
    mHistBCvsTrg->GetYaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());
    mHistTriggersSw->GetXaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());
    mHistTriggersSoftwareVsTCM->GetXaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());
  }
  mHistTriggersSw->GetXaxis()->SetRange(1, 5);
  mHistTriggersSoftwareVsTCM->GetXaxis()->SetRange(1, 5);
  mHistTriggersSoftwareVsTCM->GetYaxis()->SetBinLabel(TrgComparisonResult::kSWonly + 1, "Sw only");
  mHistTriggersSoftwareVsTCM->GetYaxis()->SetBinLabel(TrgComparisonResult::kTCMonly + 1, "TCM only");
  mHistTriggersSoftwareVsTCM->GetYaxis()->SetBinLabel(TrgComparisonResult::kNone + 1, "neither TCM nor Sw");
  mHistTriggersSoftwareVsTCM->GetYaxis()->SetBinLabel(TrgComparisonResult::kBoth + 1, "both TCM and Sw");

  mListHistGarbage = new TList();
  mListHistGarbage->SetOwner(kTRUE);

  /// ak1
  mHist2CorrTCMchAndPMch = std::make_unique<TH2F>("CorrTCMchAndPMch", "TCM charge  - (PM totalCh/8);TCM charge;TCM - PM/8 totalCh;", 1100, 0, 6600, 301, -150.5, 150.5);
  mHist2CorrTCMchAndPMch->GetYaxis()->SetRangeUser(-8, 8);

  std::map<std::string, uint8_t> mapFEE2hash;
  const auto& lut = o2::fdd::SingleLUT::Instance().getVecMetadataFEE();
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
    if (std::regex_match(strChID, std::regex("[[\\d]{1,3}"))) {
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

  mHistBCvsFEEmodules = std::make_unique<TH2F>("BCvsFEEmodules", "BC vs FEE module;BC;FEE", sBCperOrbit, 0, sBCperOrbit, mapFEE2hash.size(), 0, mapFEE2hash.size());
  mHistOrbitVsFEEmodules = std::make_unique<TH2F>("OrbitVsFEEmodules", "Orbit vs FEE module;Orbit;FEE", sOrbitsPerTF, 0, sOrbitsPerTF, mapFEE2hash.size(), 0, mapFEE2hash.size());
  for (const auto& entry : mapFEE2hash) {
    mHistBCvsFEEmodules->GetYaxis()->SetBinLabel(entry.second + 1, entry.first.c_str());
    mHistOrbitVsFEEmodules->GetYaxis()->SetBinLabel(entry.second + 1, entry.first.c_str());
  }
  /// ak1
  mHistTimeSum2Diff = std::make_unique<TH2F>("timeSumVsDiff", "time A/C side: sum VS diff;(TOC-TOA)/2 [ns];(TOA+TOC)/2 [ns]", 2000, -52.08, 52.08, 2000, -52.08, 52.08); // range of 52.08 ns = 4000*13.02ps = 4000 channels
  mHistTimeSum2Diff->GetXaxis()->SetRangeUser(-5, 5);
  mHistTimeSum2Diff->GetYaxis()->SetRangeUser(-5, 5);
  mHistNumADC = std::make_unique<TH1F>("HistNumADC", "HistNumADC", sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mHistNumCFD = std::make_unique<TH1F>("HistNumCFD", "HistNumCFD", sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mHistCFDEff = std::make_unique<TH1F>("CFD_efficiency", "Fraction of events with CFD in ADC gate vs ChannelID;ChannelID;Event fraction with CFD in ADC gate", sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mHistSaturationFraction = std::make_unique<TH1F>("ADCChargeFractionInRange", Form("Fraction of charge in [%d, %d] ADC;Channel ID;Event fraction in [%d, %d] ADC", mBinMinADCSaturationCheck, mBinMaxADCSaturationCheck, mBinMinADCSaturationCheck, mBinMaxADCSaturationCheck), sNCHANNELS_PM, 0, sNCHANNELS_PM);
  std::string gateTimeRatioTitle = "Ratio of events between time " + std::to_string(mMinTimeGate) + " and " + std::to_string(mMaxTimeGate);
  mHistGateTimeRatio2Ch = std::make_unique<TH1F>("EventsInGateTime", gateTimeRatioTitle.c_str(), sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mHistNchA = std::make_unique<TH1F>("NumChannelsA", "Number of channels(TCM), side A;Nch", sNCHANNELS_A, 0, sNCHANNELS_A);
  mHistNchC = std::make_unique<TH1F>("NumChannelsC", "Number of channels(TCM), side C;Nch", sNCHANNELS_C, 0, sNCHANNELS_C);
  mHistSumAmpA = std::make_unique<TH1F>("SumAmpA", "Sum of amplitudes(TCM), side A;", 5e3, 0, 5e3);
  mHistSumAmpC = std::make_unique<TH1F>("SumAmpC", "Sum of amplitudes(TCM), side C;", 5e3, 0, 5e3);
  mHistAverageTimeA = std::make_unique<TH1F>("AverageTimeA", "Average time(TCM), side A", 4100, -2050, 2050);
  mHistAverageTimeC = std::make_unique<TH1F>("AverageTimeC", "Average time(TCM), side C", 4100, -2050, 2050);
  mHistChannelID = std::make_unique<TH1F>("StatChannelID", "ChannelID statistics;ChannelID", sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mHistCycleDuration = std::make_unique<TH1D>("CycleDuration", "Cycle Duration;;time [ns]", 1, 0, 2);
  mHistCycleDurationNTF = std::make_unique<TH1D>("CycleDurationNTF", "Cycle Duration;;time [TimeFrames]", 1, 0, 2);
  mHistCycleDurationRange = std::make_unique<TH1D>("CycleDurationRange", "Cycle Duration (total cycle range);;time [ns]", 1, 0, 2);

  std::vector<unsigned int> vecChannelIDs;
  if (auto param = mCustomParameters.find("ChannelIDs"); param != mCustomParameters.end()) {
    const auto chIDs = param->second;
    const std::string del = ",";
    vecChannelIDs = parseParameters<unsigned int>(chIDs, del);
  }
  for (const auto& entry : vecChannelIDs) {
    mSetAllowedChIDs.insert(entry);
  }
  std::vector<unsigned int> vecChannelIDsAmpVsTime;
  if (auto param = mCustomParameters.find("ChannelIDsAmpVsTime"); param != mCustomParameters.end()) {
    const auto chIDs = param->second;
    const std::string del = ",";
    vecChannelIDsAmpVsTime = parseParameters<unsigned int>(chIDs, del);
  }
  for (const auto& entry : vecChannelIDsAmpVsTime) {
    mSetAllowedChIDsAmpVsTime.insert(entry);
  }

  for (const auto& chID : mSetAllowedChIDs) {
    auto pairHistAmp = mMapHistAmp1D.insert({ chID, new TH1F(Form("Amp_channel%i", chID), Form("Amplitude, channel %i", chID), 4200, -100, 4100) });
    auto pairHistAmpCoincidence = mMapHistAmp1DCoincidence.insert({ chID, new TH1F(Form("Amp_channelCoincidence%i", chID), Form("AmplitudeCoincidence, channel %i", chID), 4200, -100, 4100) });
    auto pairHistTime = mMapHistTime1D.insert({ chID, new TH1F(Form("Time_channel%i", chID), Form("Time, channel %i", chID), 4100, -2050, 2050) });
    auto pairHistBits = mMapHistPMbits.insert({ chID, new TH1F(Form("Bits_channel%i", chID), Form("Bits, channel %i", chID), mMapChTrgNames.size(), 0, mMapChTrgNames.size()) });
    for (const auto& entry : mMapChTrgNames) {
      pairHistBits.first->second->GetXaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());
    }
    if (pairHistAmp.second) {
      getObjectsManager()->startPublishing(pairHistAmp.first->second);
      mListHistGarbage->Add(pairHistAmp.first->second);
    }
    if (pairHistAmpCoincidence.second) {
      getObjectsManager()->startPublishing(pairHistAmpCoincidence.first->second);
      mListHistGarbage->Add(pairHistAmpCoincidence.first->second);
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
  // 1-dim hists
  getObjectsManager()->startPublishing(mHistCFDEff.get());
  getObjectsManager()->startPublishing(mHistSaturationFraction.get());
  getObjectsManager()->startPublishing(mHistGateTimeRatio2Ch.get());
  getObjectsManager()->startPublishing(mHistBC.get());
  getObjectsManager()->startPublishing(mHistNchA.get());
  getObjectsManager()->startPublishing(mHistNchC.get());
  getObjectsManager()->startPublishing(mHistSumAmpA.get());
  getObjectsManager()->startPublishing(mHistSumAmpC.get());
  getObjectsManager()->startPublishing(mHistAverageTimeA.get());
  getObjectsManager()->startPublishing(mHistAverageTimeC.get());
  getObjectsManager()->startPublishing(mHistChannelID.get());
  getObjectsManager()->startPublishing(mHistCycleDuration.get());
  getObjectsManager()->startPublishing(mHistCycleDurationNTF.get());
  getObjectsManager()->startPublishing(mHistCycleDurationRange.get());
  getObjectsManager()->startPublishing(mHistTriggersSw.get());
  /// 2d hists
  getObjectsManager()->startPublishing(mHist2CorrTCMchAndPMch.get());
  getObjectsManager()->setDefaultDrawOptions(mHist2CorrTCMchAndPMch.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistTime2Ch.get());
  getObjectsManager()->setDefaultDrawOptions(mHistTime2Ch.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistAmp2Ch.get());
  getObjectsManager()->setDefaultDrawOptions(mHistAmp2Ch.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistBCvsFEEmodules.get());
  getObjectsManager()->setDefaultDrawOptions(mHistBCvsFEEmodules.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistOrbitVsTrg.get());
  getObjectsManager()->setDefaultDrawOptions(mHistOrbitVsTrg.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistOrbitVsFEEmodules.get());
  getObjectsManager()->setDefaultDrawOptions(mHistOrbitVsFEEmodules.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistChDataBits.get());
  getObjectsManager()->setDefaultDrawOptions(mHistChDataBits.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistTimeSum2Diff.get());
  getObjectsManager()->setDefaultDrawOptions(mHistTimeSum2Diff.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistOrbit2BC.get());
  getObjectsManager()->setDefaultDrawOptions(mHistOrbit2BC.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistBCvsTrg.get());
  getObjectsManager()->setDefaultDrawOptions(mHistBCvsTrg.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistEventDensity2Ch.get());
  getObjectsManager()->setDefaultDrawOptions(mHistEventDensity2Ch.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistPmTcmNchA.get());
  getObjectsManager()->setDefaultDrawOptions(mHistPmTcmNchA.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistPmTcmSumAmpA.get());
  getObjectsManager()->setDefaultDrawOptions(mHistPmTcmSumAmpA.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistPmTcmAverageTimeA.get());
  getObjectsManager()->setDefaultDrawOptions(mHistPmTcmAverageTimeA.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistPmTcmNchC.get());
  getObjectsManager()->setDefaultDrawOptions(mHistPmTcmNchC.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistPmTcmSumAmpC.get());
  getObjectsManager()->setDefaultDrawOptions(mHistPmTcmSumAmpC.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistPmTcmAverageTimeC.get());
  getObjectsManager()->setDefaultDrawOptions(mHistPmTcmAverageTimeC.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistTriggersCorrelation.get());
  getObjectsManager()->setDefaultDrawOptions(mHistTriggersCorrelation.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistTriggersSoftwareVsTCM.get());
  getObjectsManager()->setDefaultDrawOptions(mHistTriggersSoftwareVsTCM.get(), "COLZ");

  for (int i = 0; i < getObjectsManager()->getNumberPublishedObjects(); i++) {
    TH1* obj = dynamic_cast<TH1*>(getObjectsManager()->getMonitorObject(i)->getObject());
    obj->SetTitle((string("FDD ") + obj->GetTitle()).c_str());
  }
}

void DigitQcTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
  mHist2CorrTCMchAndPMch->Reset();
  mHistTime2Ch->Reset();
  mHistAmp2Ch->Reset();
  mHistBC->Reset();
  mHistChDataBits->Reset();
  mHistCFDEff->Reset();
  mHistSaturationFraction->Reset();
  mHistGateTimeRatio2Ch->Reset();
  mHistNumADC->Reset();
  mHistNumCFD->Reset();
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
  mHistEventDensity2Ch->Reset();
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
  mHistTriggersSw->Reset();
  mHistTriggersSoftwareVsTCM->Reset();
  for (auto& entry : mMapHistAmp1D) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistAmp1DCoincidence) {
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
  auto channels = ctx.inputs().get<gsl::span<o2::fdd::ChannelData>>("channels");
  auto digits = ctx.inputs().get<gsl::span<o2::fdd::Digit>>("digits");
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
  int mPMChargeTotalAside, mPMChargeTotalCside;
  for (auto& digit : digits) {
    // Exclude all BCs, in which laser signals are expected (and trigger outputs are blocked)
    if (digit.mTriggers.getOutputsAreBlocked()) {
      continue;
    }
    mPMChargeTotalAside = 0;
    mPMChargeTotalCside = 0;
    const auto& vecChData = digit.getBunchChannelData(channels);
    bool isTCM = true;
    if (digit.mTriggers.getTimeA() == o2::fit::Triggers::DEFAULT_TIME && digit.mTriggers.getTimeC() == o2::fit::Triggers::DEFAULT_TIME) {
      isTCM = false;
    }
    mHistOrbit2BC->Fill(digit.getIntRecord().orbit % sOrbitsPerTF, digit.getIntRecord().bc);
    mHistBC->Fill(digit.getBC());

    // Fill the amplitude, if there is a coincidence of the signals in in the front or back layers
    bool hasData[16] = { 0 };
    for (const auto& chData : vecChData) {
      if (mSetAllowedChIDs.find(static_cast<unsigned int>(chData.mPMNumber)) != mSetAllowedChIDs.end()) {
        if (static_cast<int>(chData.mPMNumber) < 16) {
          hasData[static_cast<int>(chData.mPMNumber)] = 1;
        }
      }
    } // ak

    std::set<uint8_t> setFEEmodules{};
    // reset triggers
    for (auto& entry : mMapTrgSoftware) {
      mMapTrgSoftware[entry.first] = false;
    }

    Int_t pmSumAmplA = 0;
    Int_t pmSumAmplC = 0;
    Int_t pmNChanA = 0;
    Int_t pmNChanC = 0;
    Int_t pmSumTimeA = 0;
    Int_t pmSumTimeC = 0;
    Int_t pmAverTimeA = 0;
    Int_t pmAverTimeC = 0;

    std::map<uint8_t, int> mapPMhash2sumAmpl;
    for (const auto& entry : mMapPMhash2isAside) {
      mapPMhash2sumAmpl.insert({ entry.first, 0 });
    }
    for (const auto& chData : vecChData) {
      if (static_cast<int>(chData.mPMNumber) < sNCHANNELS_C)
        mPMChargeTotalCside += chData.mChargeADC;
      else
        mPMChargeTotalAside += chData.mChargeADC;

      mHistTime2Ch->Fill(static_cast<Double_t>(chData.mPMNumber), static_cast<Double_t>(chData.mTime));
      mHistAmp2Ch->Fill(static_cast<Double_t>(chData.mPMNumber), static_cast<Double_t>(chData.mChargeADC));
      mHistEventDensity2Ch->Fill(static_cast<Double_t>(chData.mPMNumber), static_cast<Double_t>(digit.mIntRecord.differenceInBC(mStateLastIR2Ch[chData.mPMNumber])));
      mStateLastIR2Ch[chData.mPMNumber] = digit.mIntRecord;
      mHistChannelID->Fill(chData.mPMNumber);
      if (chData.mChargeADC > 0) {
        mHistNumADC->Fill(chData.mPMNumber);
      }
      mHistNumCFD->Fill(chData.mPMNumber);
      if (mSetAllowedChIDs.size() != 0 && mSetAllowedChIDs.find(static_cast<unsigned int>(chData.mPMNumber)) != mSetAllowedChIDs.end()) {
        mMapHistAmp1D[chData.mPMNumber]->Fill(chData.mChargeADC);
        mMapHistTime1D[chData.mPMNumber]->Fill(chData.mTime);
        for (const auto& entry : mMapChTrgNames) {
          if ((chData.mFEEBits & (1 << entry.first))) {
            mMapHistPMbits[chData.mPMNumber]->Fill(entry.first);
          }
        }

        if (static_cast<int>(chData.mPMNumber) == 0 && hasData[4]) {
          mMapHistAmp1DCoincidence[0]->Fill(chData.mChargeADC);
        }
        if (static_cast<int>(chData.mPMNumber) == 1 && hasData[5]) {
          mMapHistAmp1DCoincidence[1]->Fill(chData.mChargeADC);
        }
        if (static_cast<int>(chData.mPMNumber) == 2 && hasData[6]) {
          mMapHistAmp1DCoincidence[2]->Fill(chData.mChargeADC);
        }
        if (static_cast<int>(chData.mPMNumber) == 3 && hasData[7]) {
          mMapHistAmp1DCoincidence[3]->Fill(chData.mChargeADC);
        }
        if (static_cast<int>(chData.mPMNumber) == 4 && hasData[0]) {
          mMapHistAmp1DCoincidence[4]->Fill(chData.mChargeADC);
        }
        if (static_cast<int>(chData.mPMNumber) == 5 && hasData[1]) {
          mMapHistAmp1DCoincidence[5]->Fill(chData.mChargeADC);
        }
        if (static_cast<int>(chData.mPMNumber) == 6 && hasData[2]) {
          mMapHistAmp1DCoincidence[6]->Fill(chData.mChargeADC);
        }
        if (static_cast<int>(chData.mPMNumber) == 7 && hasData[3]) {
          mMapHistAmp1DCoincidence[7]->Fill(chData.mChargeADC);
        }
        if (static_cast<int>(chData.mPMNumber) == 8 && hasData[12]) {
          mMapHistAmp1DCoincidence[8]->Fill(chData.mChargeADC);
        }
        if (static_cast<int>(chData.mPMNumber) == 9 && hasData[13]) {
          mMapHistAmp1DCoincidence[9]->Fill(chData.mChargeADC);
        }
        if (static_cast<int>(chData.mPMNumber) == 10 && hasData[14]) {
          mMapHistAmp1DCoincidence[10]->Fill(chData.mChargeADC);
        }
        if (static_cast<int>(chData.mPMNumber) == 11 && hasData[15]) {
          mMapHistAmp1DCoincidence[11]->Fill(chData.mChargeADC);
        }
        if (static_cast<int>(chData.mPMNumber) == 12 && hasData[8]) {
          mMapHistAmp1DCoincidence[12]->Fill(chData.mChargeADC);
        }
        if (static_cast<int>(chData.mPMNumber) == 13 && hasData[9]) {
          mMapHistAmp1DCoincidence[13]->Fill(chData.mChargeADC);
        }
        if (static_cast<int>(chData.mPMNumber) == 14 && hasData[10]) {
          mMapHistAmp1DCoincidence[14]->Fill(chData.mChargeADC);
        }
        if (static_cast<int>(chData.mPMNumber) == 15 && hasData[11]) {
          mMapHistAmp1DCoincidence[15]->Fill(chData.mChargeADC);
        }
      }
      if (mSetAllowedChIDsAmpVsTime.size() != 0 && mSetAllowedChIDsAmpVsTime.find(static_cast<unsigned int>(chData.mPMNumber)) != mSetAllowedChIDsAmpVsTime.end()) {
        mMapHistAmpVsTime[chData.mPMNumber]->Fill(chData.mChargeADC, chData.mTime);
      }
      for (const auto& binPos : mHashedBitBinPos[chData.mFEEBits]) {
        mHistChDataBits->Fill(chData.mPMNumber, binPos);
      }

      setFEEmodules.insert(mChID2PMhash[chData.mPMNumber]);

      if (chIsVertexEvent(chData)) {
        if (!mMapPMhash2isAside[mChID2PMhash[static_cast<uint8_t>(chData.mPMNumber)]]) {
          pmSumTimeC += chData.mTime;
          pmNChanC++;
        } else if (mMapPMhash2isAside[mChID2PMhash[static_cast<uint8_t>(chData.mPMNumber)]]) {
          pmSumTimeA += chData.mTime;
          pmNChanA++;
        }
      }
      if (chData.getFlag(o2::fdd::ChannelData::kIsCFDinADCgate)) {
        mapPMhash2sumAmpl[mChID2PMhash[static_cast<uint8_t>(chData.mPMNumber)]] += static_cast<Int_t>(chData.mChargeADC);
      }
    }

    for (const auto& entry : mapPMhash2sumAmpl) {
      if (mMapPMhash2isAside[entry.first])
        pmSumAmplA += std::lround(static_cast<int>(entry.second / 8.));
      else
        pmSumAmplC += std::lround(static_cast<int>(entry.second / 8.));
    }

    auto pmNChan = pmNChanA + pmNChanC;
    auto pmSumAmpl = pmSumAmplA + pmSumAmplC;
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
    } else {
      pmAverTimeA = o2::fit::Triggers::DEFAULT_TIME;
      pmAverTimeC = o2::fit::Triggers::DEFAULT_TIME;
    }
    auto vtxPos = (pmNChanA && pmNChanC) ? (pmAverTimeC - pmAverTimeA) / 2 : 0;

    /// PM charge is scaled by 8 to compare with TCM charge
    mPMChargeTotalAside = std::lround(static_cast<int>(mPMChargeTotalAside / 8));
    mPMChargeTotalCside = std::lround(static_cast<int>(mPMChargeTotalCside / 8));

    if (isTCM) {
      setFEEmodules.insert(mTCMhash);
      mHist2CorrTCMchAndPMch->Fill(digit.mTriggers.getAmplA() + digit.mTriggers.getAmplC(), (digit.mTriggers.getAmplA() + digit.mTriggers.getAmplC()) - (mPMChargeTotalAside + mPMChargeTotalCside));
    }
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
      for (const auto& binPos : mHashedPairBitBinPos[digit.mTriggers.getTriggersignals()]) {
        mHistTriggersCorrelation->Fill(binPos.first, binPos.second);
      }
      for (const auto& binPos : mHashedBitBinPos[digit.mTriggers.getTriggersignals()]) {
        mHistBCvsTrg->Fill(digit.getIntRecord().bc, binPos);
        mHistOrbitVsTrg->Fill(digit.getIntRecord().orbit % sOrbitsPerTF, binPos);
      }
    }

    // triggers re-computation
    mMapTrgSoftware[o2::fdd::Triggers::bitA] = pmNChanA > 0;
    mMapTrgSoftware[o2::fdd::Triggers::bitC] = pmNChanC > 0;

    if (mTrgThresholdTimeLow < vtxPos && vtxPos < mTrgThresholdTimeHigh && pmNChanA > 0 && pmNChanC > 0)
      mMapTrgSoftware[o2::fdd::Triggers::bitVertex] = true;

    // Central/SemiCentral logic
    switch (mTrgModeSide) {
      case TrgModeSide::kAplusC:
        if (mTrgModeThresholdVar == TrgModeThresholdVar::kAmpl) {
          if (pmSumAmplA + pmSumAmplC >= 2 * mTrgThresholdCenA)
            mMapTrgSoftware[o2::fdd::Triggers::bitCen] = true;
          else if (pmSumAmplA + pmSumAmplC >= 2 * mTrgThresholdSCenA)
            mMapTrgSoftware[o2::fdd::Triggers::bitSCen] = true;
        } else if (mTrgModeThresholdVar == TrgModeThresholdVar::kNchannels) {
          if (pmNChanA + pmNChanC >= mTrgThresholdCenA)
            mMapTrgSoftware[o2::fdd::Triggers::bitCen] = true;
          else if (pmNChanA + pmNChanC >= mTrgThresholdSCenA)
            mMapTrgSoftware[o2::fdd::Triggers::bitSCen] = true;
        }
        break;

      case TrgModeSide::kAandC:
        if (mTrgModeThresholdVar == TrgModeThresholdVar::kAmpl) {
          if (pmSumAmplA >= 2 * mTrgThresholdCenA && pmSumAmplC >= 2 * mTrgThresholdCenC)
            mMapTrgSoftware[o2::fdd::Triggers::bitCen] = true;
          else if (pmSumAmplA >= 2 * mTrgThresholdSCenA && pmSumAmplC >= 2 * mTrgThresholdSCenC)
            mMapTrgSoftware[o2::fdd::Triggers::bitSCen] = true;
        } else if (mTrgModeThresholdVar == TrgModeThresholdVar::kNchannels) {
          if (pmNChanA >= mTrgThresholdCenA && pmNChanC >= mTrgThresholdCenC)
            mMapTrgSoftware[o2::fdd::Triggers::bitCen] = true;
          else if (pmNChanA >= mTrgThresholdSCenA && pmNChanC >= mTrgThresholdSCenC)
            mMapTrgSoftware[o2::fdd::Triggers::bitSCen] = true;
        }
        break;

      case TrgModeSide::kA:
        if (mTrgModeThresholdVar == TrgModeThresholdVar::kAmpl) {
          if (pmSumAmplA >= 2 * mTrgThresholdCenA)
            mMapTrgSoftware[o2::fdd::Triggers::bitCen] = true;
          else if (pmSumAmplA >= 2 * mTrgThresholdSCenA)
            mMapTrgSoftware[o2::fdd::Triggers::bitSCen] = true;
        } else if (mTrgModeThresholdVar == TrgModeThresholdVar::kNchannels) {
          if (pmNChanA >= 2 * mTrgThresholdCenA)
            mMapTrgSoftware[o2::fdd::Triggers::bitCen] = true;
          else if (pmNChanA >= 2 * mTrgThresholdSCenA)
            mMapTrgSoftware[o2::fdd::Triggers::bitSCen] = true;
        }
        break;

      case TrgModeSide::kC:
        if (mTrgModeThresholdVar == TrgModeThresholdVar::kAmpl) {
          if (pmSumAmplC >= mTrgThresholdCenC)
            mMapTrgSoftware[o2::fdd::Triggers::bitCen] = true;
          else if (pmSumAmplC >= mTrgThresholdSCenC)
            mMapTrgSoftware[o2::fdd::Triggers::bitSCen] = true;
        } else if (mTrgModeThresholdVar == TrgModeThresholdVar::kNchannels) {
          if (pmNChanC >= mTrgThresholdCenC)
            mMapTrgSoftware[o2::fdd::Triggers::bitCen] = true;
          else if (pmNChanC >= mTrgThresholdSCenC)
            mMapTrgSoftware[o2::fdd::Triggers::bitSCen] = true;
        }
        break;
    }

    for (const auto& entry : mMapTrgSoftware) {
      if (entry.second)
        mHistTriggersSw->Fill(entry.first);
      bool isTCMFired = digit.mTriggers.getTriggersignals() & (1 << entry.first);
      bool isSwFired = entry.second;
      if (!isTCMFired && isSwFired)
        mHistTriggersSoftwareVsTCM->Fill(entry.first, TrgComparisonResult::kSWonly);
      else if (isTCMFired && !isSwFired)
        mHistTriggersSoftwareVsTCM->Fill(entry.first, TrgComparisonResult::kTCMonly);
      else if (!isTCMFired && !isSwFired)
        mHistTriggersSoftwareVsTCM->Fill(entry.first, TrgComparisonResult::kNone);
      else if (isTCMFired && isSwFired)
        mHistTriggersSoftwareVsTCM->Fill(entry.first, TrgComparisonResult::kBoth);

      if (isTCMFired != isSwFired) {
        auto msg = Form(
          "Software does not reproduce TCM decision! \n \
                         trigger name: %s\n \
                         TCM / SW: \n \
                         hasFired   = %d / %d \n \
                         nChannelsA = %d / %d \n \
                         nChannelsC = %d / %d \n \
                         sumAmplA   = %d / %d \n \
                         sumAmplC   = %d / %d \n \
                         timeA      = %d / %d \n \
                         timeC      = %d / %d \n \
                         vertexPos  = -- / %d \n \
                         TCM bits   = %d / --",
          mMapDigitTrgNames[entry.first].c_str(),
          isTCMFired, isSwFired,
          digit.mTriggers.getNChanA(), pmNChanA,
          digit.mTriggers.getNChanC(), pmNChanC,
          digit.mTriggers.getAmplA(), pmSumAmplA,
          digit.mTriggers.getAmplC(), pmSumAmplC,
          digit.mTriggers.getTimeA(), pmAverTimeA,
          digit.mTriggers.getTimeC(), pmAverTimeC, vtxPos,
          digit.mTriggers.getTriggersignals());
        ILOG(Debug, Support) << msg << ENDM;
      }
    }
    // end of triggers re-computation
  }
}

void DigitQcTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
  // add TF creation time for further match with filling scheme in PP in case of offline running
  ILOG(Debug, Support) << "adding last TF creation time: " << mTFcreationTime << ENDM;
  getObjectsManager()->getMonitorObject(mHistBCvsTrg->GetName())->addOrUpdateMetadata("TFcreationTime", std::to_string(mTFcreationTime));

  // one has to set num. of entries manually because
  // default TH1Reductor gets only mean,stddev and entries (no integral)
  mHistCFDEff->Divide(mHistNumADC.get(), mHistNumCFD.get());
  for (int iPM = 0; iPM < sNCHANNELS_PM; iPM++) {
    double intNumerator = mHistAmp2Ch->ProjectionY("yNum", iPM + 1, iPM + 1)->Integral(mBinMinADCSaturationCheck, mBinMaxADCSaturationCheck);
    double intDenominator = mHistAmp2Ch->ProjectionY("yDen", iPM + 1, iPM + 1)->Integral(mBinMinADCSaturationCheck, mHistAmp2Ch->GetNbinsY());
    if (intDenominator)
      mHistSaturationFraction->SetBinContent(iPM, intNumerator / intDenominator);
  }

  for (int channel = 0; channel <= sNCHANNELS_PM; channel++) {
    float events_in_range = 0;
    float events_per_channel = 0;
    for (int bin_on_y_axis = 1; bin_on_y_axis <= mHistTime2Ch->GetNbinsY(); bin_on_y_axis++) {
      if (mHistTime2Ch->GetYaxis()->GetBinLowEdge(bin_on_y_axis) > mMinTimeGate && mHistTime2Ch->GetYaxis()->GetBinLowEdge(bin_on_y_axis) < mMaxTimeGate) {
        events_in_range += mHistTime2Ch->GetBinContent(channel + 1, bin_on_y_axis);
      }
      events_per_channel += mHistTime2Ch->GetBinContent(channel + 1, bin_on_y_axis);
    }
    if (events_per_channel) {
      mHistGateTimeRatio2Ch->SetBinContent(channel + 1, events_in_range / events_per_channel);
    } else {
      mHistGateTimeRatio2Ch->SetBinContent(channel + 1, 0);
    }
  }
  mHistSaturationFraction->GetYaxis()->SetRangeUser(0, 1.1);
  mHistCycleDurationRange->SetBinContent(1., mTimeMaxNS - mTimeMinNS);
  mHistCycleDurationRange->SetEntries(mTimeMaxNS - mTimeMinNS);
  mHistCycleDurationNTF->SetBinContent(1., mTfCounter);
  mHistCycleDurationNTF->SetEntries(mTfCounter);
  mHistCycleDuration->SetBinContent(1., mTimeSum);
  mHistCycleDuration->SetEntries(mTimeSum);
  ILOG(Debug, Support) << "Cycle duration: NTF=" << mTfCounter << ", range = " << (mTimeMaxNS - mTimeMinNS) / 1e6 / mTfCounter << " ms/TF, sum = " << mTimeSum / 1e6 / mTfCounter << " ms/TF" << ENDM;
}

void DigitQcTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void DigitQcTask::reset()
{
  // clean all the monitor objects here
  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  mHist2CorrTCMchAndPMch->Reset();
  mHistTime2Ch->Reset();
  mHistAmp2Ch->Reset();
  mHistBC->Reset();
  mHistChDataBits->Reset();
  mHistCFDEff->Reset();
  mHistSaturationFraction->Reset();
  mHistGateTimeRatio2Ch->Reset();
  mHistNumADC->Reset();
  mHistNumCFD->Reset();
  mHistTimeSum2Diff->Reset();
  mHistOrbit2BC->Reset();
  mHistEventDensity2Ch->Reset();
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
  mHistTriggersSw->Reset();
  mHistTriggersSoftwareVsTCM->Reset();
  for (auto& entry : mMapHistAmp1D) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistAmp1DCoincidence) {
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

} // namespace o2::quality_control_modules::fdd
