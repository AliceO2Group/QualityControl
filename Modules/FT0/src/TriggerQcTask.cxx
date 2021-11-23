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
/// \file   TriggerQcTask.cxx
/// \author Sebastian Bysiak sbysiak@cern.ch
///

#include "FT0/TriggerQcTask.h"
#include <Framework/InputRecord.h>

namespace o2::quality_control_modules::ft0
{

TriggerQcTask::~TriggerQcTask()
{
}

unsigned int TriggerQcTask::getModeParameter(std::string paramName, unsigned int defaultVal, std::map<unsigned int, std::string> choices)
{
  if (auto param = mCustomParameters.find(paramName); param != mCustomParameters.end()) {
    // if parameter was provided check which option was chosen
    for (const auto& choice : choices) {
      if (param->second == choice.second) {
        ILOG(Debug) << "setting \"" << paramName << "\" to: \"" << choice.second << "\"" << ENDM;
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
    ILOG(Warning) << "Provided value (\"" << param->second << "\") for parameter \"" << paramName << "\" is not allowed. Allowed values are: " << allowedValues << " setting \"" << paramName << "\" to default value: \"" << choices[defaultVal] << "\"" << ENDM;
    return defaultVal;
  } else {
    // param not provided - use default
    ILOG(Debug) << "Setting \"" << paramName << "\" to default value: \"" << choices[defaultVal] << "\"" << ENDM;
    return defaultVal;
  }
}

int TriggerQcTask::getNumericalParameter(std::string paramName, int defaultVal)
{
  if (auto param = mCustomParameters.find(paramName); param != mCustomParameters.end()) {
    float val = stoi(param->second);
    ILOG(Debug) << "Setting \"" << paramName << "\" to: " << val << ENDM;
    return val;
  } else {
    ILOG(Debug) << "Setting \"" << paramName << "\" to default value: " << defaultVal << ENDM;
    return defaultVal;
  }
}

void TriggerQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize TriggerQcTask" << ENDM;

  mMapDigitTrgNames.insert({ o2::ft0::Triggers::bitA, "OrA" });
  mMapDigitTrgNames.insert({ o2::ft0::Triggers::bitC, "OrC" });
  mMapDigitTrgNames.insert({ o2::ft0::Triggers::bitVertex, "Vertex" });
  mMapDigitTrgNames.insert({ o2::ft0::Triggers::bitCen, "Central" });
  mMapDigitTrgNames.insert({ o2::ft0::Triggers::bitSCen, "SemiCentral" });

  mModeThresholdVar = getModeParameter("modeThresholdVar",
                                       TrgModeThresholdVar::kAmpl,
                                       { { TrgModeThresholdVar::kAmpl, "Ampl" },
                                         { TrgModeThresholdVar::kNchannels, "Nchannels" } });
  mModeSide = getModeParameter("modeSide",
                               TrgModeSide::kAplusC,
                               { { TrgModeSide::kAplusC, "A+C" },
                                 { TrgModeSide::kAandC, "A&C" },
                                 { TrgModeSide::kA, "A" },
                                 { TrgModeSide::kC, "C" } });
  mThresholdTimeLow = getNumericalParameter("thresholdTimeLow", -192);
  mThresholdTimeHigh = getNumericalParameter("thresholdTimeHigh", 192);
  if (mModeSide == TrgModeSide::kAplusC) {
    mThresholdSCenSum = getNumericalParameter("thresholdSCenSum", 300);
    mThresholdCenSum = getNumericalParameter("thresholdCenSum", 600);
  } else if (mModeSide == TrgModeSide::kAandC) {
    mThresholdCenA = getNumericalParameter("thresholdCenA", 600);
    mThresholdCenC = getNumericalParameter("thresholdCenC", 600);
    mThresholdSCenA = getNumericalParameter("thresholdSCenA", 300);
    mThresholdSCenC = getNumericalParameter("thresholdSCenC", 300);
  } else if (mModeSide == TrgModeSide::kA) {
    mThresholdCenA = getNumericalParameter("thresholdCenA", 600);
    mThresholdSCenA = getNumericalParameter("thresholdSCenA", 300);
  } else if (mModeSide == TrgModeSide::kC) {
    mThresholdCenC = getNumericalParameter("thresholdCenC", 600);
    mThresholdSCenC = getNumericalParameter("thresholdSCenC", 300);
  }

  mHistTriggersSw = std::make_unique<TH1F>("mHistTriggersSw", "Triggers from software", mMapDigitTrgNames.size(), 0, mMapDigitTrgNames.size());
  mHistTriggersSoftwareVsTCM = std::make_unique<TH2F>("TriggersSoftwareVsTCM", "Comparison of triggers from software and TCM;;Trigger name", mMapDigitTrgNames.size(), 0, mMapDigitTrgNames.size(), 4, 0, 4);
  mHistTriggersSoftwareVsTCM->SetOption("colz");
  mHistTriggersSoftwareVsTCM->SetStats(0);
  for (const auto& entry : mMapDigitTrgNames) {
    mHistTriggersSw->GetXaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());
    mHistTriggersSoftwareVsTCM->GetXaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());
  }
  mHistTriggersSoftwareVsTCM->GetYaxis()->SetBinLabel(ComparisonResult::kSWonly + 1, "Sw only");
  mHistTriggersSoftwareVsTCM->GetYaxis()->SetBinLabel(ComparisonResult::kTCMonly + 1, "TCM only");
  mHistTriggersSoftwareVsTCM->GetYaxis()->SetBinLabel(ComparisonResult::kNone + 1, "neither TCM nor Sw");
  mHistTriggersSoftwareVsTCM->GetYaxis()->SetBinLabel(ComparisonResult::kBoth + 1, "both TCM and Sw");

