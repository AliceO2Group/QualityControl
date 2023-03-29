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
/// \author Marcel Lesch
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
#include <TGraph.h>
#include <TCanvas.h>
#include <TGraphErrors.h>
#include <TList.h>
#include <TAxis.h>
#include <TLine.h>
#include <TPaveText.h>
#include <TH2.h>

#include <vector>
#include <numeric>
#include <cmath>

namespace o2::quality_control_modules::tpc
{

void CheckOfTrendings::configure()
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
    mPointToTakeForRangeCheck = std::abs(common::getFromConfig<float>(mCustomParameters, "pointsToTakeForRangeCheck", 1));
    mExpectedPhysicsValue = common::getFromConfig<float>(mCustomParameters, "expectedPhysicsValue", 1);
  }

  if (mZeroCheck) {
    mPointToTakeForZeroCheck = std::abs(common::getFromConfig<float>(mCustomParameters, "pointsToTakeForZeroCheck", 1));
  }

  mSliceTrend = common::getFromConfig<bool>(mCustomParameters, "SliceTrending", true);
}

Quality CheckOfTrendings::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  mPadQualities.clear();
  mPadMetaData.clear();
  mPadMetaData[Quality::Null.getName()] = std::vector<std::string>();
  mPadMetaData[Quality::Bad.getName()] = std::vector<std::string>();
  mPadMetaData[Quality::Medium.getName()] = std::vector<std::string>();
  mPadMetaData[Quality::Good.getName()] = std::vector<std::string>();

  std::vector<TGraph*> graphs;

  auto mo = moMap->begin()->second;
  if (!mo) {
    ILOG(Fatal, Support) << "Monitoring object not found" << ENDM;
  }
  auto* canv = dynamic_cast<TCanvas*>(mo->getObject());
  if (!canv) {
    ILOG(Fatal, Support) << "Canvas not found" << ENDM;
  }
  getGraphs(canv, graphs);

  if (graphs.size() == 0) {
    ILOG(Fatal, Support) << "Could not retrieve any TGraph for CheckOfTrendings" << ENDM;
  }
  for (int iGraph = 0; iGraph < graphs.size(); iGraph++) {
    if (!graphs[iGraph]) { // if there is no TGraph, give an error and break
      ILOG(Fatal, Support) << "TGraph number " << iGraph << " is NULL." << ENDM;
    }
  }
  if (!mSliceTrend && graphs.size() > 1) {
    ILOG(Fatal, Support) << "Multiple Graphs found even though this is not a slice trending" << ENDM;
  }

  for (int iGraph = 0; iGraph < graphs.size(); iGraph++) {

    std::string padNullString = "";
    std::string padBadString = "";
    std::string padMediumString = "";
    std::string padGoodString = "";
    std::string checkMessage = "";
    std::vector<Quality> qualitiesOfPad;

    const int nBins = graphs[iGraph]->GetN();
    const double* yValues = graphs[iGraph]->GetY();
    const double* yErrors = graphs[iGraph]->GetEY(); // returns nullptr for TGraph (no errors)

    bool useErrors = true;
    if (yErrors == nullptr) {
      useErrors = false;
      ILOG(Info, Support) << "NO ERRORS" << ENDM;
    }

    double mean = 0.;
    double stddevOfMean = 0.;

    // Mean Check:
    if (mMeanCheck) {
      if (nBins <= 1) { // If only one data point available, don't check quality for mean
        qualitiesOfPad.push_back(Quality::Null);
        padNullString += "MeanCheck: Could not perform check due as there is only one data point or less \n";
      } else {

        checkMessage = "";
        double x_last = 0., y_last = 0.;
        graphs[iGraph]->GetPoint(nBins - 1, x_last, y_last);
        int pointNumberForMean = mPointToTakeForMeanCheck;
        if ((nBins - 1) < mPointToTakeForMeanCheck) {
          pointNumberForMean = nBins - 1;
        }

        calculateStatistics(yValues, yErrors, useErrors, nBins - 1 - pointNumberForMean, nBins - 1, mean, stddevOfMean);

        double lastPointError = 0.;
        if (useErrors) {
          lastPointError = *(yErrors + nBins - 1); // Error of last point
        }
        if (lastPointError < 0.) {
          ILOG(Warning, Support) << "Last point for check of mean has negative error" << ENDM;
        }

        double totalError = sqrt(stddevOfMean * stddevOfMean + lastPointError * lastPointError);
        double nSigma = -1.;
        if (totalError != 0.) {
          nSigma = std::abs(y_last - mean) / totalError;
        }
        if (nSigma < 0.) {
          qualitiesOfPad.push_back(Quality::Bad);
          padBadString += "MeanCheck: Final uncertainty of mean is zero \n";
        } else {
          checkMessage = fmt::format("MeanCheck: {:.1f}#sigma difference to mean({:.2f}#pm{:.2f}) \n", nSigma, mean, totalError);

          if (std::abs(y_last - mean) < totalError * mNSigmaMean) {
            qualitiesOfPad.push_back(Quality::Good);
            padGoodString += checkMessage;
          } else if (std::abs(y_last - mean) > totalError * mNSigmaBadMean) {
            qualitiesOfPad.push_back(Quality::Bad);
            padBadString += checkMessage;
          } else {
            qualitiesOfPad.push_back(Quality::Medium);
            padMediumString += checkMessage;
          }
        }
      }
    } // if (mMeanCheck)

    // ExpectedValue Check:
    if (mExpectedValueCheck) {
      checkMessage = "";
      int pointNumber = mPointToTakeForExpectedValueCheck;
      if (!useErrors && pointNumber == 1) { // if we only have one point without errrors, the stddev = 0 and the check would not make any sense
        ILOG(Warning, Support) << "Expected Value Check: Cannot use 1 point without errors. Switching to two points" << ENDM;
        pointNumber = 2;
      }
      if (nBins < pointNumber) {
        pointNumber = nBins;
      }

      if (!useErrors && pointNumber == 1) { // changing from 1 to 2 points failed because only 1 point is available -> no quality
        qualitiesOfPad.push_back(Quality::Null);
        padNullString += "ExpectedValueCheck: Only one data point without errors \n";
      } else {
        calculateStatistics(yValues, yErrors, useErrors, nBins - pointNumber, nBins, mean, stddevOfMean);
        mMean = mean;
        mStdev = stddevOfMean;

        double nSigma = -1.;
        if (stddevOfMean != 0.) {
          nSigma = std::abs(mean - mExpectedPhysicsValue) / stddevOfMean;
        }
        if (nSigma < 0.) {
          qualitiesOfPad.push_back(Quality::Bad);
          padBadString += "ExpectedValueCheck: Final uncertainty of mean is zero \n";
        } else {
          checkMessage = fmt::format("ExpectedValueCheck: {:.1f}#sigma difference to ExpectedValue({:.2f}) \n", nSigma, mExpectedPhysicsValue);

          if (std::abs(mean - mExpectedPhysicsValue) > mNSigmaBadExpectedPhysicsValue * stddevOfMean) {
            qualitiesOfPad.push_back(Quality::Bad);
            padBadString += checkMessage;
          } else if (std::abs(mean - mExpectedPhysicsValue) < mNSigmaExpectedPhysicsValue * stddevOfMean) {
            qualitiesOfPad.push_back(Quality::Good);
            padGoodString += checkMessage;
          } else {
            qualitiesOfPad.push_back(Quality::Medium);
            padMediumString += checkMessage;
          }
        }
      }
    } // if (mExpectedValueCheck)

    // Range Check:
    if (mRangeCheck) {
      int pointNumber = mPointToTakeForRangeCheck;
      if (nBins < pointNumber) {
        pointNumber = nBins;
      }

      calculateStatistics(yValues, yErrors, useErrors, nBins - pointNumber, nBins, mean, stddevOfMean);

      if (std::abs(mean - mExpectedPhysicsValue) > mRangeBad) {
        qualitiesOfPad.push_back(Quality::Bad);
        padBadString += fmt::format("RangeCheck: Out of range of ExpectedValue({:.2f}) by more than #pm{:.2f} \n", mExpectedPhysicsValue, mRangeBad);
      } else if (std::abs(mean - mExpectedPhysicsValue) < mRangeMedium) {
        qualitiesOfPad.push_back(Quality::Good);
        padGoodString += fmt::format("RangeCheck: In range of ExpectedValue({:.2f}) within #pm{:.2f} \n", mExpectedPhysicsValue, mRangeMedium);
      } else {
        qualitiesOfPad.push_back(Quality::Medium);
        padMediumString += fmt::format("RangeCheck: Out of range of ExpectedValue({:.2f}) between #pm{:.2f} and #pm{:.2f} \n", mExpectedPhysicsValue, mRangeMedium, mRangeBad);
      }
    } // if(mRangeCheck)

    // Zeros Check:
    if (mZeroCheck) {
      int pointNumber = mPointToTakeForZeroCheck;
      if (nBins < pointNumber) {
        pointNumber = nBins;
      }

      const std::vector<double> v(yValues + nBins - pointNumber, yValues + nBins);
      const bool allZeros = std::all_of(v.begin(), v.end(), [](double i) { return std::abs(i) == 0.0; });
      if (allZeros) {
        qualitiesOfPad.push_back(Quality::Bad);
        padBadString += fmt::format("ZeroCheck: Last {} Points zero \n", pointNumber);
      } else {
        qualitiesOfPad.push_back(Quality::Good);
        padGoodString += "ZeroCheck: Not all Points zero \n";
      }
    } // if (mZeroCheck)

    // Quality aggregation
    if (qualitiesOfPad.size() >= 1) {
      auto Worst_Quality_Pad = std::max_element(qualitiesOfPad.begin(), qualitiesOfPad.end(),
                                                [](const Quality& q1, const Quality& q2) {
                                                  return q1.isBetterThan(q2);
                                                });
      mPadQualities.push_back(*Worst_Quality_Pad);
    } else {
      mPadQualities.push_back(Quality::Null);
      padNullString += "No Checks performed for pad " + std::to_string(iGraph) + " \n";
    }

    mPadMetaData[Quality::Null.getName()].push_back(padNullString);
    mPadMetaData[Quality::Bad.getName()].push_back(padBadString);
    mPadMetaData[Quality::Medium.getName()].push_back(padMediumString);
    mPadMetaData[Quality::Good.getName()].push_back(padGoodString);
  } // for(int iGraph = 0; iGraph < graphs.size(); iGraph++)

  // Final Aggregation of qualities and metadata of all Pads
  std::string totalBadString = "";
  std::string totalMediumString = "";
  std::string totalGoodString = "";
  std::string totalNullString = "";
  Quality totalQuality = Quality::Null;

  if (mPadQualities.size() >= 1) {
    auto Worst_Quality = std::max_element(mPadQualities.begin(), mPadQualities.end(),
                                          [](const Quality& q1, const Quality& q2) {
                                            return q1.isBetterThan(q2);
                                          });
    totalQuality = *Worst_Quality;

    // MetaData aggregation from all pads
    totalBadString = createMetaData(mPadMetaData[Quality::Bad.getName()]);
    totalMediumString = createMetaData(mPadMetaData[Quality::Medium.getName()]);
    totalGoodString = createMetaData(mPadMetaData[Quality::Good.getName()]);
    totalNullString = createMetaData(mPadMetaData[Quality::Null.getName()]);
  } else {
    totalNullString = "No Check was performed \n";
  }

  totalQuality.addMetadata(Quality::Bad.getName(), totalBadString);
  totalQuality.addMetadata(Quality::Medium.getName(), totalMediumString);
  totalQuality.addMetadata(Quality::Good.getName(), totalGoodString);
  totalQuality.addMetadata(Quality::Null.getName(), totalNullString);
  totalQuality.addMetadata("Comment", mMetadataComment);

  return totalQuality;
}

