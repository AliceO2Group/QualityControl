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
/// \file   CheckOfSlices.cxx
/// \author Maximilian Horst
///

#include "TPC/CheckOfSlices.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"

#include <fairlogger/Logger.h>
#include <TCanvas.h>
#include <TGraphErrors.h>
#include <TList.h>
#include <TPaveText.h>
#include <TAxis.h>
#include <TLine.h>

#include <vector>
#include <numeric>

namespace o2::quality_control_modules::tpc
{

// ILOG(Warning,Support) << "##########################################################################################Start Check:" << ENDM;
void CheckOfSlices::configure()
{
  ILOG(Warning, Support) << "##########################################################################################Load Parameters:" << ENDM;
  if (auto param = mCustomParameters.find("chooseCheckMeanOrExpectedPhysicsValueOrBoth"); param != mCustomParameters.end()) {
    mCheckChoice = param->second.c_str();
    if (mCheckChoice != mCheckChoiceMean && mCheckChoice != mCheckChoiceExpectedPhysicsValue && mCheckChoice != mCheckChoiceBoth) {
      mCheckChoice = mCheckChoiceMean;
      ILOG(Fatal, Support) << "The chosen value does not exist. Available options: mean, ExpectedPhysicsValue, both. Default mean selected." << ENDM;
    }
    if (mCheckChoice == mCheckChoiceExpectedPhysicsValue || mCheckChoice == mCheckChoiceBoth) {
      if (auto param = mCustomParameters.find("expectedPhysicsValue"); param != mCustomParameters.end()) {
        mExpectedPhysicsValue = std::atof(param->second.c_str());
      } else {
        // This is not properly working, does not shut down process
        ILOG(Fatal, Support) << "Chosen check requires ExpectedPhysicsValue which is not given." << ENDM;
      }
      if (auto param = mCustomParameters.find("allowedNSigmaForExpectation"); param != mCustomParameters.end()) {
        mNSigmaExpectedPhysicsValue = std::atof(param->second.c_str());
      } else {
        mNSigmaExpectedPhysicsValue = 3;
        ILOG(Warning, Support) << "Chosen check requires mNSigmaExpectedPhysicsValue which is not given. Setting to default 3." << ENDM;
      }
      if (auto param = mCustomParameters.find("badNSigmaForExpectation"); param != mCustomParameters.end()) {
        mNSigmaBadExpectedPhysicsValue = std::atof(param->second.c_str());
      } else {
        mNSigmaBadExpectedPhysicsValue = 6;
        ILOG(Warning, Support) << "Chosen check requires mNSigmaBadExpectedPhysicsValue which is not given. Setting to default 6." << ENDM;
      }
      /*
      if (auto param = mCustomParameters.find("pointsToTakeForExpectedValueCheck"); param != mCustomParameters.end()) {
        mPointToTakeForExpectedValueCheck = std::atof(param->second.c_str());
      } else {
        mPointToTakeForExpectedValueCheck = 10;
        ILOG(Warning, Support) << "Chosen check requires mPointToTakeForExpectedValueCheck which is not given. Set to default 10." << ENDM;
      }*/
    }

    if (mCheckChoice == mCheckChoiceMean || mCheckChoice == mCheckChoiceBoth) {
      if (auto param = mCustomParameters.find("allowedNSigmaForMean"); param != mCustomParameters.end()) {
        mNSigmaMean = std::atof(param->second.c_str());
      } else {
        mNSigmaMean = 3;
        ILOG(Warning, Support) << "Chosen check requires allowedNSigmaForMean which is not given. Setting to default 3." << ENDM;
      }
      if (auto param = mCustomParameters.find("badNSigmaForMean"); param != mCustomParameters.end()) {
        mNSigmaBadMean = std::atof(param->second.c_str());
      } else {
        mNSigmaBadMean = 6;
        ILOG(Warning, Support) << "Chosen check requires badNSigmaForMean which is not given. Setting to default 6." << ENDM;
      }
      /* We dont need number of Points
      if (auto param = mCustomParameters.find("pointsToTakeForMeanCheck"); param != mCustomParameters.end()) {
        mPointToTakeForMeanCheck = std::atof(param->second.c_str());
      } else {
        mPointToTakeForMeanCheck = 10;
        ILOG(Warning, Support) << "Chosen check requires mPointToTakeForMeanCheck which is not given. Set to default 10." << ENDM;
      }
      */
    }
  }
  ILOG(Warning, Support) << "Loaded all parameters from JSON:" << ENDM;
}

Quality CheckOfSlices::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;
  Quality resultMean = Quality::Null;
  Quality resultExpectedPhysicsValue = Quality::Null;

