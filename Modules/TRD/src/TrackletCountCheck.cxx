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
/// \file   TrackletCountCheck.cxx
/// \author Deependra Sharma @IITB
///

#include "TRD/TrackletCountCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include "Common/Utils.h"
// ROOT
#include <TH1.h>
#include <TPaveText.h>

#include <DataFormatsQualityControl/FlagReasons.h>

using namespace std;
using namespace o2::quality_control;
using namespace o2::quality_control_modules::common;

namespace o2::quality_control_modules::trd
{

void TrackletCountCheck::configure()
{
  ILOG(Debug, Devel) << "initializing TrackletCountCheck" << ENDM;

  mThresholdMeanHighPerTimeFrame = getFromConfig<float>(mCustomParameters, "UpperthresholdPerTimeFrame", 520.f);
  ILOG(Debug, Support) << "using Upperthreshold Per Timeframe= " << mThresholdMeanHighPerTimeFrame << ENDM;

  mThresholdMeanLowPerTimeFrame = getFromConfig<float>(mCustomParameters, "LowerthresholdPerTimeFrame", 600.f);
  ILOG(Debug, Support) << "using Lowerthreshold Per Timeframe= " << mThresholdMeanLowPerTimeFrame << ENDM;

  mThresholdMeanHighPerTrigger = getFromConfig<float>(mCustomParameters, "UpperthresholdPerTrigger", 520.f);
  ILOG(Debug, Support) << "using Upperthreshold Per Trigger= " << mThresholdMeanHighPerTrigger << ENDM;

  mThresholdMeanLowPerTrigger = getFromConfig<float>(mCustomParameters, "LowerthresholdPerTrigger", 500.f);
  ILOG(Debug, Support) << "using Lowerthreshold Per Trigger= " << mThresholdMeanLowPerTrigger << ENDM;

  mStatThresholdPerTrigger = getFromConfig<int>(mCustomParameters, "StatThresholdPerTrigger", 1000);
  ILOG(Debug, Support) << "using StatThreshold Per Trigger= " << mStatThresholdPerTrigger << ENDM;
}

Quality TrackletCountCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  mResultPertrigger = Quality::Null;
  mResultPerTimeFrame = Quality::Null;

