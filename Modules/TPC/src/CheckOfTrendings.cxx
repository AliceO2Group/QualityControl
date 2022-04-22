// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   CheckOfTrendings.cxx
/// \author Laura Serksnyte
///

#include "TPC/CheckOfTrendings.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"

#include <fairlogger/Logger.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TList.h>
#include <TPaveText.h>

#include <vector>
#include <numeric>

namespace o2::quality_control_modules::tpc
{

void CheckOfTrendings::configure()
{
  if (const auto param = mCustomParameters.find("chooseCheckMeanOrExpectedPhysicsValueOrBoth"); param != mCustomParameters.end()) {
    mCheckChoice = param->second.c_str();
    if (mCheckChoice != CheckChoiceMean && mCheckChoice != CheckChoiceExpectedPhysicsValue && mCheckChoice != CheckChoiceBoth) {
      mCheckChoice = CheckChoiceMean;
      ILOG(Fatal, Support) << "The chosen value does not exist. Available options: mean, ExpectedPhysicsValue, both. Default mean selected." << ENDM;
    }
    if (mCheckChoice == CheckChoiceExpectedPhysicsValue || mCheckChoice == CheckChoiceBoth) {
      if (const auto param = mCustomParameters.find("expectedPhysicsValue"); param != mCustomParameters.end()) {
        mExpectedPhysicsValue = std::atof(param->second.c_str());
      } else {
        // This is not properly working, does not shut down process
        ILOG(Fatal, Support) << "Chosen check requires ExpectedPhysicsValue which is not given." << ENDM;
      }
      if (const auto param = mCustomParameters.find("allowedNSigmaForExpectation"); param != mCustomParameters.end()) {
        mNSigmaExpectedPhysicsValue = std::atof(param->second.c_str());
      } else {
        mNSigmaExpectedPhysicsValue = 3;
        ILOG(Warning, Support) << "Chosen check requires mNSigmaExpectedPhysicsValue which is not given. Setting to default 3." << ENDM;
      }
      if (const auto param = mCustomParameters.find("badNSigmaForExpectation"); param != mCustomParameters.end()) {
        mNSigmaBadExpectedPhysicsValue = std::atof(param->second.c_str());
      } else {
        mNSigmaBadExpectedPhysicsValue = 6;
        ILOG(Warning, Support) << "Chosen check requires mNSigmaBadExpectedPhysicsValue which is not given. Setting to default 6." << ENDM;
      }
      if (const auto param = mCustomParameters.find("pointsToTakeForExpectedValueCheck"); param != mCustomParameters.end()) {
        mPointToTakeForExpectedValueCheck = std::atof(param->second.c_str());
      } else {
        mPointToTakeForExpectedValueCheck = 10;
        ILOG(Warning, Support) << "Chosen check requires mPointToTakeForExpectedValueCheck which is not given. Set to default 10." << ENDM;
      }
    }

    if (mCheckChoice == CheckChoiceMean || mCheckChoice == CheckChoiceBoth) {
      if (const auto param = mCustomParameters.find("allowedNSigmaForMean"); param != mCustomParameters.end()) {
        mNSigmaMean = std::atof(param->second.c_str());
      } else {
        mNSigmaMean = 3;
        ILOG(Warning, Support) << "Chosen check requires allowedNSigmaForMean which is not given. Setting to default 3." << ENDM;
      }
      if (const auto param = mCustomParameters.find("badNSigmaForMean"); param != mCustomParameters.end()) {
        mNSigmaBadMean = std::atof(param->second.c_str());
      } else {
        mNSigmaBadMean = 6;
        ILOG(Warning, Support) << "Chosen check requires badNSigmaForMean which is not given. Setting to default 6." << ENDM;
      }
      if (const auto param = mCustomParameters.find("pointsToTakeForMeanCheck"); param != mCustomParameters.end()) {
        mPointToTakeForMeanCheck = std::atof(param->second.c_str());
      } else {
        mPointToTakeForMeanCheck = 10;
        ILOG(Warning, Support) << "Chosen check requires mPointToTakeForMeanCheck which is not given. Set to default 10." << ENDM;
      }
    }
  }
}

Quality CheckOfTrendings::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;
  Quality resultMean = Quality::Null;
  Quality resultExpectedPhysicsValue = Quality::Null;

  auto mo = moMap->begin()->second;
  auto* canv = (TCanvas*)mo->getObject();
  TGraph* g = (TGraph*)canv->GetListOfPrimitives()->FindObject("Graph");
  const int NBins = g->GetN();

