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
/// \file   RawDataCheck.cxx
/// \author Ole Schmidt

// C++
#include <string>
#include <fmt/core.h>
// Fair
#include <fairlogger/Logger.h>
// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TLatex.h>
#include <TList.h>
#include <TPaveText.h>
#include <TBox.h>
#include <TPad.h>
#include <TCanvas.h>
#include <TMath.h>

// O2
#include <DataFormatsQualityControl/FlagReasons.h>
#include <CommonConstants/LHCConstants.h>
#include <DataFormatsParameters/GRPECSObject.h>
#include <CCDB/BasicCCDBManager.h>

// Quality Control
#include "TRD/RawDataCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/UserCodeInterface.h"

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::trd
{

void RawDataCheckStats::configure() {}

Quality RawDataCheckStats::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;
  int runNumber = mActivity->mId;
  auto& ccdbmgr = o2::ccdb::BasicCCDBManager::instance();
  auto runDuration = ccdbmgr.getRunDuration(runNumber, false);
  map<string, string> metaData;
  metaData.emplace(std::pair<std::string, std::string>("runNumber", to_string(runNumber)));
  auto calib = UserCodeInterface::retrieveConditionAny<o2::parameters::GRPECSObject>("GLO/Config/GRPECS", metaData, runDuration.first);
  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if (mo->getName() == "stats") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      result.set(Quality::Good);
      if (h->GetBinContent(1) < 1 || h->GetBinContent(2) < 1) {
        result.set(Quality::Bad); // no readout triggers or histogram is completely empty
      } else {
        mReadoutRate = h->GetBinContent(2) / h->GetBinContent(1); // number of triggers per TF
        mReadoutRate *= 1. / (calib->getNHBFPerTF() * o2::constants::lhc::LHCOrbitMUS * 1e-6);
        mCalTriggerRate = h->GetBinContent(3) / h->GetBinContent(2);
      }
      if (mReadoutRate > mMaxReadoutRate) {
        result.set(Quality::Bad); // too high rate (should be triggered only for COSMICS run) TOF noisy?
      }
      if (mCalTriggerRate < mMinCalTriggerRate) {
        result.set(Quality::Medium); // no calibration triggers configured?
      }
      result.addMetadata("readoutRate", to_string(mReadoutRate));
      result.addMetadata("calTriggerRate", to_string(mCalTriggerRate));
    }
  }
  return result;
}

void RawDataCheckStats::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{

  if (mo->getName() == "stats") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    if (h->GetMaximum() < 1) {
      return;
    }
    auto msg = new TPaveText(0.5, h->GetMaximum() / 1e2, 2.5, h->GetMaximum());
    TText* txtPtr = nullptr;
    txtPtr = msg->AddText(Form("Readout rate: %.3f kHz", mReadoutRate / 1000.));
    txtPtr->SetTextAlign(12);
    txtPtr = msg->AddText(Form("Calibration trigger rate: %.4f", mCalTriggerRate));
    txtPtr->SetTextAlign(12);
    static_cast<TText*>(msg->GetListOfLines()->Last())->SetTextAlign(12);
    h->GetListOfFunctions()->Add(msg);
    if (checkResult == Quality::Good) {
      h->SetFillColor(kGreen);
      msg->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      ILOG(Debug, Devel) << "Quality::Bad, setting to red" << ENDM;
      h->SetFillColor(kRed);
      msg->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      ILOG(Debug, Devel) << "Quality::medium, setting to orange" << ENDM;
      h->SetFillColor(kOrange);
      msg->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
}

std::string RawDataCheckStats::getAcceptedType()
{
  return "TH1";
}

void RawDataCheckStats::reset()
{
  ILOG(Debug, Devel) << "RawDataCheckStats::reset" << ENDM;
}

void RawDataCheckStats::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "RawDataCheckStats::start : " << activity.mId << ENDM;
  mActivity = make_shared<Activity>(activity);
  // we cannot move the parameter initialization to configure(), as there the activity is not yet initialized
  mMaxReadoutRate = std::stof(mCustomParameters.atOptional("maxReadoutRate", *mActivity).value_or("1e6"));
  mMinCalTriggerRate = std::stof(mCustomParameters.atOptional("minCalTriggerRate", *mActivity).value_or("1e-5"));
}

