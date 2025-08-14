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

#include <DataFormatsQualityControl/FlagType.h>
#include <DataFormatsQualityControl/FlagTypeFactory.h>

using namespace std;
using namespace o2::quality_control;
using namespace o2::quality_control_modules::common;

namespace o2::quality_control_modules::trd
{

void TrackletCountCheck::configure()
{
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
        mResultPertrigger.addFlag(FlagTypeFactory::Unknown(), "meanTrackletPerTrigger is not in bound region");
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

      // applying check on mean of TrackletPerTimeFrame
      float meanTrackletPerTimeframe = h2->GetMean();
      if (meanTrackletPerTimeframe > mThresholdMeanLowPerTimeFrame && meanTrackletPerTimeframe < mThresholdMeanHighPerTimeFrame) {
        TText* checkMessagePerTimeframePtr = mTrackletPerTimeFrameMessage->AddText(TString::Format("Mean Per Timeframe: %f is found in bound region [%f, %f]", meanTrackletPerTimeframe, mThresholdMeanLowPerTimeFrame, mThresholdMeanHighPerTimeFrame));
        checkMessagePerTimeframePtr->SetTextColor(kGreen);

        bool isDistributionAccepted = isTrackletDistributionAccepeted(mTrackletPerTimeFrameThreshold, mRatioThreshold, mZeroBinRatioThreshold, h2);
        if (isDistributionAccepted) {
          mResultPerTimeFrame = Quality::Good;
        } else {
          TText* checkMessagePerTimeframePtr2 = mTrackletPerTimeFrameMessage->AddText("TrackletPerTimeFrame distribution in not accepeted as per given config");
          checkMessagePerTimeframePtr2->SetTextColor(kRed);
          mResultPerTimeFrame = Quality::Bad;
          mResultPerTimeFrame.addFlag(FlagTypeFactory::Unknown(), "TrackletPerTimeFrame distribution in not accepet");
        }

      } else {
        mResultPerTimeFrame = Quality::Bad;
        TText* checkMessagePerTimeframePtr = mTrackletPerTimeFrameMessage->AddText(TString::Format("Mean per Timeframe: %f is not found in bound region[%f, %f]", meanTrackletPerTimeframe, mThresholdMeanLowPerTimeFrame, mThresholdMeanHighPerTimeFrame));
        checkMessagePerTimeframePtr->SetTextColor(kRed);
        mResultPerTimeFrame.addFlag(FlagTypeFactory::Unknown(), "meanTrackletPerTimeframe is not in bound region");
      }
      h2->GetListOfFunctions()->Add(mTrackletPerTimeFrameMessage->Clone());
    }
  }
  if (mResultPertrigger == Quality::Null && mResultPerTimeFrame == Quality::Null) {
    mFinalResult = Quality::Null;
    mFinalResult.addFlag(FlagTypeFactory::Unknown(), "Quality of both trackletspertimeframe and trackletsperevent is unknown");
  } else if ((mResultPertrigger == Quality::Null && mResultPerTimeFrame == Quality::Good) || (mResultPertrigger == Quality::Good && mResultPerTimeFrame == Quality::Null)) {
    mFinalResult = Quality::Medium;
    mFinalResult.addFlag(FlagTypeFactory::Unknown(), "Quality of any of trackletspertimeframe and trackletsperevent is unknown");
  } else if (mResultPertrigger == Quality::Bad || mResultPerTimeFrame == Quality::Bad) {
    mFinalResult = Quality::Bad;
    mFinalResult.addFlag(FlagTypeFactory::Unknown(), "Quality of both or any of trackletspertimeframe and trackletsperevent is bad");
  } else {
    mFinalResult = Quality::Good;
  }
  return mFinalResult;
}



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

void TrackletCountCheck::reset()
{
  ILOG(Debug, Devel) << "TrackletCountCheck::reset" << ENDM;
}

void TrackletCountCheck::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "TrackletCountCheck::start : " << activity.mId << ENDM;
  mActivity = make_shared<Activity>(activity);

  ILOG(Debug, Devel) << "initializing TrackletCountCheck" << ENDM;

  mThresholdMeanHighPerTimeFrame = std::stof(mCustomParameters.atOptional("UpperthresholdPerTimeFrame", *mActivity).value_or("6.0e5"));
  ILOG(Debug, Support) << "using Upperthreshold Per Timeframe= " << mThresholdMeanHighPerTimeFrame << ENDM;

  mThresholdMeanLowPerTimeFrame = std::stof(mCustomParameters.atOptional("LowerthresholdPerTimeFrame", *mActivity).value_or("5.0e5"));
  ILOG(Debug, Support) << "using Lowerthreshold Per Timeframe= " << mThresholdMeanLowPerTimeFrame << ENDM;

  mThresholdMeanHighPerTrigger = std::stof(mCustomParameters.atOptional("UpperthresholdPerTrigger", *mActivity).value_or("2.0e4"));
  ILOG(Debug, Support) << "using Upperthreshold Per Trigger= " << mThresholdMeanHighPerTrigger << ENDM;

  mThresholdMeanLowPerTrigger = std::stof(mCustomParameters.atOptional("LowerthresholdPerTrigger", *mActivity).value_or("1.0e4"));
  ILOG(Debug, Support) << "using Lowerthreshold Per Trigger= " << mThresholdMeanLowPerTrigger << ENDM;

  mStatThresholdPerTrigger = std::stof(mCustomParameters.atOptional("StatThresholdPerTrigger", *mActivity).value_or("1.0e3"));
  ILOG(Debug, Support) << "using StatThreshold Per Trigger= " << mStatThresholdPerTrigger << ENDM;

  // to check if TimeFrames with lower tracklets are higher in number wrt TimeFrames with higher tracklets
  // (the boundary is "mTrackletPerTimeFrameThreshold")
  mTrackletPerTimeFrameThreshold = std::stof(mCustomParameters.atOptional("trackletPerTimeFrameThreshold", *mActivity).value_or("5.5e5"));

  // 90% of counts must be above the threshold
  mRatioThreshold = std::stof(mCustomParameters.atOptional("ratiothreshold", *mActivity).value_or("0.9"));

  // 1% of counts can be in this bin
  mZeroBinRatioThreshold = std::stof(mCustomParameters.atOptional("zerobinratiotheshold", *mActivity).value_or("0.1"));
}

void TrackletCountCheck::endOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "TrackletCountCheck::end : " << activity.mId << ENDM;
}

bool TrackletCountCheck::isTrackletDistributionAccepeted(float trackletPerTimeFrameThreshold, float acceptedRatio, float zeroTrackletTfRatio, TH1F* hist)
{
  float ratio = -999.;

  int nBins = hist->GetNbinsX();
  float xMax = hist->GetXaxis()->GetXmax();
  float xMin = hist->GetXaxis()->GetXmin();
  float binWidth = (xMax - xMin) / nBins;

  int trackletPerTimeFrameThresholdBin = int(trackletPerTimeFrameThreshold / binWidth);
  float integralOnleftSide = hist->Integral(0, trackletPerTimeFrameThresholdBin);
  float integralOnRightSide = hist->Integral(trackletPerTimeFrameThresholdBin, hist->GetXaxis()->GetXmax());
  ILOG(Debug, Support) << "integralOnleftSide: " << integralOnleftSide << " integralOnRightSide: " << integralOnRightSide << ENDM;

  if (integralOnleftSide * integralOnRightSide > 0.) {
    ratio = integralOnleftSide / integralOnRightSide;
    ILOG(Debug, Support) << "ratio (TFs with lower tracklets/ TFs with higher tracklets): " << ratio << " Threshold limit: " << acceptedRatio << ENDM;
  }
  bool zeroTrackletRatio = isTimeframeRatioWOTrackletAccepted(zeroTrackletTfRatio, hist);
  if (!(ratio < 0.) && (ratio <= acceptedRatio)) {
    if (zeroTrackletRatio) {
      return true;
    } else {
      ILOG(Warning, Support) << "TFs with lower tracklets are still within threshold but TFs without tracklets are higher" << ENDM;
      return false;
    }
  } else if (!(ratio < 0) && (ratio > acceptedRatio)) {
    ILOG(Warning, Support) << "TFs with lower tracklets are not within threshold " << ENDM;
    return false;
  }
  ILOG(Warning, Support) << "ratio is still not updated, Some region is still empty in TrackletPerTimeFrame object" << ENDM;
  return false;
}

bool TrackletCountCheck::isTimeframeRatioWOTrackletAccepted(float zeroTrackletTfRatio, TH1F* hist)
{
  auto integral = hist->Integral(1, hist->GetXaxis()->GetXmax());
  if (integral <= 0.0) {
    integral = 1;
  }
  auto tFsWOTracklets = hist->GetBinContent(0);
  if ((tFsWOTracklets / integral) <= zeroTrackletTfRatio) {
    return true;
  }
  ILOG(Warning, Support) << "Ratio of No. of TFs without trackets to the total TFs: " << (tFsWOTracklets / integral) << " is higher than the threshold limit: " << zeroTrackletTfRatio << ENDM;
  return false;
}

} // namespace o2::quality_control_modules::trd