std::string CheckOfTrendings::getAcceptedType() { return "TCanvas"; }

void CheckOfTrendings::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  auto* canv = dynamic_cast<TCanvas*>(mo->getObject());
  if (!canv) {
    ILOG(Fatal, Support) << "Canvas not found" << ENDM;
  }

  std::vector<TGraph*> graphs;
  getGraphs(canv, graphs);

  if (graphs.size() == 0) {
    ILOG(Fatal, Support) << "Could not retrieve any TGraph for CheckOfTrendings" << ENDM;
  }
  for (int iGraph = 0; iGraph < graphs.size(); iGraph++) {
    if (!graphs[iGraph]) { // if there is no TGraph, give an error and break
      ILOG(Fatal, Support) << "TGraph number " << iGraph << " is NULL." << ENDM;
    }
  }
  if (!mSliceTrend && graphs.size() > 1) {
    ILOG(Fatal, Support) << "Multiple Graphs found even though this is not a slice trending" << ENDM;
  }

  for (int iGraph = 0; iGraph < graphs.size(); iGraph++) {
    graphs[iGraph]->SetLineColor(kBlack);
    double xMin = graphs[iGraph]->GetXaxis()->GetXmin();
    double xMax = graphs[iGraph]->GetXaxis()->GetXmax();
    double yMin = 0.;
    double yMax = 0.;
    if (mSliceTrend) {
      yMin = graphs[iGraph]->GetYaxis()->GetXmin();
      yMax = graphs[iGraph]->GetYaxis()->GetXmax();
    } else {
      TH2* htemp = (TH2*)canv->GetListOfPrimitives()->FindObject("htemp");
      if (htemp) {
        xMin = htemp->GetXaxis()->GetXmin();
        xMax = htemp->GetXaxis()->GetXmax();
        yMin = htemp->GetYaxis()->GetXmin();
        yMax = htemp->GetYaxis()->GetXmax();
      }
    }
    if (mMeanCheck) {
      int nn = graphs[iGraph]->GetN();
      xMin = graphs[iGraph]->GetPointX(nn - 1) + (xMax - graphs[iGraph]->GetPointX(nn - 1)) / 4.;
    }

    if (mExpectedValueCheck) {
      TBox* badBox = new TBox(xMin, yMin, xMax, yMax);
      badBox->SetFillColor(kRed);
      badBox->SetFillStyle(1001);
      badBox->SetLineWidth(0);
      TBox* mediumBox = new TBox(xMin, mExpectedPhysicsValue - mNSigmaBadExpectedPhysicsValue * mStdev, xMax, mExpectedPhysicsValue + mNSigmaBadExpectedPhysicsValue * mStdev);
      mediumBox->SetFillColor(kOrange);
      mediumBox->SetFillStyle(1001);
      mediumBox->SetLineWidth(0);
      TBox* goodBox = new TBox(xMin, mExpectedPhysicsValue - mNSigmaExpectedPhysicsValue * mStdev, xMax, mExpectedPhysicsValue + mNSigmaExpectedPhysicsValue * mStdev);
      goodBox->SetFillColor(kGreen - 2);
      goodBox->SetFillStyle(1001);
      goodBox->SetLineWidth(0);
      graphs[iGraph]->GetListOfFunctions()->Add(badBox);
      graphs[iGraph]->GetListOfFunctions()->Add(mediumBox);
      graphs[iGraph]->GetListOfFunctions()->Add(goodBox);
    }

    if (mRangeCheck) {
      TLine* lineUpperRangeMed = new TLine(xMin, mExpectedPhysicsValue + mRangeMedium, xMax, mExpectedPhysicsValue + mRangeMedium);
      lineUpperRangeMed->SetLineColor(kBlack);
      lineUpperRangeMed->SetLineStyle(kDashed);
      lineUpperRangeMed->SetLineWidth(1);
      graphs[iGraph]->GetListOfFunctions()->Add(lineUpperRangeMed);
      TLine* lineLowerRangeMed = new TLine(xMin, mExpectedPhysicsValue - mRangeMedium, xMax, mExpectedPhysicsValue - mRangeMedium);
      lineLowerRangeMed->SetLineColor(kBlack);
      lineLowerRangeMed->SetLineStyle(kDashed);
      lineLowerRangeMed->SetLineWidth(1);
      graphs[iGraph]->GetListOfFunctions()->Add(lineLowerRangeMed);
      TLine* lineUpperRangeBad = new TLine(xMin, mExpectedPhysicsValue + mRangeBad, xMax, mExpectedPhysicsValue + mRangeBad);
      lineUpperRangeBad->SetLineColor(kBlack);
      lineUpperRangeBad->SetLineWidth(3);
      graphs[iGraph]->GetListOfFunctions()->Add(lineUpperRangeBad);
      TLine* lineLowerRangeBad = new TLine(xMin, mExpectedPhysicsValue - mRangeBad, xMax, mExpectedPhysicsValue - mRangeBad);
      lineLowerRangeBad->SetLineColor(kBlack);
      lineLowerRangeBad->SetLineWidth(3);
      graphs[iGraph]->GetListOfFunctions()->Add(lineLowerRangeBad);
    }

    // Draw the mean+stdev each point got compared against
    if (mMeanCheck) {
      TGraph* meanGraph = new TGraph();
      TGraph* stddevGraphMediumUp = new TGraph();
      TGraph* stddevGraphMediumDown = new TGraph();
      TGraph* stddevGraphBadUp = new TGraph();
      TGraph* stddevGraphBadDown = new TGraph();

      const int nPoints = graphs[iGraph]->GetN();
      const double* yValues = graphs[iGraph]->GetY();
      const double* yErrors = graphs[iGraph]->GetEY(); // returns nullptr for TGraph (no errors)
      bool useErrors = true;
      if (yErrors == nullptr) {
        useErrors = false;
      }

      if (nPoints > 2) {
        for (int nBins = 1; nBins < nPoints; nBins++) {
          int pointNumberForMean = mPointToTakeForMeanCheck;
          if ((nBins) < mPointToTakeForMeanCheck) {
            pointNumberForMean = nBins;
          }

          double mean = 0.;
          double stdevMean = 0.;
          calculateStatistics(yValues, yErrors, useErrors, nBins - pointNumberForMean, nBins, mean, stdevMean);

          double lastPointError = 0.;
          if (useErrors) {
            lastPointError = *(yErrors + nBins); // Error of last point
          }
          if (lastPointError < 0.) {
            ILOG(Warning, Support) << "Last point for check of mean has negative error" << ENDM;
          }

          double stdev = sqrt(stdevMean * stdevMean + lastPointError * lastPointError);

          meanGraph->AddPoint(graphs[iGraph]->GetPointX(nBins), mean);
          stddevGraphMediumUp->AddPoint(graphs[iGraph]->GetPointX(nBins), mean + stdev * mNSigmaMean);
          stddevGraphMediumDown->AddPoint(graphs[iGraph]->GetPointX(nBins), mean - stdev * mNSigmaMean);
          stddevGraphBadUp->AddPoint(graphs[iGraph]->GetPointX(nBins), mean + stdev * mNSigmaBadMean);
          stddevGraphBadDown->AddPoint(graphs[iGraph]->GetPointX(nBins), mean - stdev * mNSigmaBadMean);
        } // for (int nBins = 1; nBins < nPoints; nBins++)

        meanGraph->SetLineWidth(2);
        meanGraph->SetLineColor(kGreen - 2);
        meanGraph->SetMarkerColor(kGreen - 2);

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

        graphs[iGraph]->GetListOfFunctions()->Add(meanGraph);
        graphs[iGraph]->GetListOfFunctions()->Add(stddevGraphMediumUp);
        graphs[iGraph]->GetListOfFunctions()->Add(stddevGraphMediumDown);
        graphs[iGraph]->GetListOfFunctions()->Add(stddevGraphBadUp);
        graphs[iGraph]->GetListOfFunctions()->Add(stddevGraphBadDown);
      }
    }

    // InfoBox
    std::string checkMessage;
    TPaveText* msg = new TPaveText(0.5, 0.75, 0.9, 0.9, "NDC");
    msg->SetName(Form("%s_msg_%d", mo->GetName(), iGraph));
    graphs[iGraph]->GetListOfFunctions()->Add(msg);

    if (mPadQualities[iGraph] == Quality::Good) {
      graphs[iGraph]->SetFillColor(kGreen - 2);
      msg->Clear();
      msg->AddText("Quality::Good");
      msg->SetFillColor(kGreen - 2);
    } else if (mPadQualities[iGraph] == Quality::Bad) {
      graphs[iGraph]->SetFillColor(kRed);
      msg->Clear();
      msg->AddText("Quality::Bad. Failed checks:");
      checkMessage = mPadMetaData[Quality::Bad.getName()][iGraph];
      msg->SetFillColor(kRed);
    } else if (mPadQualities[iGraph] == Quality::Medium) {
      graphs[iGraph]->SetFillColor(kOrange);
      msg->Clear();
      msg->AddText("Quality::Medium. Failed checks:");
      checkMessage = mPadMetaData[Quality::Medium.getName()][iGraph];
      msg->SetFillColor(kOrange);
    } else if (mPadQualities[iGraph] == Quality::Null) {
      graphs[iGraph]->SetFillColor(0);
      msg->AddText("Quality::Null. Failed checks:");
      checkMessage = mPadMetaData[Quality::Null.getName()][iGraph];
    }

    // Split lines by hand as \n does not work with TPaveText
    std::string delimiter = "\n";
    size_t pos = 0;
    std::string subText;
    while ((pos = checkMessage.find(delimiter)) != std::string::npos) {
      subText = checkMessage.substr(0, pos);
      msg->AddText(subText.c_str());
      checkMessage.erase(0, pos + delimiter.length());
    }
    msg->AddText(checkResult.getMetadata("Comment", "").c_str());
  } // for(int iGraph = 0; iGraph < graphs.size(); iGraph++)
}