void RawDataCheckStats::endOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "RawDataCheckStats::end : " << activity.mId << ENDM;
}

void RawDataCheckSizes::configure() {}

Quality RawDataCheckSizes::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if (mo->getName() == "datavolumepersector") {
      auto* h = dynamic_cast<TH2F*>(mo->getObject());
      result.set(Quality::Good);
      TObjArray fits;
      h->FitSlicesY(nullptr, 1, 18, 0, "QNR", &fits);
      auto hMean = (TH1F*)fits[1];
      double sum = 0, sum2 = 0, n = 0;
      for (int iBin = 1; iBin <= 18; ++iBin) {
        sum += hMean->GetBinContent(iBin);
        sum2 += hMean->GetBinContent(iBin) * hMean->GetBinContent(iBin);
        ++n;
        mMeanDataSize = sum / n;
        mStdDevDataSize = sum2 / n - TMath::Power(sum / n, 2);
      }
      mStdDevDataSize = TMath::Sqrt(mStdDevDataSize);
      for (int iBin = 1; iBin <= 18; ++iBin) {
        if (TMath::Abs(hMean->GetBinContent(iBin) - mMeanDataSize) > mWarningThreshold * mStdDevDataSize) {
          ILOG(Debug, Support) << fmt::format("Found outlier in sector {} with mean value of {}", iBin - 1, hMean->GetBinContent(iBin)) << ENDM;
          result.set(Quality::Medium);
        }
        if (TMath::Abs(hMean->GetBinContent(iBin) - mMeanDataSize) > mErrorThreshold * mStdDevDataSize) {
          ILOG(Debug, Support) << fmt::format("Found strong outlier in sector {} with mean value of {}", iBin - 1, hMean->GetBinContent(iBin)) << ENDM;
          result.set(Quality::Bad);
        }
      }
      ILOG(Debug, Support) << fmt::format("Data size plot: Mean({}), StdDev({}).", mMeanDataSize, mStdDevDataSize) << ENDM;
      result.addMetadata("dataSizeMean", to_string(mMeanDataSize));
      result.addMetadata("dataSizeStdDev", to_string(mStdDevDataSize));
    }
  }
  return result;
}

void RawDataCheckSizes::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "datavolumepersector") {
    ILOG(Debug, Support) << "Found the meta data \"foo\": " << checkResult.getMetadata("foo", "ramtam") << ENDM;
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    auto msg = new TPaveText(4, 3000, 10, 3800);
    msg->AddText(Form("Average volume per sector: %.2f Bytes/TF", mMeanDataSize));
    static_cast<TText*>(msg->GetListOfLines()->Last())->SetTextAlign(12);
    msg->AddText(Form("sigma: %.3f", mStdDevDataSize));
    static_cast<TText*>(msg->GetListOfLines()->Last())->SetTextAlign(12);
    h->GetListOfFunctions()->Add(msg);
    if (checkResult == Quality::Good) {
      msg->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      ILOG(Debug, Devel) << "Quality::Bad, setting to red" << ENDM;
      msg->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      ILOG(Debug, Devel) << "Quality::medium, setting to orange" << ENDM;
      msg->SetFillColor(kOrange);
    }
  }
}

std::string RawDataCheckSizes::getAcceptedType()
{
  return "TH2";
}

void RawDataCheckSizes::reset()
{
  ILOG(Debug, Devel) << "RawDataCheckSizes::reset" << ENDM;
}

void RawDataCheckSizes::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "RawDataCheckSizes::start : " << activity.mId << ENDM;
  mActivity = make_shared<Activity>(activity);
  // we cannot move the parameter initialization to configure(), as there the activity is not yet initialized
  mWarningThreshold = std::stof(mCustomParameters.atOptional("warningThreshold", *mActivity).value_or("3"));
  mErrorThreshold = std::stof(mCustomParameters.atOptional("errorThreshold", *mActivity).value_or("5"));
}

void RawDataCheckSizes::endOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "RawDataCheckSizes::end : " << activity.mId << ENDM;
}

} // namespace o2::quality_control_modules::trd
