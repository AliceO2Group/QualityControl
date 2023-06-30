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

#include "FV0/DigitQcTask.h"

#include "TCanvas.h"
#include "TROOT.h"

#include "QualityControl/QcInfoLogger.h"
#include "DataFormatsFIT/Triggers.h"
#include "Framework/InputRecord.h"
#include "Framework/TimingInfo.h"
#include "DataFormatsFV0/LookUpTable.h"

namespace o2::quality_control_modules::fv0
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

bool DigitQcTask::chIsVertexEvent(const o2::fv0::ChannelData chd, bool simpleCheck) const
{
  if (simpleCheck) {
    return chd.getFlag(o2::fv0::ChannelData::kIsEventInTVDC);
  } else {
    return (chd.getFlag(o2::fv0::ChannelData::kIsCFDinADCgate) &&
            !(chd.getFlag(o2::fv0::ChannelData::kIsTimeInfoNOTvalid) || chd.getFlag(o2::fv0::ChannelData::kIsTimeInfoLate) || chd.getFlag(o2::fv0::ChannelData::kIsTimeInfoLost)) &&
            std::abs(static_cast<Int_t>(chd.CFDTime)) < mTrgOrGate &&
            static_cast<Int_t>(chd.QTCAmpl) > mTrgChargeLevelLow &&
            static_cast<Int_t>(chd.QTCAmpl) < mTrgChargeLevelHigh);
  }
}

int DigitQcTask::fpgaDivision(int numerator, int denominator)
{
  // content of the ROM
  int rom7x17[] = { 16383, 8192, 5461, 4096, 3277, 2731, 2341, 2048, 1820, 1638, 1489, 1365, 1260, 1170, 1092, 1024, 964, 910, 862, 819, 780, 745, 712, 683, 655, 630, 607, 585, 565, 546, 529, 512, 496, 482, 468, 455, 443, 431, 420, 410, 400, 390, 381, 372, 364, 356, 349, 341, 334, 328, 321, 315, 309, 303, 298, 293, 287, 282, 278, 273, 269, 264, 260, 256, 252, 248, 245, 241, 237, 234, 231, 228, 224, 221, 218, 216, 213, 210, 207, 205, 202, 200, 197, 195, 193, 191, 188, 186, 184, 182, 180, 178, 176, 174, 172, 171, 169, 167, 165, 164, 162, 161, 159, 158, 156, 155, 153, 152, 150, 149, 148, 146, 145, 144, 142, 141, 140, 139, 138, 137, 135, 134, 133, 132, 131, 130, 129 };

  // return result of the operation (numerator / denominator)
  return denominator ? (numerator * rom7x17[denominator - 1]) >> 14 : 0;
}

void DigitQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize DigitQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  mStateLastIR2Ch = {};
  mMapChTrgNames.insert({ o2::fv0::ChannelData::kNumberADC, "NumberADC" });
  mMapChTrgNames.insert({ o2::fv0::ChannelData::kIsDoubleEvent, "IsDoubleEvent" });
  mMapChTrgNames.insert({ o2::fv0::ChannelData::kIsTimeInfoNOTvalid, "IsTimeInfoNOTvalid" });
  mMapChTrgNames.insert({ o2::fv0::ChannelData::kIsCFDinADCgate, "IsCFDinADCgate" });
  mMapChTrgNames.insert({ o2::fv0::ChannelData::kIsTimeInfoLate, "IsTimeInfoLate" });
  mMapChTrgNames.insert({ o2::fv0::ChannelData::kIsAmpHigh, "IsAmpHigh" });
  mMapChTrgNames.insert({ o2::fv0::ChannelData::kIsEventInTVDC, "IsEventInTVDC" });
  mMapChTrgNames.insert({ o2::fv0::ChannelData::kIsTimeInfoLost, "IsTimeInfoLost" });

  mMapDigitTrgNames.insert({ o2::fit::Triggers::bitA, "OrA" });
  mMapDigitTrgNames.insert({ o2::fit::Triggers::bitAOut, "OrAOut" });
  mMapDigitTrgNames.insert({ o2::fit::Triggers::bitTrgNchan, "TrgNChan" });
  mMapDigitTrgNames.insert({ o2::fit::Triggers::bitTrgCharge, "TrgCharge" });
  mMapDigitTrgNames.insert({ o2::fit::Triggers::bitAIn, "OrAIn" });
  mMapDigitTrgNames.insert({ o2::fit::Triggers::bitLaser, "Laser" });
  mMapDigitTrgNames.insert({ o2::fit::Triggers::bitOutputsAreBlocked, "OutputsAreBlocked" });
  mMapDigitTrgNames.insert({ o2::fit::Triggers::bitDataIsValid, "DataIsValid" });

  mMapTrgSoftware.insert({ o2::fit::Triggers::bitA, false });
  mMapTrgSoftware.insert({ o2::fit::Triggers::bitAOut, false });
  mMapTrgSoftware.insert({ o2::fit::Triggers::bitTrgNchan, false });
  mMapTrgSoftware.insert({ o2::fit::Triggers::bitTrgCharge, false });
  mMapTrgSoftware.insert({ o2::fit::Triggers::bitAIn, false });

  mTrgModeInnerOuterThresholdVar = getModeParameter("trgModeInnerOuterThresholdVar",
                                                    TrgModeThresholdVar::kNchannels,
                                                    { { TrgModeThresholdVar::kAmpl, "Ampl" },
                                                      { TrgModeThresholdVar::kNchannels, "Nchannels" } });

  mTrgOrGate = getNumericalParameter("trgOrGate", 153);
  mTrgChargeLevelLow = getNumericalParameter("trgChargeLevelLow", 0);
  mTrgChargeLevelHigh = getNumericalParameter("trgChargeLevelHigh", 4095);
  mTrgThresholdCharge = getNumericalParameter("trgThresholdCharge", 1);
  mTrgThresholdNChannels = getNumericalParameter("trgThresholdNChannels", 1);
  mMinTimeGate = getNumericalParameter("minGateTimeForRatioHistogram", -192);
  mMaxTimeGate = getNumericalParameter("maxGateTimeForRatioHistogram", 192);

  if (mTrgModeInnerOuterThresholdVar == TrgModeThresholdVar::kAmpl) {
    mTrgThresholdChargeInner = getNumericalParameter("trgThresholdChargeInner", 1);
    mTrgThresholdChargeOuter = getNumericalParameter("trgThresholdChargeOuter", 1);
  } else if (mTrgModeInnerOuterThresholdVar == TrgModeThresholdVar::kNchannels) {
    mTrgThresholdNChannelsInner = getNumericalParameter("trgThresholdNChannelsInner", 1);
    mTrgThresholdNChannelsOuter = getNumericalParameter("trgThresholdNChannelsOuter", 1);
  }

  mHistTime2Ch = std::make_unique<TH2F>("TimePerChannel", "Time vs Channel;Channel;Time", sNCHANNELS_FV0_PLUSREF, 0, sNCHANNELS_FV0_PLUSREF, 4100, -2050, 2050);
  mHistTime2Ch->SetOption("colz");
  mHistAmp2Ch = std::make_unique<TH2F>("AmpPerChannel", "Amplitude vs Channel;Channel;Amp", sNCHANNELS_FV0_PLUSREF, 0, sNCHANNELS_FV0_PLUSREF, 4200, -100, 4100);
  mHistAmp2Ch->SetOption("colz");
  mHistBC = std::make_unique<TH1F>("BC", "BC;BC;counts;", sBCperOrbit, 0, sBCperOrbit);
  mHistChDataBits = std::make_unique<TH2F>("ChannelDataBits", "ChannelData bits per ChannelID;Channel;Bit", sNCHANNELS_FV0_PLUSREF, 0, sNCHANNELS_FV0_PLUSREF, mMapChTrgNames.size(), 0, mMapChTrgNames.size());
  mHistChDataBits->SetOption("colz");
  for (const auto& entry : mMapChTrgNames) {
    mHistChDataBits->GetYaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());
  }
  mHistOrbitVsTrg = std::make_unique<TH2F>("OrbitVsTriggers", "Orbit vs Triggers;Orbit;Trg", sOrbitsPerTF, 0, sOrbitsPerTF, mMapDigitTrgNames.size(), 0, mMapDigitTrgNames.size());
  mHistOrbitVsTrg->SetOption("colz");
  mHistOrbit2BC = std::make_unique<TH2F>("OrbitPerBC", "BC-Orbit map;Orbit;BC;", sOrbitsPerTF, 0, sOrbitsPerTF, sBCperOrbit, 0, sBCperOrbit);
  mHistOrbit2BC->SetOption("colz");
  mHistEventDensity2Ch = std::make_unique<TH2F>("EventDensityPerChannel", "Event density(in BC) per Channel;Channel;BC;", sNCHANNELS_FV0_PLUSREF, 0, sNCHANNELS_FV0_PLUSREF, 10000, 0, 1e5);
  mHistEventDensity2Ch->SetOption("colz");
  mHistTriggersCorrelation = std::make_unique<TH2F>("TriggersCorrelation", "Correlation of triggers from TCM", mMapDigitTrgNames.size(), 0, mMapDigitTrgNames.size(), mMapDigitTrgNames.size(), 0, mMapDigitTrgNames.size());
  mHistTriggersCorrelation->SetOption("colz");
  mHistBCvsTrg = std::make_unique<TH2F>("BCvsTriggers", "BC vs Triggers;BC;Trg", sBCperOrbit, 0, sBCperOrbit, mMapDigitTrgNames.size(), 0, mMapDigitTrgNames.size());
  mHistBCvsTrg->SetOption("colz");
  mHistPmTcmNchA = std::make_unique<TH2F>("PmTcmNumChannelsA", "Comparison of num. channels A from PM and TCM;Number of channels(TCM), side A;PM - TCM", sNCHANNELS_FV0_PLUSREF, 0, sNCHANNELS_FV0_PLUSREF, 2 * sNCHANNELS_FV0_PLUSREF + 1, -int(sNCHANNELS_FV0_PLUSREF) - 0.5, int(sNCHANNELS_FV0_PLUSREF) + 0.5);
  mHistPmTcmSumAmpA = std::make_unique<TH2F>("PmTcmSumAmpA", "Comparison of sum of amplitudes A from PM and TCM;Sum of amplitudes(TCM), side A;PM - TCM", 2e2, 0, 1e4, 2e3, -1e3 - 0.5, 1e3 - 0.5);
  mHistPmTcmAverageTimeA = std::make_unique<TH2F>("PmTcmAverageTimeA", "Comparison of average time A from PM and TCM;Average time(TCM), side A;PM - TCM", 410, -2050, 2050, 820, -410 - 0.5, 410 - 0.5);
  mHistTriggersSw = std::make_unique<TH1F>("TriggersSoftware", "Triggers from software", mMapDigitTrgNames.size(), 0, mMapDigitTrgNames.size());
  mHistTriggersSoftwareVsTCM = std::make_unique<TH2F>("TriggersSoftwareVsTCM", "Comparison of triggers from software and TCM;;Trigger name", mMapDigitTrgNames.size(), 0, mMapDigitTrgNames.size(), 4, 0, 4);
  mHistTriggersSoftwareVsTCM->SetOption("colz");
  mHistTriggersSoftwareVsTCM->SetStats(1);
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

  std::map<std::string, uint8_t> mapFEE2hash;
  const auto& lut = o2::fv0::SingleLUT::Instance().getVecMetadataFEE();
  auto lutSorted = lut;
  std::sort(lutSorted.begin(), lutSorted.end(), [](const auto& first, const auto& second) { return first.mModuleName < second.mModuleName; });
  uint8_t binPos{ 0 };
  for (const auto& lutEntry : lutSorted) {
    const auto& moduleName = lutEntry.mModuleName;
    const auto& moduleType = lutEntry.mModuleType;
    const auto& strChID = lutEntry.mChannelID;
    const auto& pairIt = mapFEE2hash.insert({ moduleName, binPos });
    if (pairIt.second) {
      binPos++;
    }
    if (std::regex_match(strChID, std::regex("[[\\d]{1,3}"))) {
      int chID = std::stoi(strChID);
      if (chID < sNCHANNELS_FV0_PLUSREF) {
        mChID2PMhash[chID] = mapFEE2hash[moduleName];
        mMapPMhash2isInner.insert({ mapFEE2hash[moduleName], chID < sNCHANNELS_FV0_INNER });
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
  mHistBcVsFeeForOrATrg = std::make_unique<TH2F>("BCvsFEEmodulesForOrATrg", "BC vs FEE module for OrA trigger;BC;FEE", sBCperOrbit, 0, sBCperOrbit, mapFEE2hash.size(), 0, mapFEE2hash.size());
  mHistBcVsFeeForOrAOutTrg = std::make_unique<TH2F>("BCvsFEEmodulesForOrAOutTrg", "BC vs FEE module for OrAOut trigger;BC;FEE", sBCperOrbit, 0, sBCperOrbit, mapFEE2hash.size(), 0, mapFEE2hash.size());
  mHistBcVsFeeForNChanTrg = std::make_unique<TH2F>("BCvsFEEmodulesForNChanTrg", "BC vs FEE module for NChan trigger;BC;FEE", sBCperOrbit, 0, sBCperOrbit, mapFEE2hash.size(), 0, mapFEE2hash.size());
  mHistBcVsFeeForChargeTrg = std::make_unique<TH2F>("BCvsFEEmodulesForChargeTrg", "BC vs FEE module for Charge trigger;BC;FEE", sBCperOrbit, 0, sBCperOrbit, mapFEE2hash.size(), 0, mapFEE2hash.size());
  mHistBcVsFeeForOrAInTrg = std::make_unique<TH2F>("BCvsFEEmodulesForOrAInTrg", "BC vs FEE module for OrAIn trigger;BC;FEE", sBCperOrbit, 0, sBCperOrbit, mapFEE2hash.size(), 0, mapFEE2hash.size());
  mHistOrbitVsFEEmodules = std::make_unique<TH2F>("OrbitVsFEEmodules", "Orbit vs FEE module;Orbit;FEE", sOrbitsPerTF, 0, sOrbitsPerTF, mapFEE2hash.size(), 0, mapFEE2hash.size());
  for (const auto& entry : mapFEE2hash) {
    mHistBCvsFEEmodules->GetYaxis()->SetBinLabel(entry.second + 1, entry.first.c_str());
    mHistBcVsFeeForOrATrg->GetYaxis()->SetBinLabel(entry.second + 1, entry.first.c_str());
    mHistBcVsFeeForOrAOutTrg->GetYaxis()->SetBinLabel(entry.second + 1, entry.first.c_str());
    mHistBcVsFeeForNChanTrg->GetYaxis()->SetBinLabel(entry.second + 1, entry.first.c_str());
    mHistBcVsFeeForChargeTrg->GetYaxis()->SetBinLabel(entry.second + 1, entry.first.c_str());
    mHistBcVsFeeForOrAInTrg->GetYaxis()->SetBinLabel(entry.second + 1, entry.first.c_str());
    mHistOrbitVsFEEmodules->GetYaxis()->SetBinLabel(entry.second + 1, entry.first.c_str());
  }
  // mHistTimeSum2Diff = std::make_unique<TH2F>("timeSumVsDiff", "time A/C side: sum VS diff;(TOC-TOA)/2 [ns];(TOA+TOC)/2 [ns]", 400, -52.08, 52.08, 400, -52.08, 52.08); // range of 52.08 ns = 4000*13.02ps = 4000 channels
  mHistNumADC = std::make_unique<TH1F>("HistNumADC", "HistNumADC", sNCHANNELS_FV0_PLUSREF, 0, sNCHANNELS_FV0_PLUSREF);
  mHistNumCFD = std::make_unique<TH1F>("HistNumCFD", "HistNumCFD", sNCHANNELS_FV0_PLUSREF, 0, sNCHANNELS_FV0_PLUSREF);
  mHistCFDEff = std::make_unique<TH1F>("CFD_efficiency", "Fraction of events with CFD in ADC gate vs ChannelID;ChannelID;Event fraction with CFD in ADC gate", sNCHANNELS_FV0_PLUSREF, 0, sNCHANNELS_FV0_PLUSREF);
  mHistNchA = std::make_unique<TH1F>("NumChannelsA", "Number of channels(TCM), side A;Nch", sNCHANNELS_FV0_PLUSREF, 0, sNCHANNELS_FV0_PLUSREF);
  // mHistNchC = std::make_unique<TH1F>("NumChannelsC", "Number of channels(TCM), side C;Nch", sNCHANNELS_FV0_PLUSREF, 0, sNCHANNELS_FV0_PLUSREF);
  mHistSumAmpA = std::make_unique<TH1F>("SumAmpA", "Sum of amplitudes(TCM), side A;", 1e4, 0, 1e4);
  // mHistSumAmpC = std::make_unique<TH1F>("SumAmpC", "Sum of amplitudes(TCM), side C;", 1e4, 0, 1e4);
  mHistAverageTimeA = std::make_unique<TH1F>("AverageTimeA", "Average time(TCM), side A", 4100, -2050, 2050);
  // mHistAverageTimeC = std::make_unique<TH1F>("AverageTimeC", "Average time(TCM), side C", 4100, -2050, 2050);
  mHistChannelID = std::make_unique<TH1F>("StatChannelID", "ChannelID statistics;ChannelID", sNCHANNELS_FV0_PLUSREF, 0, sNCHANNELS_FV0_PLUSREF);
  mHistCycleDuration = std::make_unique<TH1D>("CycleDuration", "Cycle Duration;;time [ns]", 1, 0, 2);
  mHistCycleDurationNTF = std::make_unique<TH1D>("CycleDurationNTF", "Cycle Duration;;time [TimeFrames]", 1, 0, 2);
  mHistCycleDurationRange = std::make_unique<TH1D>("CycleDurationRange", "Cycle Duration (total cycle range);;time [ns]", 1, 0, 2);
  mHistGateTimeRatio2Ch = std::make_unique<TH1F>("EventsInGateTime", "Fraction of events with CFD in time gate vs ChannelID;ChannelID;Event fraction with CFD in time gate", sNCHANNELS_FV0_PLUSREF, 0, sNCHANNELS_FV0_PLUSREF);

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
    auto pairHistTime = mMapHistTime1D.insert({ chID, new TH1F(Form("Time_channel%i", chID), Form("Time, channel %i", chID), 4100, -2050, 2050) });
    auto pairHistBits = mMapHistPMbits.insert({ chID, new TH1F(Form("Bits_channel%i", chID), Form("Bits, channel %i", chID), mMapChTrgNames.size(), 0, mMapChTrgNames.size()) });
    for (const auto& entry : mMapChTrgNames) {
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
  // 1-dim hists
  getObjectsManager()->startPublishing(mHistGateTimeRatio2Ch.get());
  getObjectsManager()->startPublishing(mHistCFDEff.get());
  getObjectsManager()->startPublishing(mHistBC.get());
  getObjectsManager()->startPublishing(mHistNchA.get());
  // getObjectsManager()->startPublishing(mHistNchC.get());
  getObjectsManager()->startPublishing(mHistSumAmpA.get());
  // getObjectsManager()->startPublishing(mHistSumAmpC.get());
  getObjectsManager()->startPublishing(mHistAverageTimeA.get());
  // getObjectsManager()->startPublishing(mHistAverageTimeC.get());
  getObjectsManager()->startPublishing(mHistChannelID.get());
  getObjectsManager()->startPublishing(mHistCycleDuration.get());
  getObjectsManager()->startPublishing(mHistCycleDurationNTF.get());
  getObjectsManager()->startPublishing(mHistCycleDurationRange.get());
  getObjectsManager()->startPublishing(mHistTriggersSw.get());
  // 2-dim hists
  getObjectsManager()->startPublishing(mHistTime2Ch.get());
  getObjectsManager()->setDefaultDrawOptions(mHistTime2Ch.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistAmp2Ch.get());
  getObjectsManager()->setDefaultDrawOptions(mHistAmp2Ch.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistBCvsFEEmodules.get());
  getObjectsManager()->setDefaultDrawOptions(mHistBCvsFEEmodules.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistBcVsFeeForOrATrg.get());
  getObjectsManager()->setDefaultDrawOptions(mHistBcVsFeeForOrATrg.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistBcVsFeeForOrAOutTrg.get());
  getObjectsManager()->setDefaultDrawOptions(mHistBcVsFeeForOrAOutTrg.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistBcVsFeeForNChanTrg.get());
  getObjectsManager()->setDefaultDrawOptions(mHistBcVsFeeForNChanTrg.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistBcVsFeeForChargeTrg.get());
  getObjectsManager()->setDefaultDrawOptions(mHistBcVsFeeForChargeTrg.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistBcVsFeeForOrAInTrg.get());
  getObjectsManager()->setDefaultDrawOptions(mHistBcVsFeeForOrAInTrg.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistOrbitVsTrg.get());
  getObjectsManager()->setDefaultDrawOptions(mHistOrbitVsTrg.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistOrbitVsFEEmodules.get());
  getObjectsManager()->setDefaultDrawOptions(mHistOrbitVsFEEmodules.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistChDataBits.get());
  getObjectsManager()->setDefaultDrawOptions(mHistChDataBits.get(), "COLZ");
  // getObjectsManager()->startPublishing(mHistTimeSum2Diff.get());
  // getObjectsManager()->setDefaultDrawOptions(mHistTimeSum2Diff.get(), "COLZ");
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
  getObjectsManager()->startPublishing(mHistTriggersCorrelation.get());
  getObjectsManager()->setDefaultDrawOptions(mHistTriggersCorrelation.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistTriggersSoftwareVsTCM.get());
  getObjectsManager()->setDefaultDrawOptions(mHistTriggersSoftwareVsTCM.get(), "COLZ");

  for (int i = 0; i < getObjectsManager()->getNumberPublishedObjects(); i++) {
    TH1* obj = dynamic_cast<TH1*>(getObjectsManager()->getMonitorObject(i)->getObject());
    obj->SetTitle((string("FV0 ") + obj->GetTitle()).c_str());
  }
}

void DigitQcTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity" << activity.mId << ENDM;
  mHistTime2Ch->Reset();
  mHistAmp2Ch->Reset();
  mHistBC->Reset();
  mHistChDataBits->Reset();
  mHistGateTimeRatio2Ch->Reset();
  mHistCFDEff->Reset();
  mHistNumADC->Reset();
  mHistNumCFD->Reset();
  // mHistTimeSum2Diff->Reset();
  mHistBCvsFEEmodules->Reset();
  mHistBcVsFeeForOrATrg->Reset();
  mHistBcVsFeeForOrAOutTrg->Reset();
  mHistBcVsFeeForNChanTrg->Reset();
  mHistBcVsFeeForChargeTrg->Reset();
  mHistBcVsFeeForOrAInTrg->Reset();
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
  // mHistNchC->Reset();
  mHistSumAmpA->Reset();
  // mHistSumAmpC->Reset();
  mHistAverageTimeA->Reset();
  // mHistAverageTimeC->Reset();
  mHistChannelID->Reset();
  mHistPmTcmNchA->Reset();
  mHistPmTcmSumAmpA->Reset();
  mHistPmTcmAverageTimeA->Reset();
  mHistTriggersSw->Reset();
  mHistTriggersSoftwareVsTCM->Reset();
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
  auto channels = ctx.inputs().get<gsl::span<o2::fv0::ChannelData>>("channels");
  auto digits = ctx.inputs().get<gsl::span<o2::fv0::Digit>>("digits");
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
    if (digit.mTriggers.getOutputsAreBlocked()) {
      continue;
    }
    const auto& vecChData = digit.getBunchChannelData(channels);
    bool isTCM = true;
    if (digit.mTriggers.getTimeA() == o2::fit::Triggers::DEFAULT_TIME && digit.mTriggers.getTimeC() == o2::fit::Triggers::DEFAULT_TIME) {
      isTCM = false;
    }
    mHistOrbit2BC->Fill(digit.getIntRecord().orbit % sOrbitsPerTF, digit.getIntRecord().bc);
    mHistBC->Fill(digit.getBC());

    std::set<uint8_t> setFEEmodules{};
    // reset triggers
    for (auto& entry : mMapTrgSoftware) {
      mMapTrgSoftware[entry.first] = false;
    }
    Int_t pmSumAmplIn = 0;
    Int_t pmSumAmplOut = 0;
    Int_t pmNChanIn = 0;
    Int_t pmNChanOut = 0;
    Int_t pmSumTime = 0;
    Int_t pmAverTime = 0;

    std::map<uint8_t, int> mapPMhash2sumAmpl;
    for (const auto& entry : mMapPMhash2isInner) {
      mapPMhash2sumAmpl.insert({ entry.first, 0 });
    }

    for (const auto& chData : vecChData) {
      mHistTime2Ch->Fill(static_cast<Double_t>(chData.ChId), static_cast<Double_t>(chData.CFDTime));
      mHistAmp2Ch->Fill(static_cast<Double_t>(chData.ChId), static_cast<Double_t>(chData.QTCAmpl));
      mHistEventDensity2Ch->Fill(static_cast<Double_t>(chData.ChId), static_cast<Double_t>(digit.mIntRecord.differenceInBC(mStateLastIR2Ch[chData.ChId])));
      mStateLastIR2Ch[chData.ChId] = digit.mIntRecord;
      mHistChannelID->Fill(chData.ChId);
      if (chData.QTCAmpl > 0) {
        mHistNumADC->Fill(chData.ChId);
      }
      mHistNumCFD->Fill(chData.ChId);
      if (mSetAllowedChIDs.size() != 0 && mSetAllowedChIDs.find(static_cast<unsigned int>(chData.ChId)) != mSetAllowedChIDs.end()) {
        mMapHistAmp1D[chData.ChId]->Fill(chData.QTCAmpl);
        mMapHistTime1D[chData.ChId]->Fill(chData.CFDTime);
        for (const auto& entry : mMapChTrgNames) {
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

      if (chData.ChId >= sNCHANNELS_FV0) { // skip reference PMT
        continue;
      }

      if (chIsVertexEvent(chData, true)) {
        pmSumTime += chData.CFDTime;
        if (chData.ChId < sNCHANNELS_FV0_INNER) {
          pmNChanIn++;
        } else {
          pmNChanOut++;
        }
      }
      if (chData.getFlag(o2::fv0::ChannelData::kIsCFDinADCgate)) {
        mapPMhash2sumAmpl[mChID2PMhash[static_cast<uint8_t>(chData.ChId)]] += static_cast<Int_t>(chData.QTCAmpl);
      }
    } // channel data loop

    for (const auto& entry : mapPMhash2sumAmpl) {
      if (mMapPMhash2isInner[entry.first])
        pmSumAmplIn += static_cast<int>(entry.second >> 3);
      else
        pmSumAmplOut += static_cast<int>(entry.second >> 3);
    }

    auto pmNChan = pmNChanIn + pmNChanOut;
    auto pmSumAmpl = pmSumAmplIn + pmSumAmplOut;
    if (isTCM) {
      pmAverTime = fpgaDivision(pmSumTime, pmNChan);
    } else {
      pmAverTime = o2::fit::Triggers::DEFAULT_TIME;
    }

    if (isTCM) {
      setFEEmodules.insert(mTCMhash);
    }
    for (const auto& feeHash : setFEEmodules) {
      mHistBCvsFEEmodules->Fill(static_cast<double>(digit.getIntRecord().bc), static_cast<double>(feeHash));
      mHistBcVsFeeForOrATrg->Fill(static_cast<double>(digit.getIntRecord().bc), digit.mTriggers.getOrA());
      mHistBcVsFeeForOrAOutTrg->Fill(static_cast<double>(digit.getIntRecord().bc), digit.mTriggers.getOrAOut());
      mHistBcVsFeeForNChanTrg->Fill(static_cast<double>(digit.getIntRecord().bc), digit.mTriggers.getTrgNChan());
      mHistBcVsFeeForChargeTrg->Fill(static_cast<double>(digit.getIntRecord().bc), digit.mTriggers.getTrgCharge());
      mHistBcVsFeeForOrAInTrg->Fill(static_cast<double>(digit.getIntRecord().bc), digit.mTriggers.getOrAIn());
      mHistOrbitVsFEEmodules->Fill(static_cast<double>(digit.getIntRecord().orbit % sOrbitsPerTF), static_cast<double>(feeHash));
    }

    if (isTCM && digit.mTriggers.getDataIsValid() && !digit.mTriggers.getOutputsAreBlocked()) {
      if (digit.mTriggers.getNChanA() > 0) {
        mHistNchA->Fill(digit.mTriggers.getNChanA());
        mHistSumAmpA->Fill(digit.mTriggers.getAmplA());
        mHistAverageTimeA->Fill(digit.mTriggers.getTimeA());
      }
      mHistPmTcmNchA->Fill(digit.mTriggers.getNChanA(), pmNChan - digit.mTriggers.getNChanA());
      mHistPmTcmSumAmpA->Fill(digit.mTriggers.getAmplA(), pmSumAmpl - digit.mTriggers.getAmplA());
      mHistPmTcmAverageTimeA->Fill(digit.mTriggers.getTimeA(), pmAverTime - digit.mTriggers.getTimeA());

      for (const auto& binPos : mHashedPairBitBinPos[digit.mTriggers.getTriggersignals()]) {
        mHistTriggersCorrelation->Fill(binPos.first, binPos.second);
      }
      for (const auto& binPos : mHashedBitBinPos[digit.mTriggers.getTriggersignals()]) {
        mHistBCvsTrg->Fill(digit.getIntRecord().bc, binPos);
        mHistOrbitVsTrg->Fill(digit.getIntRecord().orbit % sOrbitsPerTF, binPos);
      }

      // triggers re-computation
      mMapTrgSoftware[o2::fit::Triggers::bitA] = pmNChan > 0;
      mMapTrgSoftware[o2::fit::Triggers::bitTrgNchan] = pmNChan > mTrgThresholdNChannels;
      mMapTrgSoftware[o2::fit::Triggers::bitTrgCharge] = pmSumAmpl > 2 * mTrgThresholdCharge;

      if (mTrgModeInnerOuterThresholdVar == TrgModeThresholdVar::kAmpl) {
        mMapTrgSoftware[o2::fit::Triggers::bitAIn] = pmSumAmplIn > 2 * mTrgThresholdChargeInner;
        mMapTrgSoftware[o2::fit::Triggers::bitAOut] = pmSumAmplOut > 2 * mTrgThresholdChargeOuter;
      } else if (mTrgModeInnerOuterThresholdVar == TrgModeThresholdVar::kNchannels) {
        mMapTrgSoftware[o2::fit::Triggers::bitAIn] = pmNChanIn > mTrgThresholdNChannelsInner;
        mMapTrgSoftware[o2::fit::Triggers::bitAOut] = pmNChanOut > mTrgThresholdNChannelsOuter;
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
                           hasFired       = %d / %d \n \
                           nChannelsA     = %d / %d \n \
                           nChannelsInner = -- / %d \n \
                           nChannelsOuter = -- / %d \n \
                           timeA          = %d / %d \n \
                           sumTimeA       = -- / %d \n \
                           sumAmplA       = %d / %d \n \
                           sumAmplIn      = %d / %d \n \
                           sumAmplOut     = %d / %d \n \
                           TCM bits       = %d / -- \n",
            mMapDigitTrgNames[entry.first].c_str(),
            isTCMFired, isSwFired,
            digit.mTriggers.getNChanA(), pmNChan,
            pmNChanIn,
            pmNChanOut,
            digit.mTriggers.getTimeA(), pmAverTime,
            pmSumTime,
            digit.mTriggers.getAmplA(), pmSumAmpl,
            digit.mTriggers.getAmplC(), pmSumAmplIn,
            digit.mTriggers.getAmplA() - digit.mTriggers.getAmplC(), pmSumAmplOut,
            digit.mTriggers.getTriggersignals());
          ILOG(Debug, Support) << msg << ENDM;
        }
      }
      // end of triggers re-computation
    }
  } // digit loop
}

void DigitQcTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
  // add TF creation time for further match with filling scheme in PP in case of offline running
  ILOG(Debug, Support) << "adding last TF creation time: " << mTFcreationTime << ENDM;
  getObjectsManager()->getMonitorObject(mHistBCvsTrg->GetName())->addOrUpdateMetadata("TFcreationTime", std::to_string(mTFcreationTime));

  for (int channel = 1; channel <= sNCHANNELS_FV0_PLUSREF - 1; channel++) {
    float events_in_range = 0;
    float events_per_channel = 0;
    for (int bin_on_y_axis = 1; bin_on_y_axis <= mHistTime2Ch->GetNbinsY(); bin_on_y_axis++) {
      if (mHistTime2Ch->GetYaxis()->GetBinLowEdge(bin_on_y_axis) > mMinTimeGate && mHistTime2Ch->GetYaxis()->GetBinLowEdge(bin_on_y_axis) < mMaxTimeGate) {
        events_in_range += mHistTime2Ch->GetBinContent(channel, bin_on_y_axis);
      }
      events_per_channel += mHistTime2Ch->GetBinContent(channel, bin_on_y_axis);
    }
    if (events_per_channel) {
      mHistGateTimeRatio2Ch->SetBinContent(channel, events_in_range / events_per_channel);
    } else {
      mHistGateTimeRatio2Ch->SetBinContent(channel, 0);
    }
  }

  // one has to set num. of entries manually because
  // default TH1Reductor gets only mean,stddev and entries (no integral)
  mHistCFDEff->Divide(mHistNumADC.get(), mHistNumCFD.get());
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
  mHistGateTimeRatio2Ch->Reset();
  mHistTime2Ch->Reset();
  mHistAmp2Ch->Reset();
  mHistBC->Reset();
  mHistChDataBits->Reset();
  mHistCFDEff->Reset();
  mHistNumADC->Reset();
  mHistNumCFD->Reset();
  // mHistTimeSum2Diff->Reset();
  mHistOrbit2BC->Reset();
  mHistEventDensity2Ch->Reset();
  mHistNchA->Reset();
  // mHistNchC->Reset();
  mHistSumAmpA->Reset();
  // mHistSumAmpC->Reset();
  mHistAverageTimeA->Reset();
  // mHistAverageTimeC->Reset();
  mHistChannelID->Reset();
  mHistTriggersCorrelation->Reset();
  mHistCycleDuration->Reset();
  mHistCycleDurationNTF->Reset();
  mHistCycleDurationRange->Reset();
  mHistBCvsTrg->Reset();
  mHistBCvsFEEmodules->Reset();
  mHistBcVsFeeForOrATrg->Reset();
  mHistBcVsFeeForOrAOutTrg->Reset();
  mHistBcVsFeeForNChanTrg->Reset();
  mHistBcVsFeeForChargeTrg->Reset();
  mHistBcVsFeeForOrAInTrg->Reset();
  mHistOrbitVsTrg->Reset();
  mHistOrbitVsFEEmodules->Reset();
  mHistPmTcmNchA->Reset();
  mHistPmTcmSumAmpA->Reset();
  mHistPmTcmAverageTimeA->Reset();
  mHistTriggersSw->Reset();
  mHistTriggersSoftwareVsTCM->Reset();
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

} // namespace o2::quality_control_modules::fv0
