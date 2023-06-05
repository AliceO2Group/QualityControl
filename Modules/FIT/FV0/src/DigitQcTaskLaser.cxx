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
/// \file   DigitQcTaskLaser.cxx
/// \brief  Based on the existing DigitQcTask
/// \author Sandor Lokos sandor.lokos@cern.ch

#include "FV0/DigitQcTaskLaser.h"

#include "TCanvas.h"
#include "TROOT.h"

#include "QualityControl/QcInfoLogger.h"
#include "DataFormatsFIT/Triggers.h"
#include "Framework/InputRecord.h"
#include "DataFormatsFV0/LookUpTable.h"

namespace o2::quality_control_modules::fv0
{

DigitQcTaskLaser::~DigitQcTaskLaser()
{
  delete mListHistGarbage;
}

void DigitQcTaskLaser::rebinFromConfig()
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

unsigned int DigitQcTaskLaser::getModeParameter(std::string paramName, unsigned int defaultVal, std::map<unsigned int, std::string> choices)
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

int DigitQcTaskLaser::getNumericalParameter(std::string paramName, int defaultVal)
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

void DigitQcTaskLaser::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize DigitQcTaskLaser" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
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

  mTrgModeInnerOuterThresholdVar = getModeParameter("trgModeInnerOuterThresholdVar",
                                                    TrgModeThresholdVar::kNchannels,
                                                    { { TrgModeThresholdVar::kAmpl, "Ampl" },
                                                      { TrgModeThresholdVar::kNchannels, "Nchannels" } });

  mTrgThresholdCharge = getNumericalParameter("trgThresholdCharge", 498 * 3);
  mTrgThresholdNChannels = getNumericalParameter("trgThresholdNChannels", 10);