  // If only one data point available, don't check quality for mean
  if (NBins > 1) {
    if (mCheckChoice == CheckChoiceMean || mCheckChoice == CheckChoiceBoth) {
      double x_last = 0., y_last = 0.;
      g->GetPoint(NBins - 1, x_last, y_last);
      int pointNumberForMean = mPointToTakeForMeanCheck;
      if ((NBins - 1) < mPointToTakeForMeanCheck) {
        pointNumberForMean = NBins - 1;
      }
      const double* yValues = g->GetY();
      const std::vector<double> v(yValues + NBins - 1 - pointNumberForMean, yValues + NBins - 1);

      const double sum = std::accumulate(v.begin(), v.end(), 0.0);
      const double mean = sum / v.size();

      std::vector<double> diff(v.size());
      std::transform(v.begin(), v.end(), diff.begin(), [mean](double x) { return x - mean; });
      const double sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
      const double stdev = std::sqrt(sq_sum / v.size());

      if (std::abs(y_last - mean) <= stdev * mNSigmaMean) {
        resultMean = Quality::Good;
      } else if (std::abs(y_last - mean) > stdev * mNSigmaBadMean) {
        resultMean = Quality::Bad;
      } else {
        resultMean = Quality::Medium;
      }
      result = resultMean;
    }
  }
  if (mCheckChoice == CheckChoiceExpectedPhysicsValue || mCheckChoice == CheckChoiceBoth) {
    int pointNumber = mPointToTakeForExpectedValueCheck;
    if (NBins < mPointToTakeForExpectedValueCheck) {
      pointNumber = NBins;
    }

    const double* yValues = g->GetY();
    const std::vector<double> v(yValues + NBins - pointNumber, yValues + NBins);

    const double sum = std::accumulate(v.begin(), v.end(), 0.0);
    const double meanFull = sum / v.size();

    std::vector<double> diff(v.size());
    std::transform(v.begin(), v.end(), diff.begin(), [meanFull](double x) { return x - meanFull; });
    const double sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
    const double stdevFull = std::sqrt(sq_sum / v.size());

    if (std::abs(meanFull - mExpectedPhysicsValue) > mNSigmaBadExpectedPhysicsValue * stdevFull) {
      resultExpectedPhysicsValue = Quality::Bad;
    } else if (std::abs(meanFull - mExpectedPhysicsValue) < mNSigmaExpectedPhysicsValue * stdevFull) {
      resultExpectedPhysicsValue = Quality::Good;
    } else {
      resultExpectedPhysicsValue = Quality::Medium;
    }
    result = resultExpectedPhysicsValue;
  }
  if (mCheckChoice == CheckChoiceBoth) {
    // If mean check is not performed, the total combined quality mean && expectedValue should be set to expectedValue result
    if (resultMean != Quality::Null) {
      if (resultMean.isWorseThan(resultExpectedPhysicsValue)) {
        result = resultMean;
      }
    }
  }
  return result;
}

std::string CheckOfTrendings::getAcceptedType() { return "TCanvas"; }

void CheckOfTrendings::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  auto* c1 = (TCanvas*)mo->getObject();
  TGraph* h = (TGraph*)c1->GetListOfPrimitives()->FindObject("Graph");
  TPaveText* msg = new TPaveText(0.7, 0.85, 0.9, 0.9, "NDC");
  h->GetListOfFunctions()->Add(msg);
  msg->SetName(Form("%s_msg", mo->GetName()));

  if (checkResult == Quality::Good) {
    h->SetFillColor(kGreen);
    msg->Clear();
    msg->AddText("Quality::Good");
    msg->SetFillColor(kGreen);
  } else if (checkResult == Quality::Bad) {
    LOG(info) << "Quality::Bad, setting to red";
    h->SetFillColor(kRed);
    msg->Clear();
    msg->AddText("Quality::Bad");
    msg->AddText("Outlier, more than 6sigma.");
    msg->SetFillColor(kRed);
  } else if (checkResult == Quality::Medium) {
    LOG(info) << "Quality::medium, setting to orange";
    h->SetFillColor(kOrange);
    msg->Clear();
    msg->AddText("Quality::Medium");
    msg->AddText("Outlier, more than 3sigma.");
    msg->SetFillColor(kOrange);
  } else if (checkResult == Quality::Null) {
    h->SetFillColor(0);
  }
  h->SetLineColor(kBlack);
}

} // namespace o2::quality_control_modules::tpc