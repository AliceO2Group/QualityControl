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

#include "FV0/TriggerQcTask.h"
#include <Framework/InputRecord.h>

namespace o2::quality_control_modules::fv0
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

int TriggerQcTask::getNumericalParameter(std::string paramName, int defaultVal)
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

void TriggerQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize TriggerQcTask" << ENDM;

  mMapDigitTrgNames.insert({ ETrgMenu::kMinBias, "MinBias" });
  mMapDigitTrgNames.insert({ ETrgMenu::kOuterRing, "OuterRing" });
  mMapDigitTrgNames.insert({ ETrgMenu::kNChannels, "NChannels" });
  mMapDigitTrgNames.insert({ ETrgMenu::kCharge, "Charge" });
  mMapDigitTrgNames.insert({ ETrgMenu::kInnerRing, "InnerRing" });

  mModeInnerOuterThresholdVar = getModeParameter("modeInnerOuterThresholdVar",
                                                 TrgModeThresholdVar::kNchannels,
                                                 { { TrgModeThresholdVar::kAmpl, "Ampl" },
                                                   { TrgModeThresholdVar::kNchannels, "Nchannels" } });

  mThresholdCharge = getNumericalParameter("thresholdCharge", 498 * 3);
  mThresholdNChannels = getNumericalParameter("thresholdNChannels", 10);

  if (mModeInnerOuterThresholdVar == TrgModeThresholdVar::kAmpl) {
    mThresholdChargeInner = getNumericalParameter("thresholdChargeInner", 50);
    mThresholdChargeOuter = getNumericalParameter("thresholdChargeOuter", 50);
  } else if (mModeInnerOuterThresholdVar == TrgModeThresholdVar::kNchannels) {
    mThresholdNChannelsInner = getNumericalParameter("thresholdNChannelsInner", 1);
    mThresholdNChannelsOuter = getNumericalParameter("thresholdNChannelsOuter", 1);
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
  auto channels = ctx.inputs().get<gsl::span<o2::fv0::ChannelData>>("channels");
  auto digits = ctx.inputs().get<gsl::span<o2::fv0::Digit>>("digits");

  for (auto& digit : digits) {
    const auto& vecChData = digit.getBunchChannelData(channels);
    // reset triggers
    for (auto& entry : mMapTrgSoftware) {
      mMapTrgSoftware[entry.first] = false;
    }

    float sumAmplInner = 0;
    float sumAmplOuter = 0;
    int nFiredChannelsInner = 0;
    int nFiredChannelsOuter = 0;
    for (const auto& chData : vecChData) {
      if (chData.QTCAmpl <= 0)
        continue;

      if (chData.ChId >= sNCHANNELS_PM) { // skip reference PMT
        continue;
      }

      if (chData.ChId < sNCHANNELS_PM_INNER) {
        sumAmplInner += chData.QTCAmpl;
        nFiredChannelsInner++;
      } else {
        sumAmplOuter += chData.QTCAmpl;
        nFiredChannelsOuter++;
      }
    }

    mMapTrgSoftware[ETrgMenu::kMinBias] = nFiredChannelsInner + nFiredChannelsOuter >= 1;
    mMapTrgSoftware[ETrgMenu::kNChannels] = nFiredChannelsInner + nFiredChannelsOuter > mThresholdNChannels;
    mMapTrgSoftware[ETrgMenu::kCharge] = sumAmplInner + sumAmplOuter > mThresholdCharge;

    if (mModeInnerOuterThresholdVar == TrgModeThresholdVar::kAmpl) {
      mMapTrgSoftware[ETrgMenu::kInnerRing] = sumAmplInner >= mThresholdChargeInner;
      mMapTrgSoftware[ETrgMenu::kOuterRing] = sumAmplOuter >= mThresholdChargeOuter;
    } else if (mModeInnerOuterThresholdVar == TrgModeThresholdVar::kNchannels) {
      mMapTrgSoftware[ETrgMenu::kInnerRing] = nFiredChannelsInner >= mThresholdNChannelsInner;
      mMapTrgSoftware[ETrgMenu::kOuterRing] = nFiredChannelsOuter >= mThresholdNChannelsOuter;
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
          digit.mTriggers.nChanA, nFiredChannelsInner + nFiredChannelsOuter,
          nFiredChannelsInner,
          nFiredChannelsOuter,
          digit.mTriggers.amplA, int(sumAmplInner + sumAmplOuter),
          int(sumAmplInner),
          int(sumAmplOuter));
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

} // namespace o2::quality_control_modules::fv0