void CheckOfTrendings::getGraphs(TCanvas* canv, std::vector<TGraph*>& graphs)
{
  if (mSliceTrend) {
    TList* padList = (TList*)canv->GetListOfPrimitives();
    padList->SetOwner(kTRUE);
    const int numberPads = padList->GetEntries();
    for (int iPad = 0; iPad < numberPads; iPad++) {
      auto pad = static_cast<TPad*>(padList->At(iPad));
      graphs.push_back(static_cast<TGraph*>(pad->GetPrimitive("Graph")));
    }
  } else {
    // If we have a standard trending with a TGraphErrors, then there will be a TGraph and a TGraphErrors with the same name ("Graph")
    // with the current configuration the TGraphErrors will always be added after the TGraph
    // loop from the back and find the last element with the name "Graph"
    TGraph* g;
    int jList = canv->GetListOfPrimitives()->LastIndex();
    for (; jList > 0; jList--) {
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
    graphs.push_back(g);
  }
}

void CheckOfTrendings::calculateStatistics(const double* yValues, const double* yErrors, bool useErrors, const int firstPoint, const int lastPoint, double& mean, double& stddevOfMean)
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
      stddevOfMean = 0.;
    } else {
      stddevOfMean = sqrt(1. / sumOfWeights);
    }
  } else { // for >= 2 points, we calculate the spread
    if (!useErrors) {
      std::vector<double> diff(v.size());
      std::transform(v.begin(), v.end(), diff.begin(), [mean](double x) { return x - mean; });
      double sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
      stddevOfMean = std::sqrt(sq_sum / (v.size() * (v.size() - 1.)));
    } else {
      double ratioSumWeight = sumOfSquaredWeights / (sumOfWeights * sumOfWeights);
      stddevOfMean = sqrt((sumSquare / sumOfWeights - mean * mean) * (1. / (1. - ratioSumWeight)) * ratioSumWeight);
    }
  }
}