  if (mTrgModeInnerOuterThresholdVar == TrgModeThresholdVar::kAmpl) {
    mTrgThresholdChargeInner = getNumericalParameter("trgThresholdChargeInner", 50);
    mTrgThresholdChargeOuter = getNumericalParameter("trgThresholdChargeOuter", 50);
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
  mHistTriggersSw = std::make_unique<TH1F>("TriggersSoftware", "Triggers from software", mMapDigitTrgNames.size(), 0, mMapDigitTrgNames.size());
  mHistTriggersSoftwareVsTCM = std::make_unique<TH2F>("TriggersSoftwareVsTCM", "Comparison of triggers from software and TCM;;Trigger name", mMapDigitTrgNames.size(), 0, mMapDigitTrgNames.size(), 4, 0, 4);
  mHistTriggersSoftwareVsTCM->SetOption("colz");
  mHistTriggersSoftwareVsTCM->SetStats(0);
  for (const auto& entry : mMapDigitTrgNames) {
    mHistOrbitVsTrg->GetYaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());
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

  // mHistTimeSum2Diff = std::make_unique<TH2F>("timeSumVsDiff", "time A/C side: sum VS diff;(TOC-TOA)/2 [ns];(TOA+TOC)/2 [ns]", 400, -52.08, 52.08, 400, -52.08, 52.08); // range of 52.08 ns = 4000*13.02ps = 4000 channels
  mHistNumADC = std::make_unique<TH1F>("HistNumADC", "HistNumADC", sNCHANNELS_FV0_PLUSREF, 0, sNCHANNELS_FV0_PLUSREF);
  mHistNumCFD = std::make_unique<TH1F>("HistNumCFD", "HistNumCFD", sNCHANNELS_FV0_PLUSREF, 0, sNCHANNELS_FV0_PLUSREF);
  mHistCFDEff = std::make_unique<TH1F>("CFD_efficiency", "CFD efficiency;ChannelID;efficiency", sNCHANNELS_FV0_PLUSREF, 0, sNCHANNELS_FV0_PLUSREF);

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
  getObjectsManager()->startPublishing(mHistCFDEff.get());
  getObjectsManager()->startPublishing(mHistBC.get());
  getObjectsManager()->startPublishing(mHistTriggersSw.get());
  // 2-dim hists
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
  // getObjectsManager()->startPublishing(mHistTimeSum2Diff.get());
  // getObjectsManager()->setDefaultDrawOptions(mHistTimeSum2Diff.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistTriggersSoftwareVsTCM.get());
  getObjectsManager()->setDefaultDrawOptions(mHistTriggersSoftwareVsTCM.get(), "COLZ");

  for (int i = 0; i < getObjectsManager()->getNumberPublishedObjects(); i++) {
    TH1* obj = dynamic_cast<TH1*>(getObjectsManager()->getMonitorObject(i)->getObject());
    obj->SetTitle((string("FV0 Laser ") + obj->GetTitle()).c_str());
  }
}

void DigitQcTaskLaser::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity" << activity.mId << ENDM;
  mHistTime2Ch->Reset();
  mHistAmp2Ch->Reset();
  mHistBC->Reset();
  mHistChDataBits->Reset();
  mHistCFDEff->Reset();
  mHistNumADC->Reset();
  mHistNumCFD->Reset();
  // mHistTimeSum2Diff->Reset();
  mHistBCvsFEEmodules->Reset();
  mHistOrbitVsTrg->Reset();
  mHistOrbitVsFEEmodules->Reset();
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

void DigitQcTaskLaser::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void DigitQcTaskLaser::monitorData(o2::framework::ProcessingContext& ctx)
{
  auto channels = ctx.inputs().get<gsl::span<o2::fv0::ChannelData>>("channels");
  auto digits = ctx.inputs().get<gsl::span<o2::fv0::Digit>>("digits");
  for (auto& digit : digits) {
    // Exclude all BCs, in which laser signals are NOT expected (and trigger outputs are NOT blocked)
    if (!digit.mTriggers.getOutputsAreBlocked()) {
      continue;
    }
    const auto& vecChData = digit.getBunchChannelData(channels);
    bool isTCM = true;
    if (digit.mTriggers.getTimeA() == o2::fit::Triggers::DEFAULT_TIME && digit.mTriggers.getTimeC() == o2::fit::Triggers::DEFAULT_TIME) {
      isTCM = false;
    }
    mHistBC->Fill(digit.getBC());
    if (isTCM && digit.mTriggers.getDataIsValid() && digit.mTriggers.getOutputsAreBlocked()) {
      // mHistTimeSum2Diff->Fill((digit.mTriggers.getTimeC() - digit.mTriggers.getTimeA()) * sCFDChannel2NS / 2, (digit.mTriggers.getTimeC() + digit.mTriggers.getTimeA()) * sCFDChannel2NS / 2);
      for (const auto& binPos : mHashedBitBinPos[digit.mTriggers.getTriggersignals()]) {
        mHistOrbitVsTrg->Fill(digit.getIntRecord().orbit % sOrbitsPerTF, binPos);
      }
    }
    std::set<uint8_t> setFEEmodules{};

    // reset triggers
    for (auto& entry : mMapTrgSoftware) {
      mMapTrgSoftware[entry.first] = false;
    }
    float sumAmplInner = 0;
    float sumAmplOuter = 0;
    int nFiredChannelsInner = 0;
    int nFiredChannelsOuter = 0;
    for (const auto& chData : vecChData) {
      mHistTime2Ch->Fill(static_cast<Double_t>(chData.ChId), static_cast<Double_t>(chData.CFDTime));
      mHistAmp2Ch->Fill(static_cast<Double_t>(chData.ChId), static_cast<Double_t>(chData.QTCAmpl));
      mStateLastIR2Ch[chData.ChId] = digit.mIntRecord;
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

      if (chData.ChId < sNCHANNELS_FV0_INNER) {
        sumAmplInner += chData.QTCAmpl;
        nFiredChannelsInner++;
      } else {
        sumAmplOuter += chData.QTCAmpl;
        nFiredChannelsOuter++;
      }
    }
    if (isTCM) {
      setFEEmodules.insert(mTCMhash);
    }
    for (const auto& feeHash : setFEEmodules) {
      mHistBCvsFEEmodules->Fill(static_cast<double>(digit.getIntRecord().bc), static_cast<double>(feeHash));
      mHistOrbitVsFEEmodules->Fill(static_cast<double>(digit.getIntRecord().orbit % sOrbitsPerTF), static_cast<double>(feeHash));
    }

    // triggers re-computation
    mMapTrgSoftware[o2::fit::Triggers::bitA] = nFiredChannelsInner + nFiredChannelsOuter >= 1;
    mMapTrgSoftware[o2::fit::Triggers::bitTrgNchan] = nFiredChannelsInner + nFiredChannelsOuter > mTrgThresholdNChannels;
    mMapTrgSoftware[o2::fit::Triggers::bitTrgCharge] = sumAmplInner + sumAmplOuter > mTrgThresholdCharge;

    if (mTrgModeInnerOuterThresholdVar == TrgModeThresholdVar::kAmpl) {
      mMapTrgSoftware[o2::fit::Triggers::bitAIn] = sumAmplInner >= mTrgThresholdChargeInner;
      mMapTrgSoftware[o2::fit::Triggers::bitAOut] = sumAmplOuter >= mTrgThresholdChargeOuter;
    } else if (mTrgModeInnerOuterThresholdVar == TrgModeThresholdVar::kNchannels) {
      mMapTrgSoftware[o2::fit::Triggers::bitAIn] = nFiredChannelsInner >= mTrgThresholdNChannelsInner;
      mMapTrgSoftware[o2::fit::Triggers::bitAOut] = nFiredChannelsOuter >= mTrgThresholdNChannelsOuter;
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
                         sumAmplA       = %d / %d \n \
                         sumAmplInner   = -- / %d \n \
                         sumAmplOuter   = -- / %d \n",
          mMapDigitTrgNames[entry.first].c_str(),
          isTCMFired, isSwFired,
          digit.mTriggers.getNChanA(), nFiredChannelsInner + nFiredChannelsOuter,
          nFiredChannelsInner,
          nFiredChannelsOuter,
          digit.mTriggers.getAmplA(), int(sumAmplInner + sumAmplOuter),
          int(sumAmplInner),
          int(sumAmplOuter));
        ILOG(Debug, Support) << msg << ENDM;
      }
    }
    // end of triggers re-computation
  }
}

void DigitQcTaskLaser::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
  // one has to set num. of entries manually because
  // default TH1Reductor gets only mean,stddev and entries (no integral)
  mHistCFDEff->Divide(mHistNumADC.get(), mHistNumCFD.get());
}

void DigitQcTaskLaser::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void DigitQcTaskLaser::reset()
{
  // clean all the monitor objects here
  mHistTime2Ch->Reset();
  mHistAmp2Ch->Reset();
  mHistBC->Reset();
  mHistChDataBits->Reset();
  mHistCFDEff->Reset();
  mHistNumADC->Reset();
  mHistNumCFD->Reset();
  // mHistTimeSum2Diff->Reset();
  mHistBCvsFEEmodules->Reset();
  mHistOrbitVsTrg->Reset();
  mHistOrbitVsFEEmodules->Reset();
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