  ILOG(Warning, Support) << "Qualities initiallized" << ENDM;
  auto mo = moMap->begin()->second;
  if (!mo) {
    ILOG(Fatal, Support) << "It not found mo" << ENDM;
  }
  auto* canv = (TCanvas*)mo->getObject();
  TGraphErrors* g = (TGraphErrors*)canv->GetListOfPrimitives()->FindObject("Graph");
  if (!g) {
    ILOG(Fatal, Support) << "It not found Graph" << ENDM;
  }
  const int NBins = g->GetN();

  ILOG(Warning, Support) << Form("we have a Graph with %i bins", NBins) << ENDM;
  // If only one data point available, don't check quality for mean --  not really necessary in slices. Maybe add an expected number of Slices?
  // if (NBins > 1) {
  const double* yValues = g->GetY();
  const double* yErrors = g->GetEY();
  const std::vector<double> v(yValues, yValues + NBins);    // use all points
  const std::vector<double> vErr(yErrors, yErrors + NBins); // use all points

  // now we have the mean and the stddev. Now check if all points are within the margins (default 3,6 sigma margins)

  ILOG(Warning, Support) << "We check by CheckChoice:";
  ILOG(Warning, Support) << mCheckChoice << ENDM;
  if (mCheckChoice == mCheckChoiceMean || mCheckChoice == mCheckChoiceBoth) {

    ILOG(Warning, Support) << Form("we have a entered Mean or Both") << ENDM;
    // const std::vector<double> v(yValues + NBins - 1 - pointNumberForMean, yValues + NBins - 1);

    const double sum = std::accumulate(v.begin(), v.end(), 0.0);
    const double meanFull = sum / NBins;

    for (const double& yvalue : v) {
      int i = &yvalue - &v[0];
      double yError = vErr[i];
      if (std::abs(yvalue - meanFull) <= yError * mNSigmaMean && !resultMean.isWorseThan(Quality::Good)) {
        resultMean = Quality::Good;
      } else if (std::abs(yvalue - meanFull) > yError * mNSigmaBadMean) {
        resultMean = Quality::Bad;
      } else if (!resultMean.isWorseThan(Quality::Medium)) {
        resultMean = Quality::Medium;
      } else {
        // just brick, this should hopefully never happen
        ILOG(Fatal, Support) << "Some Problem with the Quality happened. Quality: " << resultMean << ", Standard deviations: " << std::abs(yvalue - meanFull) / yError << ENDM;
      }
      if (resultMean == Quality::Bad) {
        break;
      }
    }
    ILOG(Warning, Support) << "Our Mean Quality:";
    ILOG(Warning, Support) << resultMean << ENDM;
  } // if (mCheckChoice == mCheckChoiceMean || mCheckChoice == mCheckChoiceBoth)
  result = resultMean;
  //########################## ExpectedPhysicsValue #######################################
  if (mCheckChoice == mCheckChoiceExpectedPhysicsValue || mCheckChoice == mCheckChoiceBoth) {

    ILOG(Warning, Support) << Form("we have a entered Physics Value or Both") << ENDM;
    for (const double& yvalue : v) {
      int i = &yvalue - &v[0];
      double yError = vErr[i];
      if (std::abs(yvalue - mExpectedPhysicsValue) <= yError * mNSigmaBadExpectedPhysicsValue && !resultExpectedPhysicsValue.isWorseThan(Quality::Good)) {
        resultExpectedPhysicsValue = Quality::Good;
      } else if (std::abs(yvalue - mExpectedPhysicsValue) > yError * mNSigmaBadExpectedPhysicsValue) {
        resultExpectedPhysicsValue = Quality::Bad;
      } else if (!resultExpectedPhysicsValue.isWorseThan(Quality::Medium)) {
        resultExpectedPhysicsValue = Quality::Medium;
      } else {
        // just brick, this should hopefully never happen
        ILOG(Fatal, Support) << "Some Problem with the Quality happened. Quality: " << resultExpectedPhysicsValue << ", Standard deviations: " << std::abs(yvalue - mExpectedPhysicsValue) / yError << ENDM;
      }
      if (resultExpectedPhysicsValue == Quality::Bad) {
        break;
      }
    }

    ILOG(Warning, Support) << "Our Physics Value Quality:";
    ILOG(Warning, Support) << resultExpectedPhysicsValue << ENDM;
    result = resultExpectedPhysicsValue;
  }
  if (mCheckChoice == mCheckChoiceBoth) {
    // If mean check is not performed, the total combined quality mean && expectedValue should be set to expectedValue result
    if (resultMean != Quality::Null) {
      if (resultMean.isWorseThan(resultExpectedPhysicsValue)) {
        result = resultMean;
      }
    }
  }