std::string CheckOfTrendings::createMetaData(std::vector<std::string> pointMetaData)
{

  std::string totalString = "";

  if (pointMetaData.size() > 1) {
    std::string meanString = "";
    std::string expectedValueString = "";
    std::string rangeString = "";
    std::string zeroString = "";

    for (int i; i < pointMetaData.size(); i++) {
      if (pointMetaData.at(i).find("MeanCheck") != std::string::npos) {
        meanString += " " + std::to_string(i) + ",";
      }
      if (pointMetaData.at(i).find("ExpectedValueCheck") != std::string::npos) {
        expectedValueString += " " + std::to_string(i) + ",";
      }
      if (pointMetaData.at(i).find("RangeCheck") != std::string::npos) {
        rangeString += " " + std::to_string(i) + ",";
      }
      if (pointMetaData.at(i).find("ZeroCheck") != std::string::npos) {
        zeroString += " " + std::to_string(i) + ",";
      }
    }

    if (meanString != "") {
      meanString.pop_back();
      meanString = "MeanCheck for Graphs:" + meanString + "\n";
      totalString += meanString;
    }
    if (expectedValueString != "") {
      expectedValueString.pop_back();
      expectedValueString = "ExpectedValueCheck for Graphs:" + expectedValueString + "\n";
      totalString += expectedValueString;
    }
    if (rangeString != "") {
      rangeString.pop_back();
      rangeString = "RangeCheck for Graphs:" + rangeString + "\n";
      totalString += rangeString;
    }
    if (zeroString != "") {
      zeroString.pop_back();
      zeroString = "ZeroCheck for Graphs:" + zeroString + "\n";
      totalString += zeroString;
    }
  } else {
    totalString = pointMetaData[0];
  }

  return totalString;
}

} // namespace o2::quality_control_modules::tpc