  for (auto& [moName, mo] : *moMap) {
    ILOG(Debug, Support) << "moName =" << moName << ENDM;
    (void)moName;
    if (mo->getName() == "trackletsperevent") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      if (h == nullptr) {
        // ILOG(Debug, Support) << "Requested Histogram type does not match with the Histogram in source" << ENDM;
        continue;
      }
      // warning about statistics available
      mTrackletPerTriggerMessage.reset();
      if (!mTrackletPerTriggerMessage) {
        mTrackletPerTriggerMessage = std::make_shared<TPaveText>(0.3, 0.7, 0.7, 0.9, "NDC");
      }
      // mTrackletPerTriggerMessage = std::make_shared<TPaveText>(0.3, 0.7, 0.7, 0.9, "NDC");
      // mTrackletPerTriggerMessage->DeleteText();
      mTrackletPerTriggerMessage->SetTextSize(10);
      int entriesInQcHist = h->GetEntries();
      if (entriesInQcHist > mStatThresholdPerTrigger) {
        mTrackletPerTriggerMessage->AddText(TString::Format("Hist Can't be ignored. Stat is enough. entriesInQcHist: %d > Threshold: %d", entriesInQcHist, mStatThresholdPerTrigger));
        // mTrackletPerTriggerMessage->SetTextColor(kGreen);
      } else if (entriesInQcHist > 0) {
        mTrackletPerTriggerMessage->AddText(TString::Format("Hist Can be ignored. Stat is low. entriesInQcHist: %d < Threshold: %d", entriesInQcHist, mStatThresholdPerTrigger));
        // mTrackletPerTriggerMessage->SetTextColor(kYellow);
      } else if (entriesInQcHist == 0) {
        mTrackletPerTriggerMessage->AddText(TString::Format("Hist is empty. entriesInQcHist: %d < Threshold: %d", entriesInQcHist, mStatThresholdPerTrigger));
        mTrackletPerTriggerMessage->SetTextColor(kRed);
      }

      // Warning about triggers without any tracklets
      int underFlowTrackletPerTrigger = h->GetBinContent(0);
      if (underFlowTrackletPerTrigger > 0.) {
        mTrackletPerTriggerMessage->AddText(TString::Format("Number of Triggers without Tracklets: %d", underFlowTrackletPerTrigger));
      }

      // applying check
      float meanTrackletPerTrigger = h->GetMean();
      if (meanTrackletPerTrigger > mThresholdMeanLowPerTrigger && meanTrackletPerTrigger < mThresholdMeanHighPerTrigger) {
        TText* checkMessagePerTriggerPtr = mTrackletPerTriggerMessage->AddText(TString::Format("Mean Per Trigger: %f is found in bound region [%f, %f]", meanTrackletPerTrigger, mThresholdMeanLowPerTrigger, mThresholdMeanHighPerTrigger));
        checkMessagePerTriggerPtr->SetTextColor(kGreen);
        mResultPertrigger = Quality::Good;
      } else {
        mResultPertrigger = Quality::Bad;
        TText* checkMessagePerTriggerPtr = mTrackletPerTriggerMessage->AddText(TString::Format("Mean Per Trigger: %f is not found in bound region [%f, %f]", meanTrackletPerTrigger, mThresholdMeanLowPerTrigger, mThresholdMeanHighPerTrigger));
        checkMessagePerTriggerPtr->SetTextColor(kRed);
        mResultPertrigger.addReason(FlagReasonFactory::Unknown(), "meanTrackletPerTrigger is not in bound region");
      }
      h->GetListOfFunctions()->Add(mTrackletPerTriggerMessage->Clone());
    }
    if (mo->getName() == "trackletspertimeframe") {
      auto* h2 = dynamic_cast<TH1F*>(mo->getObject());
      if (h2 == nullptr) {
        // ILOG(Debug, Support) << "Requested Histogram type does not match with the Histogram in source" << ENDM;
        continue;
      }
      mTrackletPerTimeFrameMessage.reset();
      if (!mTrackletPerTimeFrameMessage) {
        mTrackletPerTimeFrameMessage = std::make_shared<TPaveText>(0.3, 0.7, 0.7, 0.9, "NDC");
      }
      // mTrackletPerTimeFrameMessage = std::make_shared<TPaveText>(0.3, 0.7, 0.7, 0.9, "NDC");
      // mTrackletPerTimeFrameMessage->DeleteText();
      mTrackletPerTimeFrameMessage->SetTextSize(10);

      // Warning about TimeFrame without any tracklets
      int underFlowTrackletPerTimeFrame = h2->GetBinContent(0);
      if (underFlowTrackletPerTimeFrame > 0.) {
        mTrackletPerTimeFrameMessage->AddText(TString::Format("Number of TimeFrames without Tracklets: %d", underFlowTrackletPerTimeFrame));
      }

      // applying check
      float meanTrackletPerTimeframe = h2->GetMean();
      if (meanTrackletPerTimeframe > mThresholdMeanLowPerTimeFrame && meanTrackletPerTimeframe < mThresholdMeanHighPerTimeFrame) {
        TText* checkMessagePerTimeframePtr = mTrackletPerTimeFrameMessage->AddText(TString::Format("Mean Per Timeframe: %f is found in bound region [%f, %f]", meanTrackletPerTimeframe, mThresholdMeanLowPerTimeFrame, mThresholdMeanHighPerTimeFrame));
        checkMessagePerTimeframePtr->SetTextColor(kGreen);
        mResultPerTimeFrame = Quality::Good;
      } else {
        mResultPerTimeFrame = Quality::Bad;
        TText* checkMessagePerTimeframePtr = mTrackletPerTimeFrameMessage->AddText(TString::Format("Mean per Timeframe: %f is not found in bound region[%f, %f]", meanTrackletPerTimeframe, mThresholdMeanLowPerTimeFrame, mThresholdMeanHighPerTimeFrame));
        checkMessagePerTimeframePtr->SetTextColor(kRed);
        mResultPerTimeFrame.addReason(FlagReasonFactory::Unknown(), "meanTrackletPerTimeframe is not in bound region");
      }
      h2->GetListOfFunctions()->Add(mTrackletPerTimeFrameMessage->Clone());
    }
  }
  if (mResultPertrigger == Quality::Null && mResultPerTimeFrame == Quality::Null) {
    mFinalResult = Quality::Null;
    mFinalResult.addReason(FlagReasonFactory::Unknown(), "Quality of both trackletspertimeframe and trackletsperevent is unknown");
  } else if ((mResultPertrigger == Quality::Null && mResultPerTimeFrame == Quality::Good) || (mResultPertrigger == Quality::Good && mResultPerTimeFrame == Quality::Null)) {
    mFinalResult = Quality::Medium;
    mFinalResult.addReason(FlagReasonFactory::Unknown(), "Quality of any of trackletspertimeframe and trackletsperevent is unknown");
  } else if (mResultPertrigger == Quality::Bad || mResultPerTimeFrame == Quality::Bad) {
    mFinalResult = Quality::Bad;
    mFinalResult.addReason(FlagReasonFactory::Unknown(), "Quality of both or any of trackletspertimeframe and trackletsperevent is bad");
  } else {
    mFinalResult = Quality::Good;
  }
  return mFinalResult;
}

std::string TrackletCountCheck::getAcceptedType() { return "TH1"; }

void TrackletCountCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "trackletsperevent") {
    auto* h1 = dynamic_cast<TH1F*>(mo->getObject());
    if (h1 == nullptr) {
      // ILOG(Debug, Support) << "Requested Histogram type does not match with the Histogram in source" << ENDM;
      return;
    }
    if (mResultPertrigger == Quality::Good) {
      h1->SetFillColor(kGreen);
    } else if (mResultPertrigger == Quality::Bad) {
      ILOG(Debug, Devel) << "Quality::Bad, setting to red" << ENDM;
      h1->SetFillColor(kRed);
    } else if (mResultPertrigger == Quality::Medium) {
      ILOG(Debug, Devel) << "Quality::medium, setting to orange" << ENDM;
      h1->SetFillColor(kOrange);
    }
    h1->SetLineColor(kBlack);
  }
  if (mo->getName() == "trackletspertimeframe") {
    auto* h2 = dynamic_cast<TH1F*>(mo->getObject());
    if (h2 == nullptr) {
      // ILOG(Debug, Support) << "Requested Histogram type does not match with the Histogram in source" << ENDM;
      return;
    }
    if (mResultPerTimeFrame == Quality::Good) {
      h2->SetFillColor(kGreen);
    } else if (mResultPerTimeFrame == Quality::Bad) {
      ILOG(Debug, Devel) << "Quality::Bad, setting to red" << ENDM;
      h2->SetFillColor(kRed);
    } else if (mResultPerTimeFrame == Quality::Medium) {
      ILOG(Debug, Devel) << "Quality::medium, setting to orange" << ENDM;
      h2->SetFillColor(kOrange);
    }
    h2->SetLineColor(kBlack);
  }
}

} // namespace o2::quality_control_modules::trd