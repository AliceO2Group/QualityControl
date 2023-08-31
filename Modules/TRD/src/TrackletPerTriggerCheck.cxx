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
/// \file   TrackletPerTriggerCheck.cxx
/// \author Deependra Sharma
///

#include "TRD/TrackletPerTriggerCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TH1.h>
#include <TString.h>
#include <TPaveText.h>

#include <DataFormatsQualityControl/FlagReasons.h>
#include <CCDB/BasicCCDBManager.h>

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::trd
{

void TrackletPerTriggerCheck::configure()
{
  // ccdb setting
  if (auto param = mCustomParameters.find("ccdbtimestamp"); param != mCustomParameters.end()) {
    mTimeStamp = std::stol(mCustomParameters["ccdbtimestamp"]);
    ILOG(Debug, Support) << "configure() : using ccdbtimestamp = " << mTimeStamp << ENDM;
  } else {
    mTimeStamp = o2::ccdb::getCurrentTimestamp();
    ILOG(Debug, Support) << "configure() : using default timestam of now = " << mTimeStamp << ENDM;
  }
  auto& mgr = o2::ccdb::BasicCCDBManager::instance();
  mgr.setTimestamp(mTimeStamp);

  ILOG(Debug, Devel) << "initialize TrackletPertriggerCheck" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  if (auto param = mCustomParameters.find("Lowerthreshold"); param != mCustomParameters.end()) {
    mDesiredMeanRegion.first = stof(param->second);
    ILOG(Debug, Support) << "configure() : using json Lowerthreshold" << mDesiredMeanRegion.first << ENDM;
  } else {
    mDesiredMeanRegion.first = 500.0;
    ILOG(Debug, Support) << "configure() : using default Lowerthreshold" << mDesiredMeanRegion.first << ENDM;
  }

  if (auto param = mCustomParameters.find("Upperthreshold"); param != mCustomParameters.end()) {
    mDesiredMeanRegion.second = stof(param->second);
    ILOG(Debug, Support) << "configure() : using json Upperthreshold" << mDesiredMeanRegion.second << ENDM;
  } else {
    mDesiredMeanRegion.second = 520.0;
    ILOG(Debug, Support) << "configure() : using default Upperthreshold" << mDesiredMeanRegion.second << ENDM;
  }

  if (auto param = mCustomParameters.find("StatThreshold"); param != mCustomParameters.end()) {
    mStatThreshold = stod(param->second);
    ILOG(Debug, Support) << "configure() : using json mStatThreshold" << mStatThreshold << ENDM;
  } else {
    mStatThreshold = 1000;
    ILOG(Debug, Support) << "configure() : using default mStatThreshold" << mStatThreshold << ENDM;
  }
}

Quality TrackletPerTriggerCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if (mo->getName() == "trackletsperevent") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      if (h == nullptr) {
        // ILOG(Debug, Support) << "Requested Histogram type does not match with the Histogram in source" << ENDM;
        continue;
      }
      // writing text over hist
      TPaveText* msg1 = new TPaveText(0.3, 0.7, 0.7, 0.9, "NDC"); // check option "br","NDC"
      h->GetListOfFunctions()->Add(msg1);
      msg1->SetTextSize(10);
      int Entries = h->GetEntries();
      if (Entries > mStatThreshold) {
        msg1->AddText(TString::Format("Hist Can't be ignored. Stat is enough. Entries: %d > Threshold: %ld", Entries, mStatThreshold));
        // msg1->SetTextColor(kGreen);
      } else if (Entries > 0) {
        msg1->AddText(TString::Format("Hist Can be ignored. Stat is low. Entries: %d < Threshold: %ld", Entries, mStatThreshold));
        // msg1->SetTextColor(kYellow);
      } else if (Entries == 0) {
        msg1->AddText(TString::Format("Hist is empty. Entries: %d < Threshold: %ld", Entries, mStatThreshold));
        msg1->SetTextColor(kRed);
      }

      // applying check
      float MeanTracletPertrigger = h->GetMean();
      if (MeanTracletPertrigger > mDesiredMeanRegion.second && MeanTracletPertrigger < mDesiredMeanRegion.first) {
        result = Quality::Good;
      } else {
        result = Quality::Bad;
        result.addReason(FlagReasonFactory::Unknown(), "MeanTracletPertrigger is not in bound region");
      }
    }
  }
  return result;
}

std::string TrackletPerTriggerCheck::getAcceptedType() { return "TH1"; }

void TrackletPerTriggerCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "trackletsperevent") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    if (h == nullptr) {
      // ILOG(Debug, Support) << "Requested Histogram type does not match with the Histogram in source" << ENDM;
      return;
    }
    if (checkResult == Quality::Good) {
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      ILOG(Debug, Devel) << "Quality::Bad, setting to red" << ENDM;
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      ILOG(Debug, Devel) << "Quality::medium, setting to orange" << ENDM;
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
}

} // namespace o2::quality_control_modules::trd