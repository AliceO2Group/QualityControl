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
/// \author Marcel Lesch
///

#include "TPC/CheckOfSlices.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include <fmt/format.h>
#include "Common/Utils.h"

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
// Backwards compability for old choice
  const std::string oldCheckChoiceString = common::getFromConfig<std::string>(mCustomParameters, "chooseCheckMeanOrExpectedPhysicsValueOrBoth", "");
  if (oldCheckChoiceString != "") {
    ILOG(Warning, Support) << "json using CheckOfTrendings needs to be updated! chooseCheckMeanOrExpectedPhysicsValueOrBoth was replaced by CheckChoice. Available options: Mean, ExpectedValue, Range, Zero" << ENDM;

    if (size_t finder = oldCheckChoiceString.find("Both"); finder != std::string::npos) {
      mMeanCheck = true;
      mExpectedValueCheck = true;
    } else if (size_t finder = oldCheckChoiceString.find("Mean"); finder != std::string::npos) {
      mMeanCheck = true;
    } else if (size_t finder = oldCheckChoiceString.find("ExpectedPhysicsValue"); finder != std::string::npos) {
      mExpectedValueCheck = true;
    }
  }

  const std::string checkChoiceString = common::getFromConfig<std::string>(mCustomParameters, "CheckChoice", "Mean");

  if (size_t finder = checkChoiceString.find("ExpectedValue"); finder != std::string::npos) {
    mExpectedValueCheck = true;
  }
  if (size_t finder = checkChoiceString.find("Mean"); finder != std::string::npos) {
    mMeanCheck = true;
  }
  if (size_t finder = checkChoiceString.find("Range"); finder != std::string::npos) {
    mRangeCheck = true;
  }
  if (size_t finder = checkChoiceString.find("Zero"); finder != std::string::npos) {
    mZeroCheck = true;
  }

  if (!mExpectedValueCheck && !mMeanCheck && !mRangeCheck && !mZeroCheck) { // A choice was provided but does not overlap with any of the provided options
    ILOG(Warning, Support) << "The chosen check option does not exist. Available options: Mean, ExpectedValue, Range, Zero. Multiple options can be chosen in parallel. Default Mean is now selected." << ENDM;
  }

  mMetadataComment = common::getFromConfig<std::string>(mCustomParameters, "MetadataComment", "");
  if (mExpectedValueCheck) {
    mNSigmaExpectedPhysicsValue = common::getFromConfig<double>(mCustomParameters, "allowedNSigmaForExpectation", 3);
    mNSigmaBadExpectedPhysicsValue = common::getFromConfig<double>(mCustomParameters, "badNSigmaForExpectation", 6);
    if (mNSigmaBadExpectedPhysicsValue < mNSigmaExpectedPhysicsValue) { // if bad < medium flip them.
      std::swap(mNSigmaBadExpectedPhysicsValue, mNSigmaExpectedPhysicsValue);
    }
    mExpectedPhysicsValue = common::getFromConfig<double>(mCustomParameters, "expectedPhysicsValue", 1);
  }

  if (mMeanCheck) {
    mNSigmaMean = common::getFromConfig<double>(mCustomParameters, "allowedNSigmaForMean", 3);
    mNSigmaBadMean = common::getFromConfig<double>(mCustomParameters, "badNSigmaForMean", 6);
    if (mNSigmaBadMean < mNSigmaMean) { // if bad < medium flip them.
      std::swap(mNSigmaBadMean, mNSigmaMean);
    }
  }

  if (mRangeCheck) {
    mRangeMedium = common::getFromConfig<double>(mCustomParameters, "allowedRange", 1);
    mRangeBad = common::getFromConfig<double>(mCustomParameters, "badRange", 2);
    if (mRangeBad < mRangeMedium) { // if bad < medium flip them.
      std::swap(mRangeBad, mRangeMedium);
    }
    mExpectedPhysicsValue = common::getFromConfig<double>(mCustomParameters, "expectedPhysicsValue", 1);
  }
}

Quality CheckOfSlices::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality totalQuality = Quality::Null;

  std::vector<Quality> qualities;
  std::unordered_map<std::string, std::vector<std::string>> checks;
  checks[Quality::Bad.getName()] = std::vector<std::string>();
  checks[Quality::Medium.getName()] = std::vector<std::string>();
  checks[Quality::Good.getName()] = std::vector<std::string>();

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
 
  auto pad = static_cast<TPad*>(padList->At(0));
  if(!pad)  {
    ILOG(Fatal, Support) << "Could not retrieve pad containing slice graph" << ENDM;
  }

  TGraphErrors* g = nullptr;
  g = static_cast<TGraphErrors*>(pad->GetPrimitive("Graph"));
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
  const double* yErrors = g->GetEY(); //GANESHA Change. It has to check if we have errors or not 
  bool useErrors = true;
  if (yErrors == nullptr) {
    useErrors = false;
    ILOG(Info, Support) << "NO ERRORS" << ENDM;
  }

  const std::vector<double> v(yValues, yValues + NBins);    // use all points
  const std::vector<double> vErr(yErrors, yErrors + NBins); // use all points //GANESHA check if useErrors
  calculateStatistics(yValues, yErrors, useErrors, 0, NBins, mMean, mStdev);

  for (size_t i = 0; i < v.size(); ++i) {

    Quality totalQualityPoint = Quality::Null; 
    std::vector<Quality> qualityPoints; 
    const auto yvalue = v[i];
    const auto yError = vErr[i];
    
    std::string badStringPoint = "";
    std::string mediumStringPoint = "";
    std::string goodStringPoint = "";
    
    if (mMeanCheck) { 
      const double totalError = sqrt(mStdev * mStdev + yError * yError); //GANESHA should I rmemove yError in this? mStdev and yError are correlated 
      if (std::abs(yvalue - mMean) <= totalError * mNSigmaMean) {
        qualityPoints.push_back(Quality::Good);
        goodStringPoint += "MeanCheck \n";
      } else if (std::abs(yvalue - mMean) > totalError * mNSigmaBadMean) {
        qualityPoints.push_back(Quality::Bad);
        badStringPoint += "MeanCheck \n";
      } else {
        qualityPoints.push_back(Quality::Medium);
        mediumStringPoint += "MeanCheck \n";
      } 
    } // if (mMeanCheck)

    if (mExpectedValueCheck) {
      if (std::abs(yvalue - mExpectedPhysicsValue) <= yError * mNSigmaExpectedPhysicsValue) {
        qualityPoints.push_back(Quality::Good);
        goodStringPoint += "ExpectedValueCheck \n";
      } else if (std::abs(yvalue - mExpectedPhysicsValue) > yError * mNSigmaBadExpectedPhysicsValue) {
        qualityPoints.push_back(Quality::Bad);
        badStringPoint += "ExpectedValueCheck \n";
      } else {
        qualityPoints.push_back(Quality::Medium);
        mediumStringPoint += "ExpectedValueCheck \n";
      } 
    }

      // Range Check:
    if (mRangeCheck) {
      if (std::abs(yvalue - mExpectedPhysicsValue) > mRangeBad) {
        qualityPoints.push_back(Quality::Bad);
        badStringPoint += "RangeCheck \n";
      } else if (std::abs(yvalue - mExpectedPhysicsValue) < mRangeMedium) {
        qualityPoints.push_back(Quality::Good);
        goodStringPoint += "RangeCheck \n";
      } else {
        qualityPoints.push_back(Quality::Medium);
        mediumStringPoint += "RangeCheck \n";
      }
    }

   checks[Quality::Bad.getName()].push_back(badStringPoint);
   checks[Quality::Medium.getName()].push_back(mediumStringPoint);
   checks[Quality::Good.getName()].push_back(goodStringPoint);


    //aggregate qualities of this point between mean, expected and range into result
    auto Worst_Quality = std::max_element(qualityPoints.begin(), qualityPoints.end(),
                                          [](const Quality& q1, const Quality& q2) {
                                            return q1.isBetterThan(q2);
                                          }); 
    qualities.push_back(*Worst_Quality);

  }  for (size_t i = 0; i < v.size(); ++i)

  //Quality aggregation from previous checks
  if (qualities.size() >= 1) {
    auto Worst_Quality = std::max_element(qualities.begin(), qualities.end(),
                                          [](const Quality& q1, const Quality& q2) {
                                            return q1.isBetterThan(q2);
                                          }); 
    totalQuality = *Worst_Quality;

    //MetaData aggregation from previous checks
    mBadString = createMetaData(checks[Quality::Bad.getName()]);
    mMediumString = createMetaData(checks[Quality::Medium.getName()]);
    mGoodString = createMetaData(checks[Quality::Good.getName()]);
  } 
  
  // Zeros Check:
  Quality qualityZeroCheck = Quality::Null;
  if (mZeroCheck) {
    const bool allZeros = std::all_of(v.begin(), v.end(), [](double i) { return std::abs(i) == 0.0; });
    if (allZeros) {
      mBadString += "ZeroCheck \n";
      totalQuality = Quality::Bad; 
    } else {
      mGoodString += "ZeroCheck \n"; 
    }
  }

  if(totalQuality == Quality::Null){
    mNullString = "No check performed"; 
  }

  totalQuality.addMetadata(Quality::Bad.getName(), mBadString);
  totalQuality.addMetadata(Quality::Medium.getName(), mMediumString);
  totalQuality.addMetadata(Quality::Good.getName(), mGoodString);
  totalQuality.addMetadata(Quality::Null.getName(), mNullString);
  totalQuality.addMetadata("Comment", mMetadataComment);

  return totalQuality;
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
  const double mMean = sum / NBins;
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
    if (mExpectedValueCheck) {
      msg->AddText(fmt::format("Outlier, more than {} sigma.", mNSigmaBadExpectedPhysicsValue).data());
    } else if (mMeanCheck) {
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
    if (mExpectedValueCheck) {
      msg->AddText(fmt::format("Outlier, more than {} sigma.", mNSigmaExpectedPhysicsValue).data());
    } else if (mMeanCheck) {
      msg->AddText(fmt::format("Outlier, more than {} sigma.", mNSigmaMean).data());
    } else {
      msg->AddText("Outlier. Medium Quality");
    }
    msg->SetFillColor(kOrange);
  } else if (checkResult == Quality::Null) {
    h->SetFillColor(0);
  }
  h->SetLineColor(kBlack);
  if (mExpectedValueCheck) {
    Legend->AddText(fmt::format("Expected Physics Value: {}", mExpectedPhysicsValue).data());
  }
  if (mMeanCheck) {
    Legend->AddText(fmt::format("Mean: {}", mMean).data());
  }
  const double xMin = h->GetXaxis()->GetXmin();
  const double xMax = h->GetXaxis()->GetXmax();
  TLine* lineExpectedValue = new TLine(xMin, mExpectedPhysicsValue, xMax, mExpectedPhysicsValue);
  lineExpectedValue->SetLineColor(kGreen);
  lineExpectedValue->SetLineWidth(2);
  // mean Line
  TLine* lineMean = new TLine(xMin, mMean, xMax, mMean);
  lineMean->SetLineColor(kOrange);
  lineMean->SetLineWidth(2);
  lineMean->SetLineStyle(10);
  h->GetListOfFunctions()->Add(lineExpectedValue);
  h->GetListOfFunctions()->Add(lineMean);

} // beautify function

void CheckOfSlices::calculateStatistics(const double* yValues, const double* yErrors, bool useErrors, const int firstPoint, const int lastPoint, double& mean, double& mStdev)
{

  // yErrors returns nullptr for TGraph (no errors)
  if (lastPoint - firstPoint <= 0) {
    ILOG(Error, Support) << "In calculateStatistics(), the first and last point of the range have to differ!" << ENDM;
    return;
  }

  double sum = 0.;
  double sumSquare = 0.;
  double sumOfWeights = 0.;        // sum w_i
  double sumOfSquaredWeights = 0.; // sum (w_i)^2
  double weight = 0.;

  const std::vector<double> v(yValues + firstPoint, yValues + lastPoint);
  if (!useErrors) {
    // In case of no errors, we set our weights equal to 1
    sum = std::accumulate(v.begin(), v.end(), 0.0);
    sumOfWeights = v.size();
    sumOfSquaredWeights = v.size();
  } else {
    // In case of errors, we set our weights equal to 1/sigma_i^2
    const std::vector<double> vErr(yErrors + firstPoint, yErrors + lastPoint);
    for (int i = 0; i < v.size(); i++) {
      weight = 1. / std::pow(vErr[i], 2.);
      sum += v[i] * weight;
      sumSquare += v[i] * v[i] * weight;
      sumOfWeights += weight;
      sumOfSquaredWeights += weight * weight;
    }
  }

  mean = sum / sumOfWeights;

  if (v.size() == 1) { // we only have one point, we keep it's uncertainty
    if (!useErrors) {
      mStdev = 0.;
    } else {
      mStdev = sqrt(1. / sumOfWeights);
    }
  } else { // for >= 2 points, we calculate the spread
    if (!useErrors) {
      std::vector<double> diff(v.size());
      std::transform(v.begin(), v.end(), diff.begin(), [mean](double x) { return x - mean; });
      double sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
      mStdev = std::sqrt(sq_sum / (v.size() * (v.size() - 1.)));
    } else {
      double ratioSumWeight = sumOfSquaredWeights / (sumOfWeights * sumOfWeights);
      mStdev = sqrt((sumSquare / sumOfWeights - mean * mean) * (1. / (1. - ratioSumWeight)) * ratioSumWeight);
    }
  }
}

std::string CheckOfSlices::createMetaData(std::vector<std::string> pointMetaData){

  std::string meanString = ""; 
  std::string expectedValueString = "";
  std::string rangeString = "";

  for(int i; i<pointMetaData.size(); i++){
    if (pointMetaData.at(i).find("MeanCheck") != std::string::npos) {
      meanString += " " + std::to_string(i+1) + ","; 
    }
    if (pointMetaData.at(i).find("ExpectedValueCheck") != std::string::npos) {
      expectedValueString += " " + std::to_string(i+1) + ","; 
    }
    if (pointMetaData.at(i).find("RangeCheck") != std::string::npos) {
      rangeString += " " + std::to_string(i+1) + ","; 
    }
  } 

  std::string totalString = ""; 
  if(meanString != ""){
    meanString = "MeanCheck for Points:" + meanString + "\n"; 
    totalString += meanString;
  }
  if(expectedValueString != ""){
    expectedValueString = "ExpectedValueCheck for Points:" + expectedValueString + "\n"; 
    totalString += expectedValueString;
  }
  if(rangeString != ""){
    rangeString = "RangeCheck for Points:" + rangeString + "\n"; 
    totalString += rangeString;
  }

  return totalString; 
}

} // namespace o2::quality_control_modules::tpc