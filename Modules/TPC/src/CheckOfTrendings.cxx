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
#include "Common/Utils.h"
#include <fmt/format.h>
#include "TROOT.h"
#include "TRandom.h"

#include <fairlogger/Logger.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TGraphErrors.h>
#include <TList.h>
#include <TAxis.h>
#include <TLine.h>
#include <TPaveText.h>

#include <vector>
#include <numeric>

#include <cmath>

namespace o2::quality_control_modules::tpc
{

void CheckOfTrendings::configure()
{
  // mBadQualityLimit = common::getFromConfig<float>(mCustomParameters, "badQualityPercentageOfWorkingPads", 0.3);

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
  mMetadataComment = common::getFromConfig<std::string>(mCustomParameters, "MetadataComment", "");
  if (mExpectedValueCheck) {
    mNSigmaExpectedPhysicsValue = common::getFromConfig<float>(mCustomParameters, "allowedNSigmaForExpectation", 3);
    mNSigmaBadExpectedPhysicsValue = common::getFromConfig<float>(mCustomParameters, "badNSigmaForExpectation", 6);
    if (mNSigmaBadExpectedPhysicsValue < mNSigmaExpectedPhysicsValue) { // if bad < medium flip them.
      std::swap(mNSigmaBadExpectedPhysicsValue, mNSigmaExpectedPhysicsValue);
    }
    mPointToTakeForExpectedValueCheck = std::abs(common::getFromConfig<int>(mCustomParameters, "pointsToTakeForExpectedValueCheck", 10));
    mExpectedPhysicsValue = common::getFromConfig<float>(mCustomParameters, "expectedPhysicsValue", 1);
  }

  if (mMeanCheck) {
    mNSigmaMean = common::getFromConfig<float>(mCustomParameters, "allowedNSigmaForMean", 3);
    mNSigmaBadMean = common::getFromConfig<float>(mCustomParameters, "badNSigmaForMean", 6);
    if (mNSigmaBadMean < mNSigmaMean) { // if bad < medium flip them.
      std::swap(mNSigmaBadMean, mNSigmaMean);
    }
    mPointToTakeForMeanCheck = std::abs(common::getFromConfig<float>(mCustomParameters, "pointsToTakeForMeanCheck", 10));
  }

  if (mRangeCheck) {
    mRangeMedium = common::getFromConfig<float>(mCustomParameters, "allowedRange", 1);
    mRangeBad = common::getFromConfig<float>(mCustomParameters, "badRange", 2);
    if (mRangeBad < mRangeMedium) { // if bad < medium flip them.
      std::swap(mRangeBad, mRangeMedium);
    }
    mPointToTakeForRangeCheck = std::abs(common::getFromConfig<float>(mCustomParameters, "pointsToTakeForRangeCheck", 10));
    mExpectedPhysicsValue = common::getFromConfig<float>(mCustomParameters, "expectedPhysicsValue", 1);
  }

  if (mZeroCheck) {
    mPointToTakeForZeroCheck = std::abs(common::getFromConfig<float>(mCustomParameters, "pointsToTakeForZeroCheck", 10));
  }
}

Quality CheckOfTrendings::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  std::vector<std::string> checks;
  Quality result = Quality::Null;
  Quality resultMean = Quality::Null;
  Quality resultExpectedPhysicsValue = Quality::Null;
  Quality resultRange = Quality::Null;
  Quality resultZero = Quality::Null;
  std::vector<Quality> qualities;

  auto mo = moMap->begin()->second;
  auto* canv = (TCanvas*)mo->getObject();
  TGraph* g = nullptr;

  // if we have a trending with a TGraphErrors, then there will be a TGraph and a TGraphErrors with the same name ("Graph")
  // with the current configuration the TGraphErrors will always be added after the TGraph
  // loop from the back and find the last element with the name "Graph"
  int jList = canv->GetListOfPrimitives()->LastIndex();
  for (jList; jList > 0; jList--) {
    if (!strcmp(canv->GetListOfPrimitives()->At(jList)->GetName(), "Graph")) {
      g = (TGraph*)canv->GetListOfPrimitives()->At(jList);
      break;
    }
  }
  if (!g) {
    // if the upper loop somehow fails, log a warning and get the object the old fashioned way

    ILOG(Warning, Support) << "No TGraph found in the List of primitives." << ENDM;
    g = (TGraph*)canv->GetListOfPrimitives()->FindObject("Graph");
  }
  if (!g) { // if there is still no TGraph found, give an error and break

    ILOG(Fatal, Support) << "No TGraph found to perform Check against." << ENDM;
  }

