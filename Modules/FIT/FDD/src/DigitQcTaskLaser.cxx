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
/// \author Artur Furs afurs@cern.ch
/// modified by A Khuntia for FDD akhuntia@cern.ch

#include "TCanvas.h"
#include "TH1.h"
#include "TH2.h"
#include "TROOT.h"

#include "QualityControl/QcInfoLogger.h"
#include "FDD/DigitQcTaskLaser.h"
#include "DataFormatsFDD/Digit.h"
#include "DataFormatsFDD/ChannelData.h"
#include "Framework/InputRecord.h"

#include <DataFormatsFDD/LookUpTable.h>
#include <vector>

namespace o2::quality_control_modules::fdd
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
      for (const auto& chID : mSetRefPMTChIDs) {
        std::string hNameCur = hName.substr(0, hName.find(channelIdPlaceholder)) + std::to_string(chID) + hName.substr(hName.find(channelIdPlaceholder) + 1);
        rebinHisto(hNameCur, binning);
      }
    } else {
      rebinHisto(hName, binning);
    }
  }
}

void DigitQcTaskLaser::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize DigitQcTaskLaser" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  mStateLastIR2Ch = {};
  mMapChTrgNames.insert({ o2::fdd::ChannelData::kNumberADC, "NumberADC" });
  mMapChTrgNames.insert({ o2::fdd::ChannelData::kIsDoubleEvent, "IsDoubleEvent" });
  mMapChTrgNames.insert({ o2::fdd::ChannelData::kIsTimeInfoNOTvalid, "IsTimeInfoNOTvalid" });
  mMapChTrgNames.insert({ o2::fdd::ChannelData::kIsCFDinADCgate, "IsCFDinADCgate" });
  mMapChTrgNames.insert({ o2::fdd::ChannelData::kIsTimeInfoLate, "IsTimeInfoLate" });
  mMapChTrgNames.insert({ o2::fdd::ChannelData::kIsAmpHigh, "IsAmpHigh" });
  mMapChTrgNames.insert({ o2::fdd::ChannelData::kIsEventInTVDC, "IsEventInTVDC" });
  mMapChTrgNames.insert({ o2::fdd::ChannelData::kIsTimeInfoLost, "IsTimeInfoLost" });

  mMapDigitTrgNames.insert({ o2::fdd::Triggers::bitA, "OrA" });
  mMapDigitTrgNames.insert({ o2::fdd::Triggers::bitC, "OrC" });
  mMapDigitTrgNames.insert({ o2::fdd::Triggers::bitVertex, "Vertex" });
  mMapDigitTrgNames.insert({ o2::fdd::Triggers::bitCen, "Central" });
  mMapDigitTrgNames.insert({ o2::fdd::Triggers::bitSCen, "SemiCentral" });
  mMapDigitTrgNames.insert({ o2::fdd::Triggers::bitLaser, "Laser" });
  mMapDigitTrgNames.insert({ o2::fdd::Triggers::bitOutputsAreBlocked, "OutputsAreBlocked" });
  mMapDigitTrgNames.insert({ o2::fdd::Triggers::bitDataIsValid, "DataIsValid" });

  mHistTime2Ch = std::make_unique<TH2F>("TimePerChannel", "Time vs Channel;Channel;Time", sNCHANNELS_PM, 0, sNCHANNELS_PM, 4100, -2050, 2050);
  mHistTime2Ch->SetOption("colz");
  mHistAmp2Ch = std::make_unique<TH2F>("AmpPerChannel", "Amplitude vs Channel;Channel;Amp", sNCHANNELS_PM, 0, sNCHANNELS_PM, 4200, -100, 4100);
  mHistAmp2Ch->SetOption("colz");
  mHistOrbit2BC = std::make_unique<TH2F>("OrbitPerBC", "BC-Orbit map;Orbit;BC;", 256, 0, 256, sBCperOrbit, 0, sBCperOrbit);
  mHistOrbit2BC->SetOption("colz");
  mHistBC = std::make_unique<TH1F>("BC", "BC;BC;counts;", sBCperOrbit, 0, sBCperOrbit);

  mHistChDataBits = std::make_unique<TH2F>("ChannelDataBits", "ChannelData bits per ChannelID;Channel;Bit", sNCHANNELS_PM, 0, sNCHANNELS_PM, mMapChTrgNames.size(), 0, mMapChTrgNames.size());
  mHistChDataBits->SetOption("colz");

  for (const auto& entry : mMapChTrgNames) {
    mHistChDataBits->GetYaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());
  }

  mListHistGarbage = new TList();
  mListHistGarbage->SetOwner(kTRUE);

  char* p;
  for (const auto& lutEntry : o2::fdd::SingleLUT::Instance().getVecMetadataFEE()) {
    int chId = std::strtol(lutEntry.mChannelID.c_str(), &p, 10);
    if (*p) {
      // lutEntry.mChannelID is not a number
      continue;
    }
    auto moduleName = lutEntry.mModuleName;
    if (moduleName == "TCM") {
      if (mMapPmModuleChannels.find(moduleName) == mMapPmModuleChannels.end()) {
        mMapPmModuleChannels.insert({ moduleName, std::vector<int>{} });
      }
      continue;
    }
    if (mMapPmModuleChannels.find(moduleName) != mMapPmModuleChannels.end()) {
      mMapPmModuleChannels[moduleName].push_back(chId);
    } else {
      std::vector<int> vChId = {
        chId,
      };
      mMapPmModuleChannels.insert({ moduleName, vChId });
    }
  }

  for (const auto& entry : mMapPmModuleChannels) {
    auto pairModuleBcOrbit = mMapPmModuleBcOrbit.insert({ entry.first, new TH2F(Form("BcOrbitMap_%s", entry.first.c_str()), Form("BC-orbit map for %s;Orbit;BC", entry.first.c_str()), 256, 0, 256, sBCperOrbit, 0, sBCperOrbit) });
  }

  mHistNumADC = std::make_unique<TH1F>("HistNumADC", "HistNumADC", sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mHistNumCFD = std::make_unique<TH1F>("HistNumCFD", "HistNumCFD", sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mHistCFDEff = std::make_unique<TH1F>("CFD_efficiency", "CFD efficiency;ChannelID;efficiency", sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mHistCycleDuration = std::make_unique<TH1D>("CycleDuration", "Cycle Duration;;time [ns]", 1, 0, 2);

  std::vector<unsigned int> vecChannelIDs;
  std::vector<unsigned int> vecRefPMTChannelIDs;
  if (auto param = mCustomParameters.find("ChannelIDs"); param != mCustomParameters.end()) {
    const auto chIDs = param->second;
    const std::string del = ",";
    vecChannelIDs = parseParameters<unsigned int>(chIDs, del);
  } else {
    for (unsigned int iCh = 0; iCh < sNCHANNELS_PM; iCh++)
      vecChannelIDs.push_back(iCh);
  }
  if (auto param = mCustomParameters.find("RefPMTChannelIDs"); param != mCustomParameters.end()) {
    const auto chIDs = param->second;
    const std::string del = ",";
    vecRefPMTChannelIDs = parseParameters<unsigned int>(chIDs, del);
  }
  for (const auto& entry : vecChannelIDs) {
    mSetAllowedChIDs.insert(entry);
  }
  for (const auto& entry : vecRefPMTChannelIDs) {
    mSetRefPMTChIDs.insert(entry);
  }

  for (const auto& RefPMTChID : mSetRefPMTChIDs) {
    auto pairHistAmpVsBC = mMapHistAmpVsBC.insert({ RefPMTChID, new TH2F(Form("Amp_vs_BC_channel%i", RefPMTChID), Form("Amplitude vs BC, channel %i;Amp;BC", RefPMTChID), 1000, 0, 1000, 1000, 0, 1000) });
    if (pairHistAmpVsBC.second) {
      mListHistGarbage->Add(pairHistAmpVsBC.first->second);
      getObjectsManager()->startPublishing(pairHistAmpVsBC.first->second);
    }
  }

  rebinFromConfig(); // after all histos are created
  getObjectsManager()->startPublishing(mHistTime2Ch.get());
  getObjectsManager()->startPublishing(mHistAmp2Ch.get());
  getObjectsManager()->startPublishing(mHistOrbit2BC.get());
  getObjectsManager()->startPublishing(mHistBC.get());
  getObjectsManager()->startPublishing(mHistCFDEff.get());
  getObjectsManager()->startPublishing(mHistCycleDuration.get());

  for (int i = 0; i < getObjectsManager()->getNumberPublishedObjects(); i++) {
    TH1* obj = dynamic_cast<TH1*>(getObjectsManager()->getMonitorObject(i)->getObject());
    obj->SetTitle((string("FDD Laser ") + obj->GetTitle()).c_str());
  }
}

void DigitQcTaskLaser::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity" << activity.mId << ENDM;
  mHistTime2Ch->Reset();
  mHistAmp2Ch->Reset();
  mHistOrbit2BC->Reset();
  mHistBC->Reset();
  mHistCFDEff->Reset();
  mHistNumADC->Reset();
  mHistNumCFD->Reset();
  mHistCycleDuration->Reset();
  for (auto& entry : mMapHistAmpVsBC) {
    entry.second->Reset();
  }
  for (auto& entry : mMapPmModuleBcOrbit) {
    entry.second->Reset();
  }
}

void DigitQcTaskLaser::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
  mTimeMinNS = -1;
  mTimeMaxNS = 0.;
  mTimeCurNS = 0.;
  mTfCounter = 0;
  mTimeSum = 0.;
}

