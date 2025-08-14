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
/// \file   MIPCheck.cxx
/// \author andreas.molander@cern.ch
///

#include "FIT/MIPCheck.h"
#include "FITCommon/HelperCommon.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"

// ROOT
#include <Rtypes.h>
#include <TAttLine.h>
#include <TF1.h>
#include <TFitResult.h>
#include <TFitResultPtr.h>
#include <TH1.h>
#include <TLine.h>
#include <TMath.h>
#include <TPaveText.h>

#include <DataFormatsQualityControl/FlagType.h>
#include <DataFormatsQualityControl/FlagTypeFactory.h>

#include <string>

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::fit
{

void MIPCheck::configure()
{
  // Read configuration parameters

  mNameObjectToCheck = mCustomParameters.atOrDefaultValue("nameObjectToCheck", "");
  if (mNameObjectToCheck.empty()) {
    ILOG(Error, Support) << "No MO name provided to check. The check will not do anything. Please provide a valid MO name." << ENDM;
  }
  mNPeaksToFit = std::stoi(mCustomParameters.atOrDefaultValue("nPeaksToFit", "2"));
  mFitRangeLow = std::stof(mCustomParameters.atOrDefaultValue("fitRangeLow", "11.0"));
  mFitRangeHigh = std::stof(mCustomParameters.atOrDefaultValue("fitRangeHigh", "35.0"));

  std::string parseableParam;

  parseableParam = mCustomParameters.atOrDefaultValue("gausParamsMean", "");
  mGausParamsMeans = helper::parseParameters<float>(parseableParam, ",");
  if (mGausParamsMeans.size() != mNPeaksToFit) {
    ILOG(Warning, Support) << "Initial fit arguments for gaussian means not provided for all MIP peaks. Trying to estimate them..." << ENDM;
  }

  parseableParam = mCustomParameters.atOrDefaultValue("gausParamsSigma", "3.0, 7.0");
  mGausParamsSigmas = helper::parseParameters<float>(parseableParam, ",");

  parseableParam = mCustomParameters.atOrDefaultValue("meanWarningsLow", "");
  mMeanWarningsLow = helper::parseParameters<float>(parseableParam, ",");

  parseableParam = mCustomParameters.atOrDefaultValue("meanWarningsHigh", "");
  mMeanWarningsHigh = helper::parseParameters<float>(parseableParam, ",");

  parseableParam = mCustomParameters.atOrDefaultValue("meanErrorsLow", "");
  mMeanErrorsLow = helper::parseParameters<float>(parseableParam, ",");

  parseableParam = mCustomParameters.atOrDefaultValue("meanErrorsHigh", "");
  mMeanErrorsHigh = helper::parseParameters<float>(parseableParam, ",");

  parseableParam = mCustomParameters.atOrDefaultValue("sigmaWarnings", "");
  mSigmaWarnings = helper::parseParameters<float>(parseableParam, ",");

  parseableParam = mCustomParameters.atOrDefaultValue("sigmaErrors", "");
  mSigmaErrors = helper::parseParameters<float>(parseableParam, ",");

  parseableParam = mCustomParameters.atOrDefaultValue("drawMeanWarningsLow", "false, false");
  mDrawMeanWarningsLow = helper::parseParameters<bool>(parseableParam, ",");

  parseableParam = mCustomParameters.atOrDefaultValue("drawMeanWarningsHigh", "false, false");
  mDrawMeanWarningsHigh = helper::parseParameters<bool>(parseableParam, ",");

  parseableParam = mCustomParameters.atOrDefaultValue("drawMeanErrorsLow", "true, true");
  mDrawMeanErrorsLow = helper::parseParameters<bool>(parseableParam, ",");

  parseableParam = mCustomParameters.atOrDefaultValue("drawMeanErrorsHigh", "true, true");
  mDrawMeanErrorsHigh = helper::parseParameters<bool>(parseableParam, ",");

  parseableParam = mCustomParameters.atOrDefaultValue("drawSigmaWarnings", "false, false");
  mDrawSigmaWarnings = helper::parseParameters<bool>(parseableParam, ",");

  parseableParam = mCustomParameters.atOrDefaultValue("drawSigmaErrors", "false, false");
  mDrawSigmaErrors = helper::parseParameters<bool>(parseableParam, ",");

  parseableParam = mCustomParameters.atOrDefaultValue("labelPos", "0.15, 0.2, 0.85, 0.45");
  mVecLabelPos = helper::parseParameters<double>(parseableParam, ",");
  if (mVecLabelPos.size() != 4) {
    ILOG(Error, Devel) << "Incorrect label coordinates! Setting default." << ENDM;
    mVecLabelPos = { 0.15, 0.2, 0.85, 0.45 };
  }
}

Quality MIPCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {
    if (mo->getName() == mNameObjectToCheck) {
      auto* h = dynamic_cast<TH1*>(mo->getObject());
      if (h == nullptr) {
        ILOG(Error, Support) << "Could not cast " << mNameObjectToCheck << " to TH1*, skipping" << ENDM;
        continue;
      }
      // unless we find issues, we assume the quality is good
      result = Quality::Good;

      // Fit the histogram to find the MIP peaks

      // Gaussian fit function
      auto multiGausLambda = [this](Double_t* x, Double_t* par) -> Double_t {
        Double_t sum = 0;
        for (int i = 0; i < mNPeaksToFit; i++) {
          // a = par[i * 3]
          // mean = par[i * 3 + 1]
          // sigma = par[i * 3 + 2];
          sum += par[i * 3] * TMath::Gaus(x[0], par[i * 3 + 1], par[i * 3 + 2]);
        }
        return sum;
      };

      // Construct a TF1 using the lambda
      TF1 fitFunc("fitFunc", multiGausLambda, mFitRangeLow, mFitRangeHigh, 3 * mNPeaksToFit);

      // Provide initial guesses for the fit parameters
      double mean1 = -1;
      if (mGausParamsMeans.size() > 0) {
        mean1 = mGausParamsMeans.at(0);
      } else {
        mean1 = h->GetBinCenter(h->GetMaximumBin());
        ILOG(Info, Devel) << Form("No initial guess for the 1 MIP peak mean provided. Estimated: %f", mean1) << ENDM;
      }

      for (int i = 0; i < mNPeaksToFit; i++) {
        double mean = -1;
        if (i == 0) {
          mean = mean1;
        } else if (mGausParamsMeans.size() > i) {
          mean = mGausParamsMeans.at(i);
        } else {
          mean = mean1 * (i + 1);
          ILOG(Info, Devel) << Form("No initial guess for the %i MIP peak mean provided. Estimated: %f", i + 1, mean) << ENDM;
        }

        int meanBin = h->FindBin(mean);
        double a = h->GetBinContent(meanBin);
        double sigma = mGausParamsSigmas.size() > i ? mGausParamsSigmas.at(i) : 5.0; // Use 5.0 for sigmas not provided

        ILOG(Info, Devel) << "Peak " << i << " initial fit parameters: a: " << a << ", mean: " << mean << ", sigma: " << sigma << ENDM;

        fitFunc.SetParameter(i * 3, a);
        fitFunc.SetParameter(i * 3 + 1, mean);
        fitFunc.SetParameter(i * 3 + 2, sigma);
      }

      TFitResultPtr fitResult = h->Fit(&fitFunc, "SR");

      // Check if the fit was successful
      if (fitResult->Status() != 0) {
        ILOG(Error, Devel) << "Fit failed for histogram " << moName << ", status: " << fitResult->Status() << ENDM;
        result = Quality::Bad;
        result.addFlag(FlagTypeFactory::UnknownQuality(), "Fit failed");
        break; // Don't bother checking the thresholds if the fit failed
      }

      // Check the threholds, if some are not specified in the config, simply don't check them
      for (int i = 0; i < mNPeaksToFit; i++) {
        double mean = fitFunc.GetParameter(i * 3 + 1);
        double sigma = fitFunc.GetParameter(i * 3 + 2);

        bool meanBad = false;
        bool sigmaBad = false;

        // Check the error threholds

        // Mean
        if (mMeanErrorsHigh.size() > i && mMeanErrorsHigh.at(i) >= 0 && mean > mMeanErrorsHigh.at(i)) {
          result = Quality::Bad;
          result.addFlag(FlagTypeFactory::Unknown(), Form("%i MIP peak mean (%.2f) above upper error threshold (%.2f)", i + 1, mean, mMeanErrorsHigh.at(i)));
          meanBad = true;
        } else if (mMeanErrorsLow.size() > i && mMeanErrorsLow.at(i) >= 0 && mean < mMeanErrorsLow.at(i)) {
          result = Quality::Bad;
          result.addFlag(FlagTypeFactory::Unknown(), Form("%i MIP peak mean (%.2f) below lower error threshold (%.2f)", i + 1, mean, mMeanErrorsLow.at(i)));
          meanBad = true;
        }

        // Sigma
        if (mSigmaErrors.size() > i && mSigmaErrors.at(i) >= 0 && sigma > mSigmaErrors.at(i)) {
          result = Quality::Bad;
          result.addFlag(FlagTypeFactory::Unknown(), Form("%i MIP peak sigma (%.2f) larger than error threshold (%.2f)", i + 1, sigma, mSigmaErrors.at(i)));
          sigmaBad = true;
        }

        // Check the warning thresholds

        // Mean (no need to check in case the mean already crossed an error threshold)
        if (!meanBad && mMeanWarningsHigh.size() > i && mMeanWarningsHigh.at(i) >= 0 && mean > mMeanWarningsHigh.at(i)) {
          if (result.isBetterThan(Quality::Medium)) { // Don't make the quality better than it was
            result.set(Quality::Medium);
          }
          // Add a flag even if the quality was bad because of another peak
          result.addFlag(FlagTypeFactory::Unknown(), Form("%i MIP peak mean (%.2f) above upper warning threshold (%.2f)", i + 1, mean, mMeanWarningsHigh.at(i)));
        } else if (!meanBad && mMeanWarningsLow.size() > i && mMeanWarningsLow.at(i) >= 0 && mean < mMeanWarningsLow.at(i)) {
          if (result.isBetterThan(Quality::Medium)) { // Don't make the quality better than it was
            result.set(Quality::Medium);
          }
          // Add a flag even if the quality was bad because of another peak
          result.addFlag(FlagTypeFactory::Unknown(), Form("%i MIP peak mean (%.2f) below lower warning threshold (%.2f)", i + 1, mean, mMeanWarningsLow.at(i)));
        }

        // Sigma (no need to check in case the sigma already crossed an error threshold)
        if (!sigmaBad && mSigmaWarnings.size() > i && mSigmaWarnings.at(i) >= 0 && sigma > mSigmaWarnings.at(i)) {
          if (result.isBetterThan(Quality::Medium)) { // Don't make the quality better than it was
            result.set(Quality::Medium);
          }
          result.addFlag(FlagTypeFactory::Unknown(), Form("%i MIP peak sigma (%.2f) larger than warning threshold (%.2f)", i + 1, sigma, mSigmaWarnings.at(i)));
        }
      }

      beautify(mo, result);
    }
  }

  return result;
}

void MIPCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == mNameObjectToCheck) {
    auto* h = dynamic_cast<TH1*>(mo->getObject());
    if (h == nullptr) {
      ILOG(Error, Support) << "Could not cast `example` to TH1*, skipping" << ENDM;
      return;
    }

    TPaveText* msg = new TPaveText(mVecLabelPos[0], mVecLabelPos[1], mVecLabelPos[2], mVecLabelPos[3], "NDC");

    std::vector<double> meanValues;
    std::vector<double> sigmaValues;

    auto* func = h->GetFunction("fitFunc");
    if (func == nullptr) {
      ILOG(Error, Support) << "Could not find fit function in histogram " << mo->getName() << ENDM;
      msg->AddText("Gaussian fit not found!");
    } else {
      msg->AddText("Gaussian fit");
      func->SetLineColor(kBlack);

      for (int i = 0; i < mNPeaksToFit; i++) {
        double mean = func->GetParameter(i * 3 + 1);
        double sigma = func->GetParameter(i * 3 + 2);
        meanValues.push_back(mean);
        sigmaValues.push_back(sigma);

        msg->AddText(Form("%i MIP: #mu = %.2f, #sigma = %.2f", i + 1, mean, sigma));
      }
    }

    if (checkResult == Quality::Good) {
      msg->AddText(">> Quality::Good <<");
      msg->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      msg->SetFillColor(kRed);
      msg->AddText(">> Quality::Bad <<");
    } else if (checkResult == Quality::Medium) {
      msg->SetFillColor(kOrange);
      msg->AddText(">> Quality::Medium <<");
    } else if (checkResult == Quality::Null) {
      msg->AddText(">> Quality::Null <<");
      msg->SetFillColor(kGray);
    }

    for (const auto& [flag, comment] : checkResult.getFlags()) {
      if (comment.empty()) {
        msg->AddText(Form("#rightarrow Flag: %s", flag.getName().c_str()));
      } else {
        msg->AddText(Form("#rightarrow Flag: %s: %s", flag.getName().c_str(), comment.c_str()));
      }
    }

    h->GetListOfFunctions()->Add(msg);

    // Add threshold lines

    for (int i = 0; i < mNPeaksToFit; i++) {
      // Warning low
      if (mMeanWarningsLow.size() > i && mMeanWarningsLow.at(i) >= 0 && mDrawMeanWarningsLow.size() > i && mDrawMeanWarningsLow.at(i)) {
        double meanWarningLow = mMeanWarningsLow.at(i);
        auto* line = new TLine(meanWarningLow, 0, meanWarningLow, h->GetMaximum());
        line->SetLineColor(kOrange);
        line->SetLineStyle(kDashed);
        h->GetListOfFunctions()->Add(line);
      }

      // Warning high
      if (mMeanWarningsHigh.size() > i && mMeanWarningsHigh.at(i) >= 0 && mDrawMeanWarningsHigh.size() > i && mDrawMeanWarningsHigh.at(i)) {
        double meanWarningHigh = mMeanWarningsHigh.at(i);
        auto* line = new TLine(meanWarningHigh, 0, meanWarningHigh, h->GetMaximum());
        line->SetLineColor(kOrange);
        line->SetLineStyle(kDashed);
        h->GetListOfFunctions()->Add(line);
      }

      // Error low
      if (mMeanErrorsLow.size() > i && mMeanErrorsLow.at(i) >= 0 && mDrawMeanErrorsLow.size() > i && mDrawMeanErrorsLow.at(i)) {
        double meanErrorLow = mMeanErrorsLow.at(i);
        auto* line = new TLine(meanErrorLow, 0, meanErrorLow, h->GetMaximum());
        line->SetLineColor(kRed);
        line->SetLineStyle(kDashed);
        h->GetListOfFunctions()->Add(line);
      }

      // Error high
      if (mMeanErrorsHigh.size() > i && mMeanErrorsHigh.at(i) >= 0 && mDrawMeanErrorsHigh.size() > i && mDrawMeanErrorsHigh.at(i)) {
        double meanErrorHigh = mMeanErrorsHigh.at(i);
        auto* line = new TLine(meanErrorHigh, 0, meanErrorHigh, h->GetMaximum());
        line->SetLineColor(kRed);
        line->SetLineStyle(kDashed);
        h->GetListOfFunctions()->Add(line);
      }

      // Sigma warning
      if (mSigmaWarnings.size() > i && mSigmaWarnings.at(i) >= 0 && mDrawSigmaWarnings.size() > i && mDrawSigmaWarnings.at(i) && meanValues.size() > i) {
        double sigmaWarningLow = meanValues.at(i) - mSigmaWarnings.at(i);
        auto* lineLow = new TLine(sigmaWarningLow, 0, sigmaWarningLow, h->GetMaximum());
        lineLow->SetLineColor(kOrange);
        lineLow->SetLineStyle(kDotted);
        h->GetListOfFunctions()->Add(lineLow);
        double sigmaWarningHigh = meanValues.at(i) + mSigmaWarnings.at(i);
        auto* lineHigh = new TLine(sigmaWarningHigh, 0, sigmaWarningHigh, h->GetMaximum());
        lineLow->SetLineColor(kOrange);
        lineLow->SetLineStyle(kDotted);
        h->GetListOfFunctions()->Add(lineLow);
      }

      // Sigma error
      if (mSigmaErrors.size() > i && mSigmaErrors.at(i) >= 0 && mDrawSigmaErrors.size() > i && mDrawSigmaErrors.at(i) && meanValues.size() > i) {
        double sigmaErrorLow = meanValues.at(i) - mSigmaErrors.at(i);
        auto* lineLow = new TLine(sigmaErrorLow, 0, sigmaErrorLow, h->GetMaximum());
        lineLow->SetLineColor(kRed);
        lineLow->SetLineStyle(kDotted);
        h->GetListOfFunctions()->Add(lineLow);
        double sigmaErrorHigh = meanValues.at(i) + mSigmaErrors.at(i);
        auto* lineHigh = new TLine(sigmaErrorHigh, 0, sigmaErrorHigh, h->GetMaximum());
        lineHigh->SetLineColor(kRed);
        lineHigh->SetLineStyle(kDotted);
        h->GetListOfFunctions()->Add(lineHigh);
      }
    }
  }
}

void MIPCheck::startOfActivity(const Activity& activity)
{
  mActivity = make_shared<Activity>(activity);
}

} // namespace o2::quality_control_modules::fit
