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
      msg1 = std::make_shared<TPaveText>(0.3, 0.7, 0.7, 0.9, "NDC");
      msg1->SetTextSize(10);
      int Entries = h->GetEntries();
      if (Entries > mStatThresholdPerTrigger) {
        msg1->AddText(TString::Format("Hist Can't be ignored. Stat is enough. Entries: %d > Threshold: %d", Entries, mStatThresholdPerTrigger));
        // msg1->SetTextColor(kGreen);
      } else if (Entries > 0) {
        msg1->AddText(TString::Format("Hist Can be ignored. Stat is low. Entries: %d < Threshold: %d", Entries, mStatThresholdPerTrigger));
        // msg1->SetTextColor(kYellow);
      } else if (Entries == 0) {
        msg1->AddText(TString::Format("Hist is empty. Entries: %d < Threshold: %d", Entries, mStatThresholdPerTrigger));
        msg1->SetTextColor(kRed);
      }

      // Warning about triggers without any tracklets
      int UnderFlowTrackletsPerTrigger = h->GetBinContent(0);
      if (UnderFlowTrackletsPerTrigger > 0.) {
        msg1->AddText(TString::Format("Number of Triggers without Tracklets: %d", UnderFlowTrackletsPerTrigger));
      }

      // applying check
      float MeanTrackletPertrigger = h->GetMean();
      if (MeanTrackletPertrigger > mThresholdMeanLowPerTrigger && MeanTrackletPertrigger < mThresholdMeanHighPerTrigger) {
        TText* Checkmsg = msg1->AddText(TString::Format("Mean Per Trigger: %f is found in bound region [%f, %f]", MeanTrackletPertrigger, mThresholdMeanLowPerTrigger, mThresholdMeanHighPerTrigger));
        Checkmsg->SetTextColor(kGreen);
        mResultPertrigger = Quality::Good;
      } else {
        mResultPertrigger = Quality::Bad;
        TText* Checkmsg = msg1->AddText(TString::Format("Mean Per Trigger: %f is not found in bound region [%f, %f]", MeanTrackletPertrigger, mThresholdMeanLowPerTrigger, mThresholdMeanHighPerTrigger));
        Checkmsg->SetTextColor(kRed);
        mResultPertrigger.addReason(FlagReasonFactory::Unknown(), "MeanTrackletPertrigger is not in bound region");
      }
      h->GetListOfFunctions()->Add(msg1->Clone());
    }
    if (mo->getName() == "trackletspertimeframe") {
      auto* h2 = dynamic_cast<TH1F*>(mo->getObject());
      if (h2 == nullptr) {
        // ILOG(Debug, Support) << "Requested Histogram type does not match with the Histogram in source" << ENDM;
        continue;
      }
      msg2 = std::make_shared<TPaveText>(0.3, 0.7, 0.7, 0.9, "NDC");
      msg2->SetTextSize(10);

      // Warning about TimeFrame without any tracklets
      int UnderFlowTrackletsPertrigger = h2->GetBinContent(0);
      if (UnderFlowTrackletsPertrigger > 0.) {
        msg2->AddText(TString::Format("Number of TimeFrames without Tracklets: %d", UnderFlowTrackletsPertrigger));
      }

      // applying check
      float MeanTrackletPerTimeFrame = h2->GetMean();
      if (MeanTrackletPerTimeFrame > mThresholdMeanLowPerTimeFrame && MeanTrackletPerTimeFrame < mThresholdMeanHighPerTimeFrame) {
        TText* Checkmsg2 = msg2->AddText(TString::Format("Mean Per Timeframe: %f is found in bound region [%f, %f]", MeanTrackletPerTimeFrame, mThresholdMeanLowPerTimeFrame, mThresholdMeanHighPerTimeFrame));
        Checkmsg2->SetTextColor(kGreen);
        mResultPerTimeFrame = Quality::Good;
      } else {
        mResultPerTimeFrame = Quality::Bad;
        TText* Checkmsg2 = msg2->AddText(TString::Format("Mean per Timeframe: %f is not found in bound region[%f, %f]", MeanTrackletPerTimeFrame, mThresholdMeanLowPerTimeFrame, mThresholdMeanHighPerTimeFrame));
        Checkmsg2->SetTextColor(kRed);
        mResultPerTimeFrame.addReason(FlagReasonFactory::Unknown(), "MeanTrackletPerTimeFrame is not in bound region");
      }
      h2->GetListOfFunctions()->Add(msg2->Clone());
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