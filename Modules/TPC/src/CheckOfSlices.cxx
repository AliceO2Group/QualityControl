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
#include <fmt/format.h>

#include <TCanvas.h>
#include <TGraphErrors.h>
#include <TGraph.h>
#include <TList.h>
#include <TPaveText.h>
#include <TAxis.h>
#include <TLine.h>

#include <vector>
#include <numeric>

namespace o2::quality_control_modules::tpc
{

void CheckOfSlices::configure()
{
  if (auto param = mCustomParameters.find("chooseCheckMeanOrExpectedPhysicsValueOrBoth"); param != mCustomParameters.end()) {
    mCheckChoice = param->second.c_str();
    if ((mCheckChoice != CheckChoiceMean) && (mCheckChoice != CheckChoiceExpectedPhysicsValue) && (mCheckChoice != CheckChoiceBoth)) {
      mCheckChoice = CheckChoiceMean;
      ILOG(Warning, Support) << "The chosen value does not exist. Available options: mean, ExpectedPhysicsValue, both. Default mean selected." << ENDM;
    }
    if ((mCheckChoice == CheckChoiceExpectedPhysicsValue) || (mCheckChoice == CheckChoiceBoth)) {
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
        ILOG(Info, Support) << "Chosen check requires mNSigmaExpectedPhysicsValue which is not given. Setting to default 3." << ENDM;
      }
      if (auto param = mCustomParameters.find("badNSigmaForExpectation"); param != mCustomParameters.end()) {
        mNSigmaBadExpectedPhysicsValue = std::atof(param->second.c_str());
      } else {
        mNSigmaBadExpectedPhysicsValue = 6;
        ILOG(Info, Support) << "Chosen check requires mNSigmaBadExpectedPhysicsValue which is not given. Setting to default 6." << ENDM;
      }
      /*
      if (auto param = mCustomParameters.find("pointsToTakeForExpectedValueCheck"); param != mCustomParameters.end()) {
        mPointToTakeForExpectedValueCheck = std::atof(param->second.c_str());
      } else {
        mPointToTakeForExpectedValueCheck = 10;
        ILOG(Warning, Support) << "Chosen check requires mPointToTakeForExpectedValueCheck which is not given. Set to default 10." << ENDM;
      }*/
    }

    if ((mCheckChoice == CheckChoiceMean) || (mCheckChoice == CheckChoiceBoth)) {
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
}

Quality CheckOfSlices::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;
  Quality resultMean = Quality::Null;
  Quality resultExpectedPhysicsValue = Quality::Null;

  auto mo = moMap->begin()->second;
  if (!mo) {
    ILOG(Fatal, Support) << "Monitoring object not found" << ENDM;
  }
  auto* canv = dynamic_cast<TCanvas*>(mo->getObject());
  if (!canv) {
    ILOG(Fatal, Support) << "Canvas not found" << ENDM;
  }
  TList* padList = (TList*)canv->GetListOfPrimitives();
  padList->SetOwner(kTRUE);
  const int numberPads = padList->GetEntries();
  if (numberPads > 1) {
    ILOG(Fatal, Support) << "Number of Pads: " << numberPads << " Should not be more than 1" << ENDM;
  }
  TGraphErrors* g = nullptr;
  for (int iPad = 0; iPad < numberPads; iPad++) {
    auto pad = static_cast<TPad*>(padList->At(iPad));
    g = static_cast<TGraphErrors*>(pad->GetPrimitive("Graph"));
  }

  if (!g) {
    ILOG(Fatal, Support) << "No Graph object found" << ENDM;
  }
  if (strcmp(g->GetName(), "Graph") != 0) {
    ILOG(Fatal, Support) << "If found an object of type: " << g->GetName() << " should be Graph" << ENDM;
  }

  const int NBins = g->GetN();

  // If only one data point available, don't check quality for mean --  not really necessary in slices. Maybe add an expected number of Slices?
  // if (NBins > 1) {
  const double* yValues = g->GetY();
  const double* yErrors = g->GetEY();
  const std::vector<double> v(yValues, yValues + NBins);    // use all points
  const std::vector<double> vErr(yErrors, yErrors + NBins); // use all points

  // now we have the mean and the stddev. Now check if all points are within the margins (default 3,6 sigma margins)

  if ((mCheckChoice == CheckChoiceMean) || (mCheckChoice == CheckChoiceBoth)) {

    // const std::vector<double> v(yValues + NBins - 1 - pointNumberForMean, yValues + NBins - 1);

    const double sum = std::accumulate(v.begin(), v.end(), 0.0);
    const double meanFull = sum / NBins;

    for (size_t i = 0; i < v.size(); ++i) {
      const auto yvalue = v[i];
      const auto yError = vErr[i];
      if (std::abs(yvalue - meanFull) <= yError * mNSigmaMean) {
        if (!resultMean.isWorseThan(Quality::Good) || resultMean == Quality::Null) {
          resultMean = Quality::Good;
        }
      } else if (std::abs(yvalue - meanFull) > yError * mNSigmaBadMean) {
        resultMean = Quality::Bad;
      } else if (!resultMean.isWorseThan(Quality::Medium) || resultMean == Quality::Null) {
        resultMean = Quality::Medium;
      } else {
        // just brick, this should hopefully never happen
        ILOG(Fatal, Support) << "Some Problem with the Quality happened. Quality: " << resultMean << ", Standard deviations: " << std::abs(yvalue - meanFull) / yError << ENDM;
      }
      if (resultMean == Quality::Bad) {
        break;
      }
      //}
    }
  } // if (mCheckChoice == mCheckChoiceMean || mCheckChoice == mCheckChoiceBoth)
  result = resultMean;
  //########################## ExpectedPhysicsValue #######################################
  if ((mCheckChoice == CheckChoiceExpectedPhysicsValue) || (mCheckChoice == CheckChoiceBoth)) {
    for (size_t i = 0; i < v.size(); ++i) {
      const auto yvalue = v[i];
      const auto yError = vErr[i];
      if (std::abs(yvalue - mExpectedPhysicsValue) <= yError * mNSigmaExpectedPhysicsValue) {
        if (!resultExpectedPhysicsValue.isWorseThan(Quality::Good) || resultExpectedPhysicsValue == Quality::Null) {
          resultExpectedPhysicsValue = Quality::Good;
        }
      } else if (std::abs(yvalue - mExpectedPhysicsValue) > yError * mNSigmaBadExpectedPhysicsValue) {
        resultExpectedPhysicsValue = Quality::Bad;
      } else if (!resultExpectedPhysicsValue.isWorseThan(Quality::Medium) || resultExpectedPhysicsValue == Quality::Null) {
        resultExpectedPhysicsValue = Quality::Medium;
      } else {
        // just brick, this should hopefully never happen
        ILOG(Fatal, Support) << "Some Problem with the Quality happened. Quality: " << resultExpectedPhysicsValue << ", Standard deviations: " << std::abs(yvalue - mExpectedPhysicsValue) / yError << ENDM;
      }
      if (resultExpectedPhysicsValue == Quality::Bad) {
        break;
      }
      // }
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

void CheckOfSlices::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  auto* c1 = dynamic_cast<TCanvas*>(mo->getObject());

  TList* padList = (TList*)c1->GetListOfPrimitives();
  padList->SetOwner(kTRUE);
  int numberPads = padList->GetEntries();
  TGraphErrors* h = nullptr;
  for (int iPad = 0; iPad < numberPads; iPad++) {
    auto pad = static_cast<TPad*>(padList->At(iPad));
    h = static_cast<TGraphErrors*>(pad->GetPrimitive("Graph"));
  }
  if (!h) {
    ILOG(Fatal, Support) << "Could not find graph in given Pad" << ENDM;
  }
  const int NBins = h->GetN();
  if (NBins == 0) {
    ILOG(Fatal, Support) << "No bins were found for the Graph!" << ENDM;
  }
  const double* yValues = h->GetY();
  const double* yErrors = h->GetEY();
  const std::vector<double> v(yValues, yValues + NBins); // use all points
  const std::vector<double> vErr(yErrors, yErrors + NBins);
  const double sum = std::accumulate(v.begin(), v.end(), 0.0);
  const double meanFull = sum / NBins;
  TPaveText* msg = new TPaveText(0.7, 0.85, 0.9, 0.9, "NDC");
  TPaveText* Legend = new TPaveText(0.1, 0.85, 0.35, 0.9, "NDC");
  h->GetListOfFunctions()->Add(msg);
  h->GetListOfFunctions()->Add(Legend);
  msg->SetName(fmt::format("{}_msg", mo->GetName()).data());

  if (checkResult == Quality::Good) {
    h->SetFillColor(kGreen);
    msg->Clear();
    msg->AddText("Quality::Good");
    msg->SetFillColor(kGreen);
  } else if (checkResult == Quality::Bad) {
    ILOG(Info, Support) << "Quality::Bad, setting to red";
    h->SetFillColor(kRed);
    msg->Clear();
    msg->AddText("Quality::Bad");
    if (mCheckChoice == CheckChoiceExpectedPhysicsValue) {
      msg->AddText(fmt::format("Outlier, more than {} sigma.", mNSigmaBadExpectedPhysicsValue).data());
    } else if (mCheckChoice == CheckChoiceMean) {
      msg->AddText(fmt::format("Outlier, more than {} sigma.", mNSigmaBadMean).data());
    } else {
      msg->AddText("Outlier. Bad Quality.");
    }
    msg->SetFillColor(kRed);
  } else if (checkResult == Quality::Medium) {
    ILOG(Info, Support) << "Quality::medium, setting to orange";
    h->SetFillColor(kOrange);
    msg->Clear();
    msg->AddText("Quality::Medium");
    if (mCheckChoice == CheckChoiceExpectedPhysicsValue) {
      msg->AddText(fmt::format("Outlier, more than {} sigma.", mNSigmaExpectedPhysicsValue).data());
    } else if (mCheckChoice == CheckChoiceMean) {
      msg->AddText(fmt::format("Outlier, more than {} sigma.", mNSigmaMean).data());
    } else {
      msg->AddText("Outlier. Medium Quality");
    }
    msg->SetFillColor(kOrange);
  } else if (checkResult == Quality::Null) {
    h->SetFillColor(0);
  }
  h->SetLineColor(kBlack);
  if (mCheckChoice == CheckChoiceExpectedPhysicsValue || mCheckChoice == CheckChoiceBoth) {
    Legend->AddText(fmt::format("Expected Physics Value: {}", mExpectedPhysicsValue).data());
  }
  if (mCheckChoice == CheckChoiceMean || mCheckChoice == CheckChoiceBoth) {
    Legend->AddText(fmt::format("Mean: {}", meanFull).data());
  }
  const double xMin = h->GetXaxis()->GetXmin();
  const double xMax = h->GetXaxis()->GetXmax();
  TLine* lineExpectedValue = new TLine(xMin, mExpectedPhysicsValue, xMax, mExpectedPhysicsValue);
  lineExpectedValue->SetLineColor(kGreen);
  lineExpectedValue->SetLineWidth(2);
  // mean Line
  TLine* lineMean = new TLine(xMin, meanFull, xMax, meanFull);
  lineMean->SetLineColor(kOrange);
  lineMean->SetLineWidth(2);
  lineMean->SetLineStyle(10);
  h->GetListOfFunctions()->Add(lineExpectedValue);
  h->GetListOfFunctions()->Add(lineMean);

} // beautify function
} // namespace o2::quality_control_modules::tpc