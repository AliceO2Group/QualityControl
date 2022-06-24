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
// ROOT
#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <THn.h>
#include <TPaveText.h>
#include <TLine.h>

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::tpc
{

void GenericHistogramCheck::configure()
{
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
  Quality result = Quality::Null;
  for (auto const& moObj : *moMap) {
    auto mo = moObj.second;
    if (!mo) {
      continue;
      ILOG(Error, Support) << "No MO found" << ENDM;
    }
    const auto moName = mo->getName();

    THn* hN = THn::CreateHn(moName.c_str(), moName.c_str(), (TH2F*)mo->getObject());
    mHistDimension = hN->GetNdimensions();
    delete hN;
    hN = nullptr;
    
    if (mHistDimension == 1) {
      if (!mCheckXAxis) {
        ILOG(Error, Support) << "a 1D Histogram was given, but the X-axis is not assigned to be checked. No Check was performed." << ENDM;
        mMeanX = 999999999; // set it to some number so that the math does not break
        mStdevX = 999999999;
      } else {
        TH1D* hNProjX = (TH1D*)mo->getObject();
        hNProjX->ResetStats();
        mMeanX = hNProjX->GetMean(1);
        mStdevX = hNProjX->GetStdDev(1);
      }
    } else if (mHistDimension == 2) {
      TH2D* h2d = (TH2D*)mo->getObject();
      if (mCheckXAxis) {
        mMeanX = h2d->GetMean(1);
        mStdevX = h2d->GetStdDev(1);
      }
      if (mCheckYAxis) {
        mMeanY = h2d->GetMean(2);
        mStdevY = h2d->GetStdDev(2);
      }

    } else {
      ILOG(Warning, Support) << "This check only supports 1 and 2 dimensional histograms." << ENDM;
      // Brick.
    }

    // now we havve the mean
    // calculate quality
    // initialize result

    Quality resultRangeX = Quality::Null;
    Quality resultStdDevX = Quality::Null;
    Quality resultRangeY = Quality::Null;
    Quality resultStdDevY = Quality::Null;
    if (mCheckXAxis) {
      // Check for a range around an expected Value
      if (mCheckRange) {
        if (std::abs(mMeanX - mExpectedValueX) < mRangeX) {
          resultRangeX = Quality::Good;
        } else if (std::abs(mMeanX - mExpectedValueX) > 2 * mRangeX) {
          resultRangeX = Quality::Bad;
        } else {
          resultRangeX = Quality::Medium;
        }
      }
      // Check for 3/6 StdDev around expected value
      if (mCheckStdDev) {
        if (std::abs(mMeanX - mExpectedValueX) < 3 * mStdevX) {
          resultStdDevX = Quality::Good;
        } else if (std::abs(mMeanX - mExpectedValueX) > 6 * mStdevX) {
          resultStdDevX = Quality::Bad;
        } else {
          resultStdDevX = Quality::Medium;
        }
      }
    }
    if (mCheckYAxis) {
      if (mHistDimension == 2) {
        // Check for a range around an expected Value
        if (mCheckRange) {
          if (std::abs(mMeanY - mExpectedValueY) < mRangeY) {
            resultRangeY = Quality::Good;
          } else if (std::abs(mMeanY - mExpectedValueY) > 2 * mRangeY) {

            resultRangeY = Quality::Bad;
          } else {
            resultRangeY = Quality::Medium;
          }
        }
        // Check for 3/6 StdDev around expected value
        if (mCheckStdDev) {
          if (std::abs(mMeanY - mExpectedValueY) < 3 * mStdevY) {
            resultStdDevY = Quality::Good;
          } else if (std::abs(mMeanY - mExpectedValueY) > 6 * mStdevY) {
            resultStdDevY = Quality::Bad;
          } else {
            resultStdDevY = Quality::Medium;
          }
        }
      }
    }
    // find the worst quality that is NOT 10 ("Null")
    std::vector<Quality> quals = { resultRangeX, resultRangeY, resultStdDevX, resultStdDevY };
    std::sort(quals.begin(), quals.end(), [](Quality a, Quality b) { return a.isWorseThan(b); });
    auto result_iter = std::lower_bound(quals.begin(), quals.end(), Quality::Bad, [](Quality a, Quality b) { return a.isWorseThan(b); });
    if (result_iter != quals.end()) {
      result = quals[std::distance(quals.begin(), result_iter)];
    } else {
      ILOG(Warning, Support) << "There is a problem with the Qualities. All Qualities might be Null. Is there a check requested?" << ENDM;
    }

  } // for Mo map

  return result;
}

std::string GenericHistogramCheck::getAcceptedType() { return "TCanvas"; }

void GenericHistogramCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  auto moName = mo->getName();
  TPaveText* msg = new TPaveText(0.11, 0.85, 0.9, 0.95, "NDC");
  msg->SetBorderSize(1);
  TText* txt = new TText(0, 0, "Quality::Null");
  TText* txt2 = new TText(0, 0, "Quality::Null");
  double xText = 0;
  double yText = 0;
  double yText2 = 0;
  if (mHistDimension == 1) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    TLine* lineX = new TLine(mMeanX, h->GetMinimum() * 1.1, mMeanX, h->GetMaximum() * 1.1);
    TLine* lineXEV = new TLine(mExpectedValueX, h->GetMinimum() * 1.1, mExpectedValueX, h->GetMaximum() * 1.1);
    lineX->SetLineWidth(3);
    lineX->SetLineColor(kRed);
    lineXEV->SetLineWidth(3);
    lineXEV->SetLineStyle(kDashed);

    h->GetListOfFunctions()->Add(lineX);
    h->GetListOfFunctions()->Add(lineXEV);
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));
    h->SetLineColor(kBlack);
  }
  if (mHistDimension == 2) {

    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    xText = h->GetXaxis()->GetXmin() + std::abs(h->GetXaxis()->GetXmax() - h->GetXaxis()->GetXmin()) * 0.01;
    yText = h->GetYaxis()->GetXmax() * 0.9;
    // if we need a second line, move it 5% down
    yText2 = h->GetYaxis()->GetXmax() * 0.9 - std::abs(h->GetYaxis()->GetXmax() - h->GetYaxis()->GetXmin()) * 0.05;
    if (!h) {

      ILOG(Warning, Support) << "h not found in D= 2" << ENDM;
    }
    TLine* lineX = new TLine(mMeanX, h->GetYaxis()->GetXmin(), mMeanX, h->GetYaxis()->GetXmax());
    TLine* lineY = new TLine(h->GetXaxis()->GetXmin(), mMeanY, h->GetXaxis()->GetXmax(), mMeanY);
    TLine* lineXEV = new TLine(mExpectedValueX, h->GetYaxis()->GetXmin(), mExpectedValueX, h->GetYaxis()->GetXmax());
    TLine* lineYEV = new TLine(h->GetXaxis()->GetXmin(), mExpectedValueY, h->GetXaxis()->GetXmax(), mExpectedValueY);

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

    h->GetListOfFunctions()->Add(lineX);
    h->GetListOfFunctions()->Add(lineY);
    h->GetListOfFunctions()->Add(lineXEV);
    h->GetListOfFunctions()->Add(lineYEV);
    h->GetListOfFunctions()->Add(msg);
    h->GetListOfFunctions()->Add(txt);
    h->GetListOfFunctions()->Add(txt2);

    msg->SetName(Form("%s_msg", mo->GetName()));
    h->SetLineColor(kBlack);
  }

  if (checkResult == Quality::Good) {
    msg->Clear();
    msg->AddText("Quality::Good");
    msg->SetFillColor(kGreen);
    if (mCheckXAxis) {
      if (mCheckYAxis && mHistDimension == 2) {

        txt->SetText(xText, yText, fmt::format("Quality::Medium, MeanX: {:.3}, ExpectedX: {:.3}", mMeanX, mExpectedValueX).data());
        txt2->SetText(xText, yText2, fmt::format("MeanY: {:.3}, ExpectedY: {:.3}", mMeanY, mExpectedValueY).data());
      } else {
        txt->SetText(xText, yText, fmt::format("Quality::Good, MeanX: {:.3}, ExpectedX: {:.3}", mMeanX, mExpectedValueX).data());
      }
    } else {
      txt->SetText(xText, yText, fmt::format("Quality::Good, MeanY: {:.3}, ExpectedY: {:.3}", mMeanY, mExpectedValueY).data());
    }
    txt->SetTextColor(kGreen);
    txt2->SetTextColor(kGreen);
  } else if (checkResult == Quality::Bad) {

    msg->Clear();
    msg->AddText("Quality::Bad");
    // msg->AddText("Outlier, more than 6sigma.");
    msg->SetFillColor(kRed);
    if (mCheckXAxis) {
      if (mCheckYAxis && mHistDimension == 2) {

        txt->SetText(xText, yText, fmt::format("Quality::Bad, MeanX: {:.3}, ExpectedX: {:.3}", mMeanX, mExpectedValueX).data());
        txt2->SetText(xText, yText2, fmt::format("MeanY: {:.3}, ExpectedY: {:.3}", mMeanY, mExpectedValueY).data());
      } else {
        txt->SetText(xText, yText, fmt::format("Quality::Bad, MeanX: {:.3}, ExpectedX: {:.3}", mMeanX, mExpectedValueX).data());
      }
    } else {
      txt->SetText(xText, yText, fmt::format("Quality::Bad, MeanY: {:.3}, ExpectedY: {:.3}", mMeanY, mExpectedValueY).data());
    }
    txt->SetTextColor(kRed);
    txt2->SetTextColor(kRed);
  } else if (checkResult == Quality::Medium) {

    msg->Clear();
    msg->AddText("Quality::Medium");
    // msg->AddText("Outlier, more than 3sigma.");
    msg->SetFillColor(kOrange);
    if (mCheckXAxis) {
      if (mCheckYAxis && mHistDimension == 2) {
        txt->SetText(xText, yText, fmt::format("Quality::Medium, MeanX: {:.3}, ExpectedX: {:.3}", mMeanX, mExpectedValueX).data());
        txt2->SetText(xText, yText2, fmt::format("MeanY: {:.3}, ExpectedY: {:.3}", mMeanY, mExpectedValueY).data());
      } else {
        txt->SetText(xText, yText, fmt::format("Quality::Medium, MeanX: {:.3}, ExpectedX: {:.3}", mMeanX, mExpectedValueX).data());
      }
    } else {
      txt->SetText(xText, yText, fmt::format("Quality::Medium, MeanY: {:.3}, ExpectedY: {:.3}", mMeanY, mExpectedValueY).data());
    }
    txt->SetTextColor(kOrange);
    txt2->SetTextColor(kOrange);
  }
  if (checkResult == Quality::Null) {
    msg->AddText("No Check was performed.");
  } else {
    msg->AddText(fmt::format("X-Mean: {:.3}, Expected: {:.3}", mMeanX, mExpectedValueX).data());
    if (mHistDimension == 2) {
      msg->AddText(fmt::format("Y-Mean: {:.3}, Expected: {:.3}", mMeanY, mExpectedValueY).data());
    }
  }
  // msg->Draw("same");
}

} // namespace o2::quality_control_modules::tpc