void DigitQcTaskLaser::monitorData(o2::framework::ProcessingContext& ctx)
{
  double curTfTimeMin = -1;
  double curTfTimeMax = 0;
  mTfCounter++;
  auto channels = ctx.inputs().get<gsl::span<o2::fdd::ChannelData>>("channels");
  auto digits = ctx.inputs().get<gsl::span<o2::fdd::Digit>>("digits");
  bool isFirst = true;
  uint32_t firstOrbit;

  for (auto& digit : digits) {
    // Exclude all BCs, in which laser signals are NOT expected (and trigger outputs are NOT blocked)
    if (!digit.mTriggers.getOutputsAreBlocked()) {
      continue;
    }
    const auto& vecChData = digit.getBunchChannelData(channels);
    bool isTCM = true;
    mTimeCurNS = o2::InteractionRecord::bc2ns(digit.getBC(), digit.getOrbit());
    if (mTimeMinNS < 0)
      mTimeMinNS = mTimeCurNS;
    if (isFirst == true) {
      firstOrbit = digit.getOrbit();
      isFirst = false;
    }
    if (mTimeCurNS < mTimeMinNS)
      mTimeMinNS = mTimeCurNS;
    if (mTimeCurNS > mTimeMaxNS)
      mTimeMaxNS = mTimeCurNS;

    if (curTfTimeMin < 0)
      curTfTimeMin = mTimeCurNS;
    if (mTimeCurNS < curTfTimeMin)
      curTfTimeMin = mTimeCurNS;
    if (mTimeCurNS > curTfTimeMax)
      curTfTimeMax = mTimeCurNS;

    if (digit.mTriggers.getTimeA() == o2::fit::Triggers::DEFAULT_TIME && digit.mTriggers.getTimeC() == o2::fit::Triggers::DEFAULT_TIME) {
      isTCM = false;
    }
    mHistOrbit2BC->Fill(digit.getIntRecord().orbit % sOrbitsPerTF, digit.getIntRecord().bc);
    mHistBC->Fill(digit.getBC());

    for (auto& entry : mMapPmModuleChannels) {
      for (const auto& chData : vecChData) {
        if (std::find(entry.second.begin(), entry.second.end(), chData.mPMNumber) != entry.second.end()) {
          mMapPmModuleBcOrbit[entry.first]->Fill(digit.getOrbit() % sOrbitsPerTF, digit.getBC());
          break;
        }
      }
      if (entry.first == "TCM" && isTCM && (digit.mTriggers.getDataIsValid())) {
        mMapPmModuleBcOrbit[entry.first]->Fill(digit.getOrbit() % sOrbitsPerTF, digit.getBC());
      }
    }

    for (const auto& chData : vecChData) {
      mHistTime2Ch->Fill(static_cast<Double_t>(chData.mPMNumber), static_cast<Double_t>(chData.mTime));
      mHistAmp2Ch->Fill(static_cast<Double_t>(chData.mPMNumber), static_cast<Double_t>(chData.mChargeADC));
      mStateLastIR2Ch[chData.mPMNumber] = digit.mIntRecord;

      if (chData.mChargeADC > 0)
        mHistNumADC->Fill(chData.mPMNumber);
      mHistNumCFD->Fill(chData.mPMNumber);

      if (mSetRefPMTChIDs.find(static_cast<unsigned int>(chData.mPMNumber)) != mSetRefPMTChIDs.end()) {
        mMapHistAmpVsBC[chData.mPMNumber]->Fill(chData.mChargeADC, digit.getIntRecord().bc);
      }

      for (const auto& entry : mMapChTrgNames) {
        if ((chData.mFEEBits & (1 << entry.first))) {
          mHistChDataBits->Fill(chData.mPMNumber, entry.first);
        }
      }
    }
    mHistCFDEff->Reset();
    mHistCFDEff->Divide(mHistNumADC.get(), mHistNumCFD.get());
  }
  mTimeSum += curTfTimeMax - curTfTimeMin;
}

void DigitQcTaskLaser::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
  mHistCycleDuration->SetBinContent(1., mTimeSum);
  mHistCycleDuration->SetEntries(mTimeSum);
  ILOG(Debug) << "Cycle duration: NTF=" << mTfCounter << ", range = " << (mTimeMaxNS - mTimeMinNS) / 1e6 / mTfCounter << " ms/TF, sum = " << mTimeSum / 1e6 / mTfCounter << " ms/TF" << ENDM;
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
  mHistOrbit2BC->Reset();
  mHistBC->Reset();
  mHistChDataBits->Reset();
  mHistCFDEff->Reset();
  mHistNumADC->Reset();
  mHistNumCFD->Reset();
  mHistCycleDuration->Reset();
  for (auto& entry : mMapHistAmpVsBC) {
    entry.second->Reset();
  }
  for (auto& entry : mMapPmModuleBcOrbit) {
    entry.second->Reset();
  }
}

} // namespace o2::quality_control_modules::fdd