  ILOG(Warning, Support) << "Our Overall Quality:";
  ILOG(Warning, Support) << result << ENDM;
  return result;
}

std::string CheckOfSlices::getAcceptedType() { return "TCanvas"; }

void CheckOfSlices::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  auto* c1 = (TCanvas*)mo->getObject();
  TGraphErrors* h = (TGraphErrors*)c1->GetListOfPrimitives()->FindObject("Graph");
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
    if (mCheckChoice == mCheckChoiceExpectedPhysicsValue) {
      msg->AddText(Form("Outlier, more than %isigma.", mNSigmaBadExpectedPhysicsValue));
    } else if (mCheckChoice == mCheckChoiceMean) {
      msg->AddText(Form("Outlier, more than %isigma.", mNSigmaBadMean));
    } else {
      msg->AddText("Outlier. Bad Quality.");
    }
    msg->SetFillColor(kRed);
  } else if (checkResult == Quality::Medium) {
    LOG(info) << "Quality::medium, setting to orange";
    h->SetFillColor(kOrange);
    msg->Clear();
    msg->AddText("Quality::Medium");
    if (mCheckChoice == mCheckChoiceExpectedPhysicsValue) {
      msg->AddText(Form("Outlier, more than %isigma.", mNSigmaMediumExpectedPhysicsValue));
    } else if (mCheckChoice == mCheckChoiceMean) {
      msg->AddText(Form("Outlier, more than %isigma.", mNSigmaMediumMean));
    } else {
      msg->AddText("Outlier. Medium Quality");
    }
    msg->SetFillColor(kOrange);
  } else if (checkResult == Quality::Null) {
    h->SetFillColor(0);
  }
  h->SetLineColor(kBlack);

  const double xMin = h->GetXaxis()->GetXmin();
  const double xMax = h->GetXaxis()->GetXmax();
  TLine* lineExpectedValue = new TLine(xMin, mExpectedPhysicsValue, xMax, mExpectedPhysicsValue);
  lineExpectedValue->SetLineColor(kGreen);
  lineExpectedValue->SetLineWidth(2);
  // mean Line
  TLine* lineMean = new TLine(xMin, mean, xMax, mean);
  lineMean->SetLineColor(kGreen);
  lineMean->SetLineWidth(2);
  lineMean->SetLineStyle(10);

  h->GetListOfFunctions()->Add(lineExpectedValue);
  h->GetListOfFunctions()->Add(lineMean);

} // beautify function
} // namespace o2::quality_control_modules::tpc