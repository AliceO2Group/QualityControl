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
/// \file   PulsePositionCheck.cxx
/// \author Deependra (deependra.sharma@cern.ch)
///

#include "TRD/PulsePositionCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TH1.h>
#include <TF1.h>
#include <TMath.h>
#include <TFitResult.h>

#include <fairlogger/Logger.h>
#include <DataFormatsQualityControl/FlagReasons.h>
#include "CCDB/BasicCCDBManager.h"

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::trd
{

void PulsePositionCheck::configure()
{
  if (auto param = mCustomParameters.find("ccdbtimestamp"); param != mCustomParameters.end()) {
    mTimeStamp = std::stol(mCustomParameters["ccdbtimestamp"]);
    ILOG(Debug, Support) << "configure() : using ccdbtimestamp = " << mTimeStamp << ENDM;
  } else {
    mTimeStamp = o2::ccdb::getCurrentTimestamp();
    ILOG(Debug, Support) << "configure() : using default timestam of now = " << mTimeStamp << ENDM;
  }
  auto& mgr = o2::ccdb::BasicCCDBManager::instance();
  mgr.setTimestamp(mTimeStamp);
  ILOG(Debug, Devel) << "initialize PulseHeight" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  if (auto param = mCustomParameters.find("pulseheightpeaklower"); param != mCustomParameters.end()) {
    mPulseHeightPeakRegion.first = stof(param->second);
    ILOG(Debug, Support) << "configure() : using pulseheightpeaklower " << mPulseHeightPeakRegion.first << ENDM;
  } else {
    mPulseHeightPeakRegion.first = 0.5;
    ILOG(Debug, Support) << "configure() : using default pulseheightpeaklower  = " << mPulseHeightPeakRegion.first << ENDM;
  }
  if (auto param = mCustomParameters.find("pulseheightpeakupper"); param != mCustomParameters.end()) {
    mPulseHeightPeakRegion.second = stof(param->second);
    ILOG(Debug, Support) << "configure() : using pulseheightpeakupper = " << mPulseHeightPeakRegion.second << ENDM;
  } else {
    mPulseHeightPeakRegion.second = 4.0;
    ILOG(Debug, Support) << "configure() : using default pulseheightpeakupper = " << mPulseHeightPeakRegion.second << ENDM;
  }

  // peak region should be well inside the fitting range(0_4)
  // safe Gaurd
  if (mPulseHeightPeakRegion.second > 4.0) {
    mPulseHeightPeakRegion.second = 4.0;
  }

  if (auto param = mCustomParameters.find("Chi2byNDF_threshold"); param != mCustomParameters.end()) {
    chi2byNDF_threshold = stod(param->second);
    ILOG(Debug, Support) << "configure() : using chi2/NDF threshold = " << chi2byNDF_threshold << ENDM;
  } else {
    chi2byNDF_threshold = 0.22;
    ILOG(Debug, Support) << "configure() : using chi2/NDF threshold = " << chi2byNDF_threshold << ENDM;
  }
  // Fit param 0
  if (auto param = mCustomParameters.find("FitParameter0"); param != mCustomParameters.end()) {
    FitParam0 = stod(param->second);
    ILOG(Debug, Support) << "configure() : using FitParameter0= " << FitParam0 << ENDM;
  } else {
    FitParam0 = 100000.0;
    ILOG(Debug, Support) << "configure() : using FitParameter0= " << FitParam0 << ENDM;
  }
  // Fit param 1
  if (auto param = mCustomParameters.find("FitParameter1"); param != mCustomParameters.end()) {
    FitParam1 = stod(param->second);
    ILOG(Debug, Support) << "configure() : using FitParameter1= " << FitParam1 << ENDM;
  } else {
    FitParam1 = 100000.0;
    ILOG(Debug, Support) << "configure() : using FitParameter1= " << FitParam1 << ENDM;
  }
  // Fit param 2
  if (auto param = mCustomParameters.find("FitParameter2"); param != mCustomParameters.end()) {
    FitParam2 = stod(param->second);
    ILOG(Debug, Support) << "configure() : using FitParameter2= " << FitParam2 << ENDM;
  } else {
    FitParam2 = 1.48;
    ILOG(Debug, Support) << "configure() : using FitParameter2= " << FitParam2 << ENDM;
  }
  // Fit param 3
  if (auto param = mCustomParameters.find("FitParameter3"); param != mCustomParameters.end()) {
    FitParam3 = stod(param->second);
    ILOG(Debug, Support) << "configure() : using FitParameter3= " << FitParam3 << ENDM;
  } else {
    FitParam3 = 1.09;
    ILOG(Debug, Support) << "configure() : using FitParameter3= " << FitParam3 << ENDM;
  }
  // Defined function range
  if (auto param = mCustomParameters.find("DefinedFunctionRangeL"); param != mCustomParameters.end()) {
    FunctionRange[0] = stod(param->second);
  } else {
    FunctionRange[0] = 0.0;
  }
  if (auto param = mCustomParameters.find("DefinedFunctionRangeU"); param != mCustomParameters.end()) {
    FunctionRange[1] = stod(param->second);
  } else {
    FunctionRange[1] = 6.0;
  }
  ILOG(Debug, Support) << "configure() : using defined function range = " << FunctionRange[0] << " to " << FunctionRange[1] << ENDM;

  // Fiiting range lower value
  if (auto param = mCustomParameters.find("FitRangeL"); param != mCustomParameters.end()) {
    FitRange[0] = stod(param->second);
  } else {
    FitRange[0] = 0.0;
  }
  // Fiiting range upper value
  if (auto param = mCustomParameters.find("FitRangeU"); param != mCustomParameters.end()) {
    FitRange[1] = stod(param->second);
  } else {
    FitRange[1] = 4.0;
  }
  ILOG(Debug, Support) << "config() : using fit range= " << FitRange[0] << " to " << FitRange[1] << ENDM;
}

Quality PulsePositionCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  // you can get details about the activity via the object mActivity:
  ILOG(Debug, Trace) << "Run type: " << getActivity()->mType << ENDM;

  // ILOG(Debug, Trace) << "Check function called"<< ENDM;
  //  LOG(info)<<"Check function called";

  for (auto& [moName, mo] : *moMap) {

    (void)moName;
    if (mo->getName() == "mPulseHeight") {

      // LOG(info) << "mPulseHeight object found";

      auto* h = dynamic_cast<TH1F*>(mo->getObject());

      // Defining Fit function
      TF1* f1 = new TF1("landaufit", "((x<2) ? ROOT::Math::erf(x)*[0]:[0]) + [1]*TMath::Landau(x,[2],[3])", FunctionRange[0], FunctionRange[1]);
      f1->SetParameters(FitParam0, FitParam1, FitParam2, FitParam3);

      // LOGF(info,"p0: %f, p1: %f, p2: %f, p3: %f",FitParam0, FitParam1, FitParam2, FitParam3);
      // LOGF(info,"FitRang[0]: %f, FitRange[1]: %f",FitRange[0],FitRange[1]);
      // LOGF(info,"FunctionRange[0]: %f, FunctionRange[1]: %f",FunctionRange[0],FunctionRange[1]);

      // Fitting Pulse Distribution with defined fit function
      auto FitResult = h->Fit(f1, "S", "", FitRange[0], FitRange[1]);
      // LOGF(info,"Status: %d, Cov Matrix Status: %d",FitResult->Status(),FitResult->CovMatrixStatus());

      if (FitResult->CovMatrixStatus() != 3) // if covMatrix is not accurate
      {
        result = Quality::Bad;
        result.addReason(FlagReasonFactory::Unknown(), "Covariance matrix is not accurate.");
        return result;
      }

      double_t peak_value_x = f1->GetMaximumX(mPulseHeightPeakRegion.first, mPulseHeightPeakRegion.second);

      // LOGF(info,"peak_value_x %f",peak_value_x);

      double_t chi2_value = f1->GetChisquare();
      Int_t NDF = f1->GetNDF();
      double_t Chi2byNDF = chi2_value / NDF;

      // LOGF(info,"chi2/ndf %f",Chi2byNDF);

      if (Chi2byNDF > chi2byNDF_threshold) {
        result = Quality::Bad;
        result.addReason(FlagReasonFactory::Unknown(), "chi2/ndf is very large");
        return result;
      }

      auto x0 = h->GetXaxis()->GetBinLowEdge(0);
      auto x1 = h->GetXaxis()->GetBinLowEdge(1);
      auto x2 = h->GetXaxis()->GetBinLowEdge(2);
      auto x3 = h->GetXaxis()->GetBinLowEdge(3);

      if ((peak_value_x < x1) && (peak_value_x > x0)) {
        result = Quality::Bad;
        result.addReason(FlagReasonFactory::Unknown(), "pulse peak is in first bin");
        // LOG(info)<<"peak is in the first bin from left";
        return result;
      } else if ((peak_value_x < x2) && (peak_value_x > x1)) {
        result = Quality::Good;
        // LOG(info)<<"peak is in the second bin from left";
      } else if ((peak_value_x < x3) && (peak_value_x > x2)) {
        result = Quality::Good;
        // LOG(info)<<"peak is in the third bin from left";
      } else {
        // LOG(info)<<"peak is not in Good region";
        result = Quality::Bad;
        result.addReason(FlagReasonFactory::Unknown(), "amplification peak is not in good position. Out of [" + std::to_string(x0) + " " + std::to_string(x3) + "] range");
        return result;
      }

      result.addMetadata("Peak_in", "third_Bin");
    } else {
      // LOG(info) << "PulseHeight object not found";
    }
  }
  return result;
}

std::string PulsePositionCheck::getAcceptedType() { return "TH1"; }

void PulsePositionCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{

  //  ILOG(Debug, Trace) << "beautify function called"<< ENDM;
  // LOG(info)<< "beautify function called";

  if (mo->getName() == "mPulseHeight") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());

    if (checkResult == Quality::Good) {
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      ILOG(Debug, Devel) << "Quality::Bad, setting to red" << ENDM;
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      ILOG(Debug, Devel) << "Quality::medium, setting to orange" << ENDM;
      h->SetFillColor(kOrange);
    } else if (checkResult == Quality::Null) {
      ILOG(Debug, Devel) << "Quality::Null, setting to Blue" << ENDM;
      h->SetFillColor(kBlue);
    } else if (checkResult == Quality::NullLevel) {
      ILOG(Debug, Devel) << "Quality::Null, setting to Pink" << ENDM;
      h->SetFillColor(kPink);
    }
    h->SetLineColor(kBlack);
    h->Draw();
  }
}

} // namespace o2::quality_control_modules::trd