  getObjectsManager()->startPublishing(mHistTriggersSw.get());
  getObjectsManager()->startPublishing(mHistTriggersSoftwareVsTCM.get());
}

void TriggerQcTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity " << activity.mId << ENDM;
  mHistTriggersSw->Reset();
  mHistTriggersSoftwareVsTCM->Reset();
}

void TriggerQcTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void TriggerQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  auto channels = ctx.inputs().get<gsl::span<o2::ft0::ChannelData>>("channels");
  auto digits = ctx.inputs().get<gsl::span<o2::ft0::Digit>>("digits");

  for (auto& digit : digits) {
    const auto& vecChData = digit.getBunchChannelData(channels);
    // reset triggers
    for (auto& entry : mMapTrgSoftware) {
      mMapTrgSoftware[entry.first] = false;
    }

    float sumAmplA = 0;
    float sumAmplC = 0;
    int sumTimeA = 0;
    int sumTimeC = 0;
    int nFiredChannelsA = 0;
    int nFiredChannelsC = 0;
    for (const auto& chData : vecChData) {
      if (chData.QTCAmpl <= 0)
        continue;

      if (chData.ChId < mNChannelsA) {
        sumAmplA += chData.QTCAmpl;
        sumTimeA += chData.CFDTime;
        nFiredChannelsA++;
      } else {
        sumAmplC += chData.QTCAmpl;
        sumTimeC += chData.CFDTime;
        nFiredChannelsC++;
      }
    }

    mMapTrgSoftware[o2::ft0::Triggers::bitA] = nFiredChannelsA > 0;
    mMapTrgSoftware[o2::ft0::Triggers::bitC] = nFiredChannelsC > 0;

    int avgTimeA = nFiredChannelsA ? int(sumTimeA / nFiredChannelsA) : 0;
    int avgTimeC = nFiredChannelsC ? int(sumTimeC / nFiredChannelsC) : 0;
    int vtxPos = (nFiredChannelsA && nFiredChannelsC) ? (avgTimeC - avgTimeA) / 2 : 0;
    if (mThresholdTimeLow < vtxPos && vtxPos < mThresholdTimeHigh && nFiredChannelsA && nFiredChannelsC)
      mMapTrgSoftware[o2::ft0::Triggers::bitVertex] = true;

    // Central/SemiCentral logic
    switch (mModeSide) {
      case TrgModeSide::kAplusC:
        if (mModeThresholdVar == TrgModeThresholdVar::kAmpl) {
          if (sumAmplA + sumAmplC >= mThresholdCenSum)
            mMapTrgSoftware[o2::ft0::Triggers::bitCen] = true;
          if (sumAmplA + sumAmplC >= mThresholdSCenSum)
            mMapTrgSoftware[o2::ft0::Triggers::bitSCen] = true;
        } else if (mModeThresholdVar == TrgModeThresholdVar::kNchannels) {
          if (nFiredChannelsA + nFiredChannelsC >= mThresholdCenSum)
            mMapTrgSoftware[o2::ft0::Triggers::bitCen] = true;
          if (nFiredChannelsA + nFiredChannelsA >= mThresholdSCenSum)
            mMapTrgSoftware[o2::ft0::Triggers::bitSCen] = true;
        }
        break;

      case TrgModeSide::kAandC:
        if (mModeThresholdVar == TrgModeThresholdVar::kAmpl) {
          if (sumAmplA >= mThresholdCenA && sumAmplC >= mThresholdCenC)
            mMapTrgSoftware[o2::ft0::Triggers::bitCen] = true;
          if (sumAmplA >= mThresholdSCenA && sumAmplC >= mThresholdSCenC)
            mMapTrgSoftware[o2::ft0::Triggers::bitSCen] = true;
        } else if (mModeThresholdVar == TrgModeThresholdVar::kNchannels) {
          if (nFiredChannelsA >= mThresholdCenA && nFiredChannelsC >= mThresholdCenC)
            mMapTrgSoftware[o2::ft0::Triggers::bitCen] = true;
          if (nFiredChannelsA >= mThresholdSCenA && nFiredChannelsC >= mThresholdSCenC)
            mMapTrgSoftware[o2::ft0::Triggers::bitSCen] = true;
        }
        break;

      case TrgModeSide::kA:
        if (mModeThresholdVar == TrgModeThresholdVar::kAmpl) {
          if (sumAmplA >= mThresholdCenA)
            mMapTrgSoftware[o2::ft0::Triggers::bitCen] = true;
          if (sumAmplA >= mThresholdSCenA)
            mMapTrgSoftware[o2::ft0::Triggers::bitSCen] = true;
        } else if (mModeThresholdVar == TrgModeThresholdVar::kNchannels) {
          if (nFiredChannelsA >= mThresholdCenA)
            mMapTrgSoftware[o2::ft0::Triggers::bitCen] = true;
          if (nFiredChannelsA >= mThresholdSCenA)
            mMapTrgSoftware[o2::ft0::Triggers::bitSCen] = true;
        }
        break;

      case TrgModeSide::kC:
        if (mModeThresholdVar == TrgModeThresholdVar::kAmpl) {
          if (sumAmplC >= mThresholdCenC)
            mMapTrgSoftware[o2::ft0::Triggers::bitCen] = true;
          if (sumAmplC >= mThresholdSCenC)
            mMapTrgSoftware[o2::ft0::Triggers::bitSCen] = true;
        } else if (mModeThresholdVar == TrgModeThresholdVar::kNchannels) {
          if (nFiredChannelsC >= mThresholdCenC)
            mMapTrgSoftware[o2::ft0::Triggers::bitCen] = true;
          if (nFiredChannelsC >= mThresholdSCenC)
            mMapTrgSoftware[o2::ft0::Triggers::bitSCen] = true;
        }
        break;
    }

    for (const auto& entry : mMapTrgSoftware)
      if (entry.second)
        mHistTriggersSw->Fill(entry.first);

    for (const auto& entry : mMapTrgSoftware) {
      bool isTCMFired = digit.mTriggers.triggersignals & (1 << entry.first);
      bool isSwFired = entry.second;
      if (!isTCMFired && isSwFired)
        mHistTriggersSoftwareVsTCM->Fill(entry.first, ComparisonResult::kSWonly);
      else if (isTCMFired && !isSwFired)
        mHistTriggersSoftwareVsTCM->Fill(entry.first, ComparisonResult::kTCMonly);
      else if (!isTCMFired && !isSwFired)
        mHistTriggersSoftwareVsTCM->Fill(entry.first, ComparisonResult::kNone);
      else if (isTCMFired && isSwFired)
        mHistTriggersSoftwareVsTCM->Fill(entry.first, ComparisonResult::kBoth);

      if (isTCMFired != isSwFired) {
        // (*) = triggers.amplA/C are sums of amplitudes **divided by 8**
        auto msg = Form(
          "Software does not reproduce TCM decision! \n \
                         trigger name: %s\n \
                         TCM / SW: \n \
                         hasFired   = %d / %d \n \
                         nChannelsA = %d / %d \n \
                         nChannelsC = %d / %d \n \
                         sumAmplA   = %d / %d (*) \n \
                         sumAmplC   = %d / %d (*) \n \
                         timeA      = %d / %d \n \
                         timeC      = %d / %d \n \
                         vertexPos  = -- / %d",
          mMapDigitTrgNames[entry.first].c_str(),
          isTCMFired, isSwFired,
          digit.mTriggers.nChanA, nFiredChannelsA,
          digit.mTriggers.nChanC, nFiredChannelsC,
          digit.mTriggers.amplA, int(sumAmplA / 8),
          digit.mTriggers.amplC, int(sumAmplC / 8),
          digit.mTriggers.timeA, avgTimeA,
          digit.mTriggers.timeC, avgTimeC, vtxPos);
        ILOG(Debug, Support) << msg << ENDM;
      }
    }
  }
}

void TriggerQcTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void TriggerQcTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void TriggerQcTask::reset()
{
  // clean all the monitor objects here
  ILOG(Info, Support) << "Resetting the histograms" << ENDM;
  mHistTriggersSw->Reset();
  mHistTriggersSoftwareVsTCM->Reset();
}

} // namespace o2::quality_control_modules::ft0
