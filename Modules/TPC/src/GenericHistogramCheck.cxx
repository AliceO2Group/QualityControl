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
/// \file   GenericHistogramCheck.cxx
/// \author Maximilian Horst
///

#include "TPC/GenericHistogramCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include "Common/Utils.h"

// ROOT
#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TPaveText.h>
#include <TLine.h>

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::tpc
{
Quality CheckQuality(double Mean, double Comparison, double Offset, double nMed, double nBad, std::string& message)
{
  message = "";
  Quality result = Quality::Null;
  double deviation = std::abs(Mean - Comparison);

  if (deviation < nMed * Offset) {
    result = Quality::Good;
  } else if (deviation > nBad * Offset) {
    result = Quality::Bad;
  } else {
    result = Quality::Medium;
  }
  message = "Deviation to expected Value: " + std::to_string(deviation);

  return result;
}
void GenericHistogramCheck::configure()
{
  mMetadataComment = common::getFromConfig<std::string>(mCustomParameters, "MetadataComment", "");

  // ILOG(Warning, Support) << "Config started....?" << ENDM;
  if (const auto param = mCustomParameters.find("checks"); param != mCustomParameters.end()) {
    const std::string checkString = param->second.c_str();
    if (const auto param1 = checkString.find("Range"); param1 != string::npos) {
      mCheckRange = true;
    }
    if (const auto param1 = checkString.find("StdDev"); param1 != string::npos) {
      mCheckStdDev = true;
    }
    if (!mCheckRange && !mCheckStdDev) {

      mCheckStdDev = true;
      ILOG(Warning, Support) << "The given value for check was not readable. The options are Range and StdDev. As a default StdDev was chosen." << ENDM;
    }
  } else {
    mCheckStdDev = true;
    ILOG(Warning, Support) << "No Check was given. The options are Range and StdDev. As a default StdDev was chosen." << ENDM;
  }

  if (const auto param = mCustomParameters.find("axis"); param != mCustomParameters.end()) {
    const std::string axisString = param->second.c_str();
    if (const auto param1 = axisString.find("X"); param1 != string::npos) {
      mCheckXAxis = true;
    }
    if (const auto param1 = axisString.find("Y"); param1 != string::npos) {
      mCheckYAxis = true;
    }
    if (!mCheckXAxis && !mCheckYAxis) {

      mCheckXAxis = true;
      ILOG(Warning, Support) << "The given value for axis was not readable. The options are X and Y. As a default X was chosen." << ENDM;
    }
  } else {
    mCheckStdDev = true;
    ILOG(Warning, Support) << "No axis was given. The options are X and Y. As a default X was chosen." << ENDM;
  }

  if (const auto param = mCustomParameters.find("ExpectedValueX"); param != mCustomParameters.end()) {
    mExpectedValueX = std::atof(param->second.c_str());
  } else {
    mExpectedValueX = 0;
    ILOG(Warning, Support) << "No ExpectedValueX was given. Please always give an expected value. As a default 0 was chosen. " << ENDM;
  }
  if (const auto param = mCustomParameters.find("ExpectedValueY"); param != mCustomParameters.end()) {
    mExpectedValueY = std::atof(param->second.c_str());
  } else {
    mExpectedValueY = 0;
    ILOG(Warning, Support) << "No ExpectedValueY was given. Please always give an expected value. As a default 0 was chosen. " << ENDM;
  }

  if (mCheckRange) {
    if (const auto param = mCustomParameters.find("RangeX"); param != mCustomParameters.end()) {
      mRangeX = std::atof(param->second.c_str());
    } else {
      mRangeX = 0.1 * mExpectedValueX;
      ILOG(Warning, Support) << "No RangeX was given even though it was requested as a check. As a default " << 0.1 * mExpectedValueX << " was chosen. " << ENDM;
    }
    if (const auto param = mCustomParameters.find("RangeY"); param != mCustomParameters.end()) {
      mRangeY = std::atof(param->second.c_str());
    } else {
      mRangeY = 0.1 * mExpectedValueY;
      ILOG(Warning, Support) << "No RangeY was given even though it was requested as a check. As a default " << 0.1 * mExpectedValueY << " was chosen. " << ENDM;
    }
  }
}
Quality GenericHistogramCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  std::vector<Quality> mosQuality;

  std::vector<std::string> checkMessage;
  std::string message;

  Quality result = Quality::Null;
  result.addMetadata("Comment", mMetadataComment);

  for (auto const& moObj : *moMap) {
    auto mo = moObj.second;
    if (!mo) {
      continue;
      ILOG(Error, Support) << "No MO found" << ENDM;
      result.addMetadata(Quality::Null.getName(), "No MO found");
      return result;
    }
    auto h = dynamic_cast<TH1*>(mo->getObject());
    if (!h) {
      ILOG(Error, Support) << "No Histogram found" << ENDM;
      result.addMetadata(Quality::Null.getName(), "No Histogram found");
      return result;
    }

    mHistDimension = h->GetDimension();
    if (mCheckXAxis) {
      mMeanX = h->GetMean(1);
      mStdevX = h->GetStdDev(1);
    } else if (mHistDimension == 1) {
      ILOG(Error, Support) << "a 1D Histogram was given, but the X-axis is not assigned to be checked. No Check was performed." << ENDM;
      mMeanX = 999999999; // set it to some number so that the math does not break
      mStdevX = 999999999;
    }
    if (mHistDimension == 2) {
      if (mCheckYAxis) {
        mMeanY = h->GetMean(2);
        mStdevY = h->GetStdDev(2);
      }
    } else if (mHistDimension != 1) {
      ILOG(Warning, Support) << "This check only supports 1 and 2 dimensional histograms." << ENDM;
      // Brick.
    }

    Quality resultRangeX = Quality::Null;
    Quality resultStdDevX = Quality::Null;
    Quality resultRangeY = Quality::Null;
    Quality resultStdDevY = Quality::Null;

    // make function:
    //  CheckQuality(Axis,mean,Compare,offset,nMed,nBad)

    if (mCheckXAxis) {
      if (mCheckRange) {
        resultRangeX = CheckQuality(mMeanX, mExpectedValueX, mRangeX, 1, 2, message);
        checkMessage.push_back("RangeCheck: " + message);
        mosQuality.push_back(resultRangeX);
      }
      if (mCheckStdDev) {
        resultStdDevX = CheckQuality(mMeanX, mExpectedValueX, mStdevX, 3, 6, message);
        checkMessage.push_back("StdDevCheck: " + message);
        mosQuality.push_back(resultStdDevX);
      }
    }
    if (mCheckYAxis) {
      if (mHistDimension == 2) {
        if (mCheckRange) {
          resultRangeY = CheckQuality(mMeanY, mExpectedValueY, mRangeY, 1, 2, message);
          checkMessage.push_back("RangeCheck: " + message);
          mosQuality.push_back(resultRangeY);
        }

        if (mCheckStdDev) {
          resultStdDevY = CheckQuality(mMeanX, mExpectedValueX, mStdevY, 3, 6, message);
          checkMessage.push_back("StdDevCheck: " + message);
          mosQuality.push_back(resultStdDevY);
        }
      }
    }
    // find the worst quality that is NOT ("Null")
    std::vector<Quality> quals = { resultRangeX, resultRangeY, resultStdDevX, resultStdDevY };
    std::sort(quals.begin(), quals.end(), [](Quality a, Quality b) { return a.isWorseThan(b); });
    auto result_iter = std::lower_bound(quals.begin(), quals.end(), Quality::Bad, [](Quality a, Quality b) { return a.isWorseThan(b); });
    if (result_iter != quals.end()) {
      result = quals[std::distance(quals.begin(), result_iter)];
    } else {
      ILOG(Warning, Support) << "There is a problem with the Qualities. All Qualities might be Null. Is there a check requested?" << ENDM;
    }

  } // for Mo map

  // For writing to metadata and drawing later
  mBadString = "";
  mMediumString = "";
  mGoodString = "";
  mNullString = "";

  // Aggregation of quality strings used for MO
  for (int qualityindex = 0; qualityindex < mosQuality.size(); qualityindex++) {
    Quality q = mosQuality.at(qualityindex);
    if (q == Quality::Bad) {
      mBadString = mBadString + checkMessage.at(qualityindex) + "\n";
    } else if (q == Quality::Medium) {
      mMediumString = mMediumString + checkMessage.at(qualityindex) + "\n";
    } else if (q == Quality::Good) {
      mGoodString = mGoodString + checkMessage.at(qualityindex) + "\n";
    } else {
      mNullString = mNullString + checkMessage.at(qualityindex) + "\n";
    }
  }
  mosQuality.clear();

  result.addMetadata(Quality::Bad.getName(), mBadString);
  result.addMetadata(Quality::Medium.getName(), mMediumString);
  result.addMetadata(Quality::Good.getName(), mGoodString);
  result.addMetadata(Quality::Null.getName(), mNullString);

  return result;
}



void GenericHistogramCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  TPaveText* msg = new TPaveText(0.11, 0.85, 0.9, 0.95, "NDC");
  msg->SetBorderSize(1);
  TText* txt = new TText(0, 0, "Quality::Null");
  TText* txt2 = new TText(0, 0, "Quality::Null");
  double xText = 0;
  double yText = 0;
  double yText2 = 0;

  if (mHistDimension == 1) {

    auto h1 = dynamic_cast<TH1F*>((mo->getObject()));
    if (!h1) {
      ILOG(Warning, Support) << "h1 not found in D= 1" << ENDM;
      return;
    }
    TLine* lineX = new TLine(mMeanX, h1->GetMinimum() * 1.1, mMeanX, h1->GetMaximum() * 1.1);
    TLine* lineXEV = new TLine(mExpectedValueX, h1->GetMinimum() * 1.1, mExpectedValueX, h1->GetMaximum() * 1.1);
    lineX->SetLineWidth(3);
    lineX->SetLineColor(kRed);
    lineXEV->SetLineWidth(3);
    lineXEV->SetLineStyle(kDashed);

    h1->GetListOfFunctions()->Add(lineX);
    h1->GetListOfFunctions()->Add(lineXEV);

    h1->SetLineColor(kBlack);

    msg->SetName(Form("%s_msg", mo->GetName()));

    h1->GetListOfFunctions()->Add(msg);
  }
  if (mHistDimension == 2) {

    auto h2 = dynamic_cast<TH2F*>((mo->getObject()));
    if (!h2) {
      ILOG(Warning, Support) << "h2 not found in D= 2" << ENDM;
      return;
    }
    xText = h2->GetXaxis()->GetXmin() + std::abs(h2->GetXaxis()->GetXmax() - h2->GetXaxis()->GetXmin()) * 0.01;
    yText = h2->GetYaxis()->GetXmax() * 0.9;
    yText2 = h2->GetYaxis()->GetXmax() * 0.9 - std::abs(h2->GetYaxis()->GetXmax() - h2->GetYaxis()->GetXmin()) * 0.05;

    TLine* lineX = new TLine(mMeanX, h2->GetYaxis()->GetXmin(), mMeanX, h2->GetYaxis()->GetXmax());
    TLine* lineY = new TLine(h2->GetXaxis()->GetXmin(), mMeanY, h2->GetXaxis()->GetXmax(), mMeanY);
    TLine* lineXEV = new TLine(mExpectedValueX, h2->GetYaxis()->GetXmin(), mExpectedValueX, h2->GetYaxis()->GetXmax());
    TLine* lineYEV = new TLine(h2->GetXaxis()->GetXmin(), mExpectedValueY, h2->GetXaxis()->GetXmax(), mExpectedValueY);

    lineY->SetLineWidth(3);
    lineY->SetLineColor(kOrange);
    lineYEV->SetLineWidth(3);
    lineYEV->SetLineColor(kOrange);
    lineYEV->SetLineStyle(kDashed);
    lineX->SetLineWidth(3);
    lineX->SetLineColor(kRed);
    lineXEV->SetLineWidth(3);
    lineXEV->SetLineColor(kRed);
    lineXEV->SetLineStyle(kDashed);

    h2->GetListOfFunctions()->Add(lineX);
    h2->GetListOfFunctions()->Add(lineY);
    h2->GetListOfFunctions()->Add(lineXEV);
    h2->GetListOfFunctions()->Add(lineYEV);
    h2->GetListOfFunctions()->Add(txt);
    h2->GetListOfFunctions()->Add(txt2);

    h2->SetLineColor(kBlack);

    msg->SetName(Form("%s_msg", mo->GetName()));

    h2->GetListOfFunctions()->Add(msg);

    // make generic text for the 2D
    if (mCheckXAxis && mHistDimension == 2) {
      if (mCheckYAxis) {
        txt->SetText(xText, yText, fmt::format("MeanX: {:.3}, ExpectedX: {:.3}", mMeanX, mExpectedValueX).data());
        txt2->SetText(xText, yText2, fmt::format("MeanY: {:.3}, ExpectedY: {:.3}", mMeanY, mExpectedValueY).data());
      } else {
        txt->SetText(xText, yText, fmt::format("MeanX: {:.3}, ExpectedX: {:.3}", mMeanX, mExpectedValueX).data());
      }
    } else {
      txt->SetText(xText, yText, fmt::format("MeanY: {:.3}, ExpectedY: {:.3}", mMeanY, mExpectedValueY).data());
    }
  }
  if (checkResult == Quality::Good) {
    msg->Clear();
    msg->AddText("Quality::Good");
    msg->AddText(checkResult.getMetadata(Quality::Good.getName(), "").c_str());
    msg->SetFillColor(kGreen);

    txt->SetTextColor(kGreen);
    txt2->SetTextColor(kGreen);
  } else if (checkResult == Quality::Bad) {

    msg->Clear();
    msg->AddText("Quality::Bad");
    msg->AddText(checkResult.getMetadata(Quality::Bad.getName(), "").c_str());

    msg->SetFillColor(kRed);

    txt->SetTextColor(kRed);
    txt2->SetTextColor(kRed);
  } else if (checkResult == Quality::Medium) {

    msg->Clear();
    msg->AddText("Quality::Medium");
    msg->AddText(checkResult.getMetadata(Quality::Medium.getName(), "").c_str());
    msg->SetFillColor(kOrange);

    txt->SetTextColor(kOrange);
    txt2->SetTextColor(kOrange);
  }
  if (checkResult == Quality::Null) {
    msg->AddText("No Check was performed.");
    msg->AddText(checkResult.getMetadata(Quality::Null.getName(), "").c_str());
  } else {
    msg->AddText(fmt::format("X-Mean: {:.3}, Expected: {:.3}", mMeanX, mExpectedValueX).data());
    msg->AddText(checkResult.getMetadata("Comment", "").c_str());
    if (mHistDimension == 2) {
      msg->AddText(fmt::format("Y-Mean: {:.3}, Expected: {:.3}", mMeanY, mExpectedValueY).data());
    }
  }
}

} // namespace o2::quality_control_modules::tpc