  // this is for testing only! //GANESHA remove later
  int nn = g->GetN();
  for (int i = 0; i < nn; i++) {
    double xxx = g->GetPointX(i);
    g->SetPoint(i, xxx, g->GetPointY(i) - 0.5 * erf(i / 20.));
    // g->SetPointError(i, 0., 0.);
    if (i == 18) {
      g->SetPoint(i, xxx, g->GetPointY(i) + 0.5);
    }
  }

  const int NBins = g->GetN();
  // Mean Check here:
  //  If only one data point available, don't check quality for mean
  if (NBins > 1) {

    if (mMeanCheck) {
      double x_last = 0., y_last = 0.;
      g->GetPoint(NBins - 1, x_last, y_last);
      int pointNumberForMean = mPointToTakeForMeanCheck;
      if ((NBins - 1) < mPointToTakeForMeanCheck) {
        pointNumberForMean = NBins - 1;
      }
      const double* yValues = g->GetY();

      const double* yErrors = nullptr; // GANESHA add back -> this g->GetEY(); // returns nullptr for TGraph (no errors)

      bool NoError = false;
      if (yErrors == nullptr) {
        NoError = true;
      }

      double sum = 0;
      // double mean = 0;
      double sumOfWeights = 0;
      const std::vector<double> v(yValues + NBins - 1 - pointNumberForMean, yValues + NBins - 1);
      if (NoError) {
        sum = std::accumulate(v.begin(), v.end(), 0.0);
        sumOfWeights = v.size();
      }
      // can this be done with std::acumulate??
      else {
        const std::vector<double> vErr(yErrors + NBins - 1 - pointNumberForMean, yErrors + NBins - 1);
        for (int i = 0; i < v.size(); i++) {
          sum += v[i] * std::pow(vErr[i] / v[i], -2.);
          sumOfWeights += std::pow(vErr[i] / v[i], -2.);
        }
      }
      if (sumOfWeights == 0) {
        sumOfWeights = 1;
      }
      const double mean = sum / sumOfWeights;
      mMean = mean; // transfer to mMean for usage down in the beautify function

      // how do I handle statistical errors from spread and the actual points?
      // double mStdev = 0;
      double sq_sum = 0;
      if (NoError) { // NoError
        std::vector<double> diff(v.size());
        std::transform(v.begin(), v.end(), diff.begin(), [mean](double x) { return x - mean; });
        sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
        mStdev = std::sqrt(sq_sum / v.size());
      } else {
        mStdev = std::sqrt(1. / (sumOfWeights)); // this is only the error from the data point errors, how do we add in the spread of the points? should this be equivalent?
      }

      ILOG(Warning, Support) << mean << " " << mStdev << " " << y_last << ENDM;

      if (std::abs(y_last - mean) <= mStdev * mNSigmaMean) {
        resultMean = Quality::Good;
      } else if (std::abs(y_last - mean) > mStdev * mNSigmaBadMean) {
        resultMean = Quality::Bad;
      } else {
        resultMean = Quality::Medium;
      }
      qualities.push_back(resultMean);
      checks.push_back(fmt::format("Mean({:.2f}+-{:.2f})", mean, mStdev));
    }
  }
  // ExpectedValue Check here:
  if (mExpectedValueCheck) {
    int pointNumber = mPointToTakeForExpectedValueCheck;
    if (NBins < pointNumber) {
      pointNumber = NBins;
    }

    const double* yValues = g->GetY();
    const std::vector<double> v(yValues + NBins - pointNumber, yValues + NBins);

    const double sum = std::accumulate(v.begin(), v.end(), 0.0);
    const double mMeanFull = sum / v.size();

    std::vector<double> diff(v.size());
    std::transform(v.begin(), v.end(), diff.begin(), [mMeanFull](double x) { return x - mMeanFull; });
    const double sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
    const double mStdevFull = std::sqrt(sq_sum / v.size());

    if (std::abs(mMeanFull - mExpectedPhysicsValue) > mNSigmaBadExpectedPhysicsValue * mStdevFull) {
      resultExpectedPhysicsValue = Quality::Bad;
    } else if (std::abs(mMeanFull - mExpectedPhysicsValue) <= mNSigmaExpectedPhysicsValue * mStdevFull) {
      resultExpectedPhysicsValue = Quality::Good;
    } else {
      resultExpectedPhysicsValue = Quality::Medium;
    }
    qualities.push_back(resultExpectedPhysicsValue);
    checks.push_back(fmt::format("ExpectedValue({:.2f}+-{:.2f})", mExpectedPhysicsValue, mStdevFull));
  }

  // Range Check here:

  if (mRangeCheck) {
    int pointNumber = mPointToTakeForRangeCheck;
    if (NBins < pointNumber) {
      pointNumber = NBins;
    }

    const double* yValues = g->GetY();
    const std::vector<double> v(yValues + NBins - pointNumber, yValues + NBins);

    const double sum = std::accumulate(v.begin(), v.end(), 0.0);
    const double mMeanFull = sum / v.size();

    std::vector<double> diff(v.size());
    std::transform(v.begin(), v.end(), diff.begin(), [mMeanFull](double x) { return x - mMeanFull; });
    const double sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
    const double mStdevFull = std::sqrt(sq_sum / v.size());

    if (std::abs(mMeanFull - mExpectedPhysicsValue) > mRangeBad) {
      resultRange = Quality::Bad;
    } else if (std::abs(mMeanFull - mExpectedPhysicsValue) < mRangeMedium) {
      resultRange = Quality::Good;
    } else {
      resultRange = Quality::Medium;
    }

    qualities.push_back(resultRange);
    checks.push_back(fmt::format("Range({:.2f}+-{:.2f}/{:.2f})", mExpectedPhysicsValue, mRangeMedium, mRangeBad));
  }

  // Zeros Check here:
  if (mZeroCheck) {

    int pointNumber = mPointToTakeForZeroCheck;
    if (NBins < pointNumber) {
      pointNumber = NBins;
    }

    const double* yValues = g->GetY();
    const std::vector<double> v(yValues + NBins - pointNumber, yValues + NBins);
    bool AllZeros = std::all_of(v.begin(), v.end(), [](double i) { return std::abs(i) < 0.000001; });
    if (AllZeros) {
      resultZero = Quality::Bad;
    } else {
      resultZero = Quality::Good;
    }
    qualities.push_back(resultZero);
    checks.push_back("Zero");
  }

  // Quality aggregation
  if (qualities.size() >= 1) {
    auto Worst_Quality = std::max_element(qualities.begin(), qualities.end(),
                                          [](const Quality& q1, const Quality& q2) {
                                            return q1.isBetterThan(q2);
                                          });
    int Worst_Quality_Index = std::distance(qualities.begin(), Worst_Quality);
    result = *Worst_Quality;

    // For writing to metadata and drawing later

    mBadString = "";
    mMediumString = "";
    mGoodString = "";
    mNullString = "";

    for (int qualityindex = 0; qualityindex < qualities.size(); qualityindex++) {
      Quality q = qualities.at(qualityindex);
      if (q == Quality::Bad) {
        mBadString = mBadString + checks.at(qualityindex) + "\n";
      } else if (q == Quality::Medium) {
        mMediumString = mMediumString + checks.at(qualityindex) + " ";
      } else if (q == Quality::Good) {
        mGoodString = mGoodString + checks.at(qualityindex) + " ";
      } else {
        mNullString = mNullString + checks.at(qualityindex) + " ";
      }
    }
  }

  result.addMetadata("Bad", mBadString);
  result.addMetadata("Medium", mMediumString);
  result.addMetadata("Good", mGoodString);
  result.addMetadata("Null", mNullString);
  result.addMetadata("Comment", mMetadataComment);

  return result;
}

std::string CheckOfTrendings::getAcceptedType() { return "TCanvas"; }

void CheckOfTrendings::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  auto* c1 = dynamic_cast<TCanvas*>(mo->getObject());
  TGraph* h = (TGraph*)c1->GetListOfPrimitives()->FindObject("Graph");
  /*
    TGraph* h = nullptr;

    // if we have a trending with a TGraphErrors, then there will be a TGraph and a TGraphErrors with the same name ("Graph")
    // with the current configuration the TGraphErrors will always be added after the TGraph
    // loop from the back and find the last element with the name "Graph"
    int jList = c1->GetListOfPrimitives()->LastIndex();
    for (jList; jList > 0; jList--) {
      if (!strcmp(c1->GetListOfPrimitives()->At(jList)->GetName(), "Graph")) {
        h = (TGraph*)c1->GetListOfPrimitives()->At(jList);
        break;
      }
    }
    if (!h) {
      // if the upper loop somehow fails, log a warning and get the object the old fashioned way

      ILOG(Warning, Support) << "No TGraph found in the List of primitives." << ENDM;
      h = (TGraph*)c1->GetListOfPrimitives()->FindObject("Graph");
    }
    if (!h) { // if there is still no TGraph found, give an error and break

      ILOG(Fatal, Support) << "No TGraph found to perform Check against." << ENDM;
    }
  */
  // this is for testing only

  int nn = h->GetN();
  for (int i = 0; i < nn; i++) {
    double xxx = h->GetPointX(i);
    h->SetPoint(i, xxx, h->GetPointY(i) - 0.5 * erf(i / 20.));
    if (i == 18) {
      h->SetPoint(i, xxx, h->GetPointY(i) + 0.5);
    }
  }

  TPaveText* msg = new TPaveText(0.5, 0.75, 0.9, 0.9, "NDC");
  h->GetListOfFunctions()->Add(msg);
  msg->SetName(Form("%s_msg", mo->GetName()));

  if (checkResult == Quality::Good) {
    h->SetFillColor(kGreen);
    msg->Clear();
    msg->AddText("Quality::Good");
    msg->SetFillColor(kGreen);
  } else if (checkResult == Quality::Bad) {
    ILOG(Debug, Devel) << "Quality::Bad, setting to red";
    h->SetFillColor(kRed);
    msg->Clear();
    msg->AddText("Quality::Bad. Failed checks:");
    msg->AddText(mBadString.c_str());
    // msg->AddText("Outlier, more than 6sigma.");
    msg->SetFillColor(kRed);
  } else if (checkResult == Quality::Medium) {
    ILOG(Debug, Devel) << "Quality::medium, setting to orange";
    h->SetFillColor(kOrange);
    msg->Clear();
    msg->AddText("Quality::Medium. Failed checks:");
    msg->AddText(mMediumString.c_str());
    // msg->AddText("Outlier, more than 3sigma.");
    msg->SetFillColor(kOrange);
  } else if (checkResult == Quality::Null) {
    h->SetFillColor(0);
    msg->AddText(mNullString.c_str());
  }
  // msg->AddText(mMetadataComment.c_str());
  msg->AddText(checkResult.getMetadata("Comment", "Buh").c_str());
  h->SetLineColor(kBlack);

  const double xMin = h->GetXaxis()->GetXmin();
  const double xMax = h->GetXaxis()->GetXmax();

  if (mExpectedValueCheck) {
    TLine* lineExpectedValue = new TLine(xMin, mExpectedPhysicsValue, xMax, mExpectedPhysicsValue);
    lineExpectedValue->SetLineColor(kBlue);
    lineExpectedValue->SetLineWidth(2);
    lineExpectedValue->SetLineStyle(10);
    //+- 3sigma
    TLine* lineThreeSigmaP = new TLine(xMin, mExpectedPhysicsValue + mNSigmaExpectedPhysicsValue * mStdev, xMax, mExpectedPhysicsValue + mNSigmaExpectedPhysicsValue * mStdev);
    lineThreeSigmaP->SetLineColor(kOrange);
    lineThreeSigmaP->SetLineWidth(2);
    TLine* lineThreeSigmaM = new TLine(xMin, mExpectedPhysicsValue - mNSigmaExpectedPhysicsValue * mStdev, xMax, mExpectedPhysicsValue - mNSigmaExpectedPhysicsValue * mStdev);
    lineThreeSigmaM->SetLineColor(kOrange);
    lineThreeSigmaM->SetLineWidth(2);
    //+- 6sigma
    TLine* lineSixSigmaP = new TLine(xMin, mExpectedPhysicsValue + mNSigmaBadExpectedPhysicsValue * mStdev, xMax, mExpectedPhysicsValue + mNSigmaBadExpectedPhysicsValue * mStdev);
    lineSixSigmaP->SetLineColor(kRed);
    lineSixSigmaP->SetLineWidth(2);
    TLine* lineSixSigmaM = new TLine(xMin, mExpectedPhysicsValue - mNSigmaBadExpectedPhysicsValue * mStdev, xMax, mExpectedPhysicsValue - mNSigmaBadExpectedPhysicsValue * mStdev);
    lineSixSigmaM->SetLineColor(kRed);
    lineSixSigmaM->SetLineWidth(2);
    h->GetListOfFunctions()->Add(lineThreeSigmaP);
    h->GetListOfFunctions()->Add(lineThreeSigmaM);
    h->GetListOfFunctions()->Add(lineSixSigmaP);
    h->GetListOfFunctions()->Add(lineSixSigmaM);
    h->GetListOfFunctions()->Add(lineExpectedValue);
  }
  // this is omitted for the more advanced method below.
  /* if (mMeanCheck) {
     // mean Line
     TLine* lineMean = new TLine(xMin, mMean, xMax, mMean);
     lineMean->SetLineColor(kGreen);
     lineMean->SetLineWidth(2);
     lineMean->SetLineStyle(2);
     //+- 3sigma
     TLine* lineThreeSigmaP = new TLine(xMin, mMean + mNSigmaMean * mStdev, xMax, mMean + mNSigmaMean * mStdev);
     lineThreeSigmaP->SetLineColor(kOrange);
     lineThreeSigmaP->SetLineWidth(2);
     TLine* lineThreeSigmaM = new TLine(xMin, mMean - mNSigmaMean * mStdev, xMax, mMean - mNSigmaMean * mStdev);
     lineThreeSigmaM->SetLineColor(kOrange);
     lineThreeSigmaM->SetLineWidth(2);
     //+- 6sigma
     TLine* lineSixSigmaP = new TLine(xMin, mMean + mNSigmaBadMean * mStdev, xMax, mMean + mNSigmaBadMean * mStdev);
     lineSixSigmaP->SetLineColor(kRed);
     lineSixSigmaP->SetLineWidth(2);
     TLine* lineSixSigmaM = new TLine(xMin, mMean - mNSigmaBadMean * mStdev, xMax, mMean - mNSigmaBadMean * mStdev);
     lineSixSigmaM->SetLineColor(kRed);
     lineSixSigmaM->SetLineWidth(2);
     h->GetListOfFunctions()->Add(lineMean);
     h->GetListOfFunctions()->Add(lineThreeSigmaP);
     h->GetListOfFunctions()->Add(lineThreeSigmaM);
     h->GetListOfFunctions()->Add(lineSixSigmaP);
     h->GetListOfFunctions()->Add(lineSixSigmaM);
   }*/

  if (mRangeCheck) {

    TLine* lineUpperRangeMed = new TLine(xMin, mExpectedPhysicsValue + mRangeMedium, xMax, mExpectedPhysicsValue + mRangeMedium);
    lineUpperRangeMed->SetLineColor(kBlack);
    lineUpperRangeMed->SetLineWidth(1);
    h->GetListOfFunctions()->Add(lineUpperRangeMed);
    TLine* lineLowerRangeMed = new TLine(xMin, mExpectedPhysicsValue - mRangeMedium, xMax, mExpectedPhysicsValue - mRangeMedium);
    lineLowerRangeMed->SetLineColor(kBlack);
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
  }
  // for a continuous bad along the points if they are shifting calculate the mean for each point as a history
  // Draw the mean+stdev each point got compared against
  if (mMeanCheck) {
    TGraph* MeanGraph = new TGraph();
    TGraph* StddevGraphMediumUp = new TGraph();
    TGraph* StddevGraphMediumDown = new TGraph();
    TGraph* StddevGraphBadUp = new TGraph();
    TGraph* StddevGraphBadDown = new TGraph();

    const int nPoints = h->GetN();
    if (nPoints > 2) {
      for (int NBins = 1; NBins < nPoints; NBins++) {

        const double* yValues = h->GetY();
        const double* yErrors = nullptr; // GANESHA add back -> this h->GetEY(); // returns nullptr for TGraph (no errors)
        int pointNumberForMean = mPointToTakeForMeanCheck;
        if ((NBins) < mPointToTakeForMeanCheck) {
          pointNumberForMean = NBins;
        }

        bool NoError = false;
        if (yErrors == nullptr) {
          NoError = true;
        }

        double sum = 0;
        double sumOfWeights = 0;
        const std::vector<double> v(yValues + NBins - pointNumberForMean, yValues + NBins);

        if (NoError) {
          sum = std::accumulate(v.begin(), v.end(), 0.0);
          sumOfWeights = v.size();
        }

        // can this be done with std::acumulate??
        else {
          const std::vector<double> vErr(yErrors + NBins - pointNumberForMean, yErrors + NBins);
          for (int i = 0; i < v.size(); i++) {
            sum += v[i] * std::pow(vErr[i], -2.);
            sumOfWeights += std::pow(vErr[i], -2.);
          }
        }

        if (sumOfWeights == 0) {
          sumOfWeights = 1;
        }

        const double mean = sum / sumOfWeights;

        // how do I handle statistical errors from spread and the actual points?
        // double mStdev = 0;
        double stdev = 0;
        double sq_sum = 0;
        if (NoError) {
          std::vector<double> diff(v.size());
          std::transform(v.begin(), v.end(), diff.begin(), [mean](double x) { return x - mean; });
          sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
          stdev = std::sqrt(sq_sum / v.size());
        } else {
          stdev = std::sqrt(1. / (sumOfWeights)); // this is only the error from the data point errors, how do we add in the spread of the points? should this be equivalent?
        }
        // stdev += 0.05;
        //  Add the mean+-Stddev to a new TGraph and draw later
        MeanGraph->AddPoint(h->GetPointX(NBins), mean);
        MeanGraph->SetLineWidth(1);
        MeanGraph->SetLineColor(kBlack);
        StddevGraphMediumUp->AddPoint(h->GetPointX(NBins), mean + stdev * mNSigmaMean);
        StddevGraphMediumUp->SetLineWidth(1);
        StddevGraphMediumUp->SetLineColor(kOrange);
        StddevGraphMediumDown->AddPoint(h->GetPointX(NBins), mean - stdev * mNSigmaMean);
        StddevGraphMediumDown->SetLineWidth(1);
        StddevGraphMediumDown->SetLineColor(kOrange);
        StddevGraphBadUp->AddPoint(h->GetPointX(NBins), mean + stdev * mNSigmaBadMean);
        StddevGraphBadUp->SetLineWidth(1);
        StddevGraphBadUp->SetLineColor(kRed);
        StddevGraphBadDown->AddPoint(h->GetPointX(NBins), mean - stdev * mNSigmaBadMean);
        StddevGraphBadDown->SetLineWidth(1);
        StddevGraphBadDown->SetLineColor(kRed);
      } // for (int NBins = 1; NBins < nPoints; NBins++) {

      h->GetListOfFunctions()->Add(MeanGraph);
      h->GetListOfFunctions()->Add(StddevGraphMediumUp);
      h->GetListOfFunctions()->Add(StddevGraphMediumDown);
      h->GetListOfFunctions()->Add(StddevGraphBadUp);
      h->GetListOfFunctions()->Add(StddevGraphBadDown);
    }
  }
}

} // namespace o2::quality_control_modules::tpc