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
#include "TPC/Utility.h"

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
  if (padList->GetEntries() > 1) {
    ILOG(Error, Support) << "CheckOfSlices does not support multiple pads from SliceTrending" << ENDM;
  }
  auto pad = static_cast<TPad*>(padList->At(0));
  if (!pad) {
    ILOG(Fatal, Support) << "Could not retrieve pad containing slice graph" << ENDM;
  }

  TGraphErrors* g = nullptr;
  g = static_cast<TGraphErrors*>(pad->GetPrimitive("Graph"));
  if (!g) {
    ILOG(Fatal, Support) << "No Graph object found" << ENDM;
  }

  const int NBins = g->GetN();
  const double* yValues = g->GetY();
  const double* yErrors = g->GetEY();
  bool useErrors = true;
  if (yErrors == nullptr) {
    useErrors = false;
    ILOG(Info, Support) << "NO ERRORS" << ENDM;
  }
  const std::vector<double> v(yValues, yValues + NBins); // use all points
  std::vector<double> vErr;
  if (useErrors) {
    const std::vector<double> vErrTemp(yErrors, yErrors + NBins); // use all points
    vErr = vErrTemp;
  } else {
    for (int i = 0; i < NBins; ++i) {
      vErr.push_back(0.);
    }
  }

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
      const double totalError = sqrt(mStdev * mStdev + yError * yError);
      if (std::abs(yvalue - mMean) < totalError * mNSigmaMean) {
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
      if (std::abs(yvalue - mExpectedPhysicsValue) < yError * mNSigmaExpectedPhysicsValue) {
        qualityPoints.push_back(Quality::Good);
        goodStringPoint += "ExpectedValueCheck \n";
      } else if (std::abs(yvalue - mExpectedPhysicsValue) > yError * mNSigmaBadExpectedPhysicsValue) {
        qualityPoints.push_back(Quality::Bad);
        badStringPoint += "ExpectedValueCheck \n";
      } else {
        qualityPoints.push_back(Quality::Medium);
        mediumStringPoint += "ExpectedValueCheck \n";
      }
    } // if (mExpectedValueCheck)

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
    } // if (mRangeCheck) {

    checks[Quality::Bad.getName()].push_back(badStringPoint);
    checks[Quality::Medium.getName()].push_back(mediumStringPoint);
    checks[Quality::Good.getName()].push_back(goodStringPoint);

    // aggregate qualities of this point between mean, expected and range into result
    auto Worst_Quality = std::max_element(qualityPoints.begin(), qualityPoints.end(),
                                          [](const Quality& q1, const Quality& q2) {
                                            return q1.isBetterThan(q2);
                                          });
    qualities.push_back(*Worst_Quality);

  } // for (size_t i = 0; i < v.size(); ++i)

  // Quality aggregation from previous checks
  if (qualities.size() >= 1) {
    auto Worst_Quality = std::max_element(qualities.begin(), qualities.end(),
                                          [](const Quality& q1, const Quality& q2) {
                                            return q1.isBetterThan(q2);
                                          });
    totalQuality = *Worst_Quality;

    // MetaData aggregation from previous checks
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

  if (totalQuality == Quality::Null) {
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
  auto* canv = dynamic_cast<TCanvas*>(mo->getObject());
  if (!canv) {
    ILOG(Fatal, Support) << "Canvas not found" << ENDM;
  }
  TList* padList = (TList*)canv->GetListOfPrimitives();
  padList->SetOwner(kTRUE);
  if (padList->GetEntries() > 1) {
    ILOG(Error, Support) << "CheckOfSlices does not support multiple pads from SliceTrending" << ENDM;
  }
  auto pad = static_cast<TPad*>(padList->At(0));
  if (!pad) {
    ILOG(Fatal, Support) << "Could not retrieve pad containing slice graph" << ENDM;
  }
  TGraphErrors* h = nullptr;
  h = static_cast<TGraphErrors*>(pad->GetPrimitive("Graph"));
  if (!h) {
    ILOG(Fatal, Support) << "No Graph object found" << ENDM;
  }

  const int nPoints = h->GetN();
  if (nPoints == 0) {
    ILOG(Fatal, Support) << "No bins were found for the Graph!" << ENDM;
  }

  const double* yValues = h->GetY();
  const double* yErrors = h->GetEY(); // returns nullptr for TGraph (no errors)
  bool useErrors = true;
  if (yErrors == nullptr) {
    useErrors = false;
  }

  TPaveText* msg = new TPaveText(0.5, 0.75, 0.9, 0.9, "NDC");
  h->GetListOfFunctions()->Add(msg);
  msg->SetName(fmt::format("{}_msg", mo->GetName()).data());

  std::string checkMessage;
  if (checkResult == Quality::Good) {
    h->SetFillColor(kGreen - 2);
    msg->Clear();
    msg->AddText("Quality::Good");
    msg->SetFillColor(kGreen - 2);
  } else if (checkResult == Quality::Bad) {
    h->SetFillColor(kRed);
    msg->Clear();
    msg->AddText("Quality::Bad. Failed checks:");
    checkMessage = mBadString;
    msg->SetFillColor(kRed);
  } else if (checkResult == Quality::Medium) {
    h->SetFillColor(kOrange);
    msg->Clear();
    msg->AddText("Quality::Medium. Failed checks:");
    checkMessage = mMediumString;
    msg->SetFillColor(kOrange);
  } else if (checkResult == Quality::Null) {
    h->SetFillColor(0);
    msg->AddText("Quality::Null. Failed checks:");
    checkMessage = mNullString;
  }

  // Split lines by hand as \n does not work with TPaveText
  const std::string delimiter = "\n";
  size_t pos = 0;
  std::string subText;
  while ((pos = checkMessage.find(delimiter)) != std::string::npos) {
    subText = checkMessage.substr(0, pos);
    msg->AddText(subText.c_str());
    checkMessage.erase(0, pos + delimiter.length());
  }
  msg->AddText(checkResult.getMetadata("Comment", "").c_str());

  const double xMin = h->GetPointX(0);
  const double xMax = h->GetPointX(nPoints - 1);

  if (mRangeCheck) {
    TLine* lineUpperRangeMed = new TLine(xMin, mExpectedPhysicsValue + mRangeMedium, xMax, mExpectedPhysicsValue + mRangeMedium);
    lineUpperRangeMed->SetLineColor(kBlack);
    lineUpperRangeMed->SetLineStyle(kDashed);
    lineUpperRangeMed->SetLineWidth(1);
    h->GetListOfFunctions()->Add(lineUpperRangeMed);
    TLine* lineLowerRangeMed = new TLine(xMin, mExpectedPhysicsValue - mRangeMedium, xMax, mExpectedPhysicsValue - mRangeMedium);
    lineLowerRangeMed->SetLineColor(kBlack);
    lineLowerRangeMed->SetLineStyle(kDashed);
    lineLowerRangeMed->SetLineWidth(1);
    h->GetListOfFunctions()->Add(lineLowerRangeMed);
    TLine* lineUpperRangeBad = new TLine(xMin, mExpectedPhysicsValue + mRangeBad, xMax, mExpectedPhysicsValue + mRangeBad);
    lineUpperRangeBad->SetLineColor(kBlack);
    lineUpperRangeBad->SetLineWidth(3);
    h->GetListOfFunctions()->Add(lineUpperRangeBad);
    TLine* lineLowerRangeBad = new TLine(xMin, mExpectedPhysicsValue - mRangeBad, xMax, mExpectedPhysicsValue - mRangeBad);
    lineLowerRangeBad->SetLineColor(kBlack);
    lineLowerRangeBad->SetLineWidth(3);
    h->GetListOfFunctions()->Add(lineLowerRangeBad);
  } // if (mRangeCheck)

  if (mExpectedValueCheck) {
    TLine* expectedValueLine = new TLine(xMin, mExpectedPhysicsValue, xMax, mExpectedPhysicsValue);

    TGraph* stddevGraphMediumUp = new TGraph();
    TGraph* stddevGraphMediumDown = new TGraph();
    TGraph* stddevGraphBadUp = new TGraph();
    TGraph* stddevGraphBadDown = new TGraph();

    for (int nBins = 0; nBins < nPoints; nBins++) {

      double PointError = 0.;
      if (useErrors) {
        PointError = *(yErrors + nBins);
      }
      if (PointError < 0.) {
        ILOG(Warning, Support) << "Point for check of mean has negative error" << ENDM;
      }

      stddevGraphMediumUp->AddPoint(h->GetPointX(nBins), mExpectedPhysicsValue + PointError * mNSigmaExpectedPhysicsValue);
      stddevGraphMediumDown->AddPoint(h->GetPointX(nBins), mExpectedPhysicsValue - PointError * mNSigmaExpectedPhysicsValue);
      stddevGraphBadUp->AddPoint(h->GetPointX(nBins), mExpectedPhysicsValue + PointError * mNSigmaBadExpectedPhysicsValue);
      stddevGraphBadDown->AddPoint(h->GetPointX(nBins), mExpectedPhysicsValue - PointError * mNSigmaBadExpectedPhysicsValue);
    } // for (int nBins = 1; nBins < nPoints; nBins++)

    expectedValueLine->SetLineWidth(2);
    expectedValueLine->SetLineColor(kGreen - 7);
    expectedValueLine->SetLineStyle(kDashed);

    stddevGraphMediumUp->SetLineWidth(2);
    stddevGraphMediumUp->SetLineColor(kOrange - 3);
    stddevGraphMediumUp->SetMarkerColor(kOrange - 3);
    stddevGraphMediumUp->SetLineStyle(kDashed);

    stddevGraphMediumDown->SetLineWidth(2);
    stddevGraphMediumDown->SetLineColor(kOrange - 3);
    stddevGraphMediumDown->SetMarkerColor(kOrange - 3);
    stddevGraphMediumDown->SetLineStyle(kDashed);

    stddevGraphBadUp->SetLineWidth(2);
    stddevGraphBadUp->SetLineColor(kRed - 3);
    stddevGraphBadUp->SetMarkerColor(kRed - 3);
    stddevGraphBadUp->SetLineStyle(kDashed);

    stddevGraphBadDown->SetLineWidth(2);
    stddevGraphBadDown->SetLineColor(kRed - 3);
    stddevGraphBadDown->SetMarkerColor(kRed - 3);
    stddevGraphBadDown->SetLineStyle(kDashed);

    h->GetListOfFunctions()->Add(expectedValueLine);
    h->GetListOfFunctions()->Add(stddevGraphMediumUp);
    h->GetListOfFunctions()->Add(stddevGraphMediumDown);
    h->GetListOfFunctions()->Add(stddevGraphBadUp);
    h->GetListOfFunctions()->Add(stddevGraphBadDown);
  } // if (mExpectedValueCheck)

  if (mMeanCheck) {
    TLine* meanGraph = new TLine(xMin, mMean, xMax, mMean);
    TGraph* stddevGraphMediumUp = new TGraph();
    TGraph* stddevGraphMediumDown = new TGraph();
    TGraph* stddevGraphBadUp = new TGraph();
    TGraph* stddevGraphBadDown = new TGraph();

    for (int nBins = 0; nBins < nPoints; nBins++) {

      double PointError = 0.;
      if (useErrors) {
        PointError = *(yErrors + nBins);
      }
      if (PointError < 0.) {
        ILOG(Warning, Support) << "Last point for check of mean has negative error" << ENDM;
      }

      double totalError = sqrt(mStdev * mStdev + PointError * PointError);

      stddevGraphMediumUp->AddPoint(h->GetPointX(nBins), mMean + totalError * mNSigmaMean);
      stddevGraphMediumDown->AddPoint(h->GetPointX(nBins), mMean - totalError * mNSigmaMean);
      stddevGraphBadUp->AddPoint(h->GetPointX(nBins), mMean + totalError * mNSigmaBadMean);
      stddevGraphBadDown->AddPoint(h->GetPointX(nBins), mMean - totalError * mNSigmaBadMean);
    } // for (int nBins = 1; nBins < nPoints; nBins++)

    meanGraph->SetLineWidth(2);
    meanGraph->SetLineColor(kGreen - 2);

    stddevGraphMediumUp->SetLineWidth(2);
    stddevGraphMediumUp->SetLineColor(kOrange);
    stddevGraphMediumUp->SetMarkerColor(kOrange);

    stddevGraphMediumDown->SetLineWidth(2);
    stddevGraphMediumDown->SetLineColor(kOrange);
    stddevGraphMediumDown->SetMarkerColor(kOrange);

    stddevGraphBadUp->SetLineWidth(2);
    stddevGraphBadUp->SetLineColor(kRed);
    stddevGraphBadUp->SetMarkerColor(kRed);

    stddevGraphBadDown->SetLineWidth(2);
    stddevGraphBadDown->SetLineColor(kRed);
    stddevGraphBadDown->SetMarkerColor(kRed);

    h->GetListOfFunctions()->Add(meanGraph);
    h->GetListOfFunctions()->Add(stddevGraphMediumUp);
    h->GetListOfFunctions()->Add(stddevGraphMediumDown);
    h->GetListOfFunctions()->Add(stddevGraphBadUp);
    h->GetListOfFunctions()->Add(stddevGraphBadDown);
  } // if (mMeanCheck)
} // beautify function

std::string CheckOfSlices::createMetaData(const std::vector<std::string>& pointMetaData)
{

  std::string meanString = "";
  std::string expectedValueString = "";
  std::string rangeString = "";

  for (int i; i < pointMetaData.size(); i++) {
    if (pointMetaData.at(i).find("MeanCheck") != std::string::npos) {
      meanString += " " + std::to_string(i + 1) + ",";
    }
    if (pointMetaData.at(i).find("ExpectedValueCheck") != std::string::npos) {
      expectedValueString += " " + std::to_string(i + 1) + ",";
    }
    if (pointMetaData.at(i).find("RangeCheck") != std::string::npos) {
      rangeString += " " + std::to_string(i + 1) + ",";
    }
  }

  std::string totalString = "";
  if (meanString != "") {
    meanString.pop_back();
    meanString = "MeanCheck (solid, coloured lines) for Points:" + meanString + "\n";
    totalString += meanString;
  }
  if (expectedValueString != "") {
    expectedValueString.pop_back();
    expectedValueString = "ExpectedValueCheck (dashed, coloured lines) for Points:" + expectedValueString + "\n";
    totalString += expectedValueString;
  }
  if (rangeString != "") {
    rangeString.pop_back();
    rangeString = "RangeCheck (black lines) for Points:" + rangeString + "\n";
    totalString += rangeString;
  }

  return totalString;
}

} // namespace o2::quality_control_modules::tpc