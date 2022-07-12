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
/// \file   PulseHeightCheck.cxx
/// \author My Name
///

#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TH1.h>
#include <TList.h>
#include <TLine.h>
#include <TMath.h>
#include <TPaveText.h>

#include <DataFormatsQualityControl/FlagReasons.h>

#include "CCDB/BasicCCDBManager.h"
#include "TRDQC/StatusHelper.h"
#include "TRD/PulseHeightCheck.h"

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::trd
{

void PulseHeightCheck::configure(string /*name*/)
{
  if (auto param = mCustomParameters.find("ccdbtimestamp"); param != mCustomParameters.end()) {
    mTimeStamp = std::stol(mCustomParameters["ccdbtimestamp"]);
    ILOG(Info, Support) << "configure() : using ccdbtimestamp = " << mTimeStamp << ENDM;
  } else {
    mTimeStamp = o2::ccdb::getCurrentTimestamp();
    ILOG(Info, Support) << "configure() : using default timestam of now = " << mTimeStamp << ENDM;
  }
  auto& mgr = o2::ccdb::BasicCCDBManager::instance();
  mgr.setTimestamp(mTimeStamp);
  ILOG(Info, Support) << "initialize PulseHeight" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  if (auto param = mCustomParameters.find("driftregionstart"); param != mCustomParameters.end()) {
    mDriftRegion.first = stof(param->second);
    ILOG(Info, Support) << "configure() : using driftregionstart = " << mDriftRegion.first << ENDM;
  } else {
    mDriftRegion.first = 7.0;
    ILOG(Info, Support) << "configure() : using default dritfregionstart = " << mDriftRegion.first << ENDM;
  }
  if (auto param = mCustomParameters.find("driftregionend"); param != mCustomParameters.end()) {
    mDriftRegion.second = stof(param->second);
    ILOG(Info, Support) << "configure() : using dritftregionend = " << mDriftRegion.second << ENDM;
  } else {
    mDriftRegion.second = 20.0;
    ILOG(Info, Support) << "configure() : using default dritfregionend = " << mDriftRegion.second << ENDM;
  }
  if (auto param = mCustomParameters.find("peakregionstart"); param != mCustomParameters.end()) {
    mPulseHeightPeakRegion.first = stof(param->second);
    ILOG(Info, Support) << "configure() : using peakregionstart " << mPulseHeightPeakRegion.first << ENDM;
  } else {
    mPulseHeightPeakRegion.first = 1.0;
    ILOG(Info, Support) << "configure() : using default peakregionstart  = " << mPulseHeightPeakRegion.first << ENDM;
  }
  if (auto param = mCustomParameters.find("peakregionend"); param != mCustomParameters.end()) {
    mPulseHeightPeakRegion.second = stof(param->second);
    ILOG(Info, Support) << "configure() : using peak region ends = " << mPulseHeightPeakRegion.second << ENDM;
  } else {
    mPulseHeightPeakRegion.second = 5.0;
    ILOG(Info, Support) << "configure() : using default peak region end = " << mPulseHeightPeakRegion.second << ENDM;
  }
}

Quality PulseHeightCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;
    if (mo->getName() == "pulseheightscaled") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());

      result = Quality::Good;

      for (int i = 2; i < 6; ++i) {
        if (i > 0 && i < 8 && h->GetBinContent(i) < 50 && h->GetSum() > 100) {
          result = Quality::Bad;
          result.addReason(FlagReasonFactory::Unknown(),
                           "Peak missing " + std::to_string(i));
          break;
        } else if (i > 0 && i < 8 && h->GetBinContent(i) < 100 && h->GetSum() > 100) {
          result = Quality::Medium;
          result.addReason(FlagReasonFactory::Unknown(),
                           "Peak rather low " + std::to_string(i) + " is not empty");
          result.addReason(FlagReasonFactory::BadTracking(),
                           "This is to demonstrate that we can assign more than one Reason to a Quality");
        }
      }
    }
  }
  return result;
}

std::string PulseHeightCheck::getAcceptedType() { return "TH1"; }

void PulseHeightCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "PulseHeight/mPulseHeight") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.3, 0.9, 0.7, 0.95, "NDC");
    h->GetListOfFunctions()->Add(msg);
    //std::string message = fmt::format("Pulseheight message");
    std::string message = "Pulseheight message";
    msg->SetName(message.c_str());

    if (checkResult == Quality::Good) {
      h->SetFillColor(kGreen);
      h->SetLineColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      ILOG(Info, Support) << "Quality::Bad, something wrong with the pulseheight spectrum" << ENDM;
      h->SetFillColor(kRed);
      h->SetLineColor(kRed);
    } else if (checkResult == Quality::Medium) {
      ILOG(Info, Support) << "Quality::medium, pusleheight spectrum is a bit suspect" << ENDM;
      h->SetFillColor(kOrange);
      h->SetLineColor(kOrange);
    }
    //h->SetLineColor(kBlack);
    h->Draw();
  }
}

} // namespace o2::quality_control_modules::trd
