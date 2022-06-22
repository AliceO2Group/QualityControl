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
/// \file   CheckOfPads.cxx
/// \author Laura Serksnyte, Maximilian Horst
///

#include "TPC/CheckOfPads.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"

// ROOT
#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TPad.h>
#include <TList.h>
#include <TPaveText.h>

#include <iostream>

namespace o2::quality_control_modules::tpc
{
void CheckOfPads::configure()
{
  if (auto param = mCustomParameters.find("mediumQualityPercentageOfWorkingPads"); param != mCustomParameters.end()) {
    mMediumQualityLimit = std::atof(param->second.c_str());
  } else {
    mMediumQualityLimit = 0.7;
    ILOG(Info, Support) << "Chosen check requires mediumQualityPercentageOfWorkingPads which is not given. Setting to default 0.7 ." << ENDM;
  }
  if (auto param = mCustomParameters.find("badQualityPercentageOfWorkingPads"); param != mCustomParameters.end()) {
    mBadQualityLimit = std::atof(param->second.c_str());
  } else {
    mBadQualityLimit = 0.3;
    ILOG(Info, Support) << "Chosen check requires badQualityPercentageOfWorkingPads which is not given. Setting to default 0.3 ." << ENDM;
  }
  // check if expected Value Check is wished for:
  if (auto param = mCustomParameters.find("CheckChoice"); param != mCustomParameters.end()) {
    std::string CheckChoiceString = param->second.c_str();
    if (size_t finder = CheckChoiceString.find("ExpectedValue"); finder != std::string::npos) {
      mExpectedValueCheck = true;
      mFoundCheck = true;
    }
    if (size_t finder = CheckChoiceString.find("Mean"); finder != std::string::npos) {
      mMeanCheck = true;
      mFoundCheck = true;
    }
    if (size_t finder = CheckChoiceString.find("Empty"); finder != std::string::npos) {
      mEmptyCheck = true;
      mFoundCheck = true;
    }
  }

  if (!mFoundCheck) {
    mMeanCheck = true;
    ILOG(Warning, Support) << "This Check requires a CheckChoice. There was no value or the given value is wrong or not readable. Chose between 'ExpectedValue'(Compare the pad mean to an expectedValue), 'Mean' (compare pad mean to global mean) and/or 'Empty' (check for a threshold of empty pads). As a default 'Mean' was selected." << ENDM;
  }
  // }

  if (mExpectedValueCheck) {
    // load expectedValue
    if (auto param = mCustomParameters.find("ExpectedValue"); param != mCustomParameters.end()) {
      mExpectedValue = std::atof(param->second.c_str());
      // ILOG(Warning, Support) << " Expected Value: " << mExpectedValue << ENDM;
    } else {
      mExpectedValue = 1.0;
      ILOG(Info, Support) << "Chosen check requires ExpectedValue which is not given. Setting to default 1 ." << ENDM;
    }
    // load expectedValue Sigma Medium
    if (auto param = mCustomParameters.find("ExpectedValueSigmaMedium"); param != mCustomParameters.end()) {
      mExpectedValueMediumSigmas = std::atof(param->second.c_str());
    } else {
      mExpectedValueMediumSigmas = 3.;
      ILOG(Info, Support) << "Chosen check requires ExpectedValueSigmaMedium which is not given. Setting to default 3 sigma ." << ENDM;
    }
    // load expectedValue Sigma Bad
    if (auto param = mCustomParameters.find("ExpectedValueSigmaBad"); param != mCustomParameters.end()) {
      mExpectedValueBadSigmas = std::atof(param->second.c_str());
    } else {
      mExpectedValueBadSigmas = 6.;
      ILOG(Info, Support) << "Chosen check requires ExpectedValueSigmaBad which is not given. Setting to default 6 sigma ." << ENDM;
    }
  }
  // Cehck if Mean comparison is wished for:
  if (mMeanCheck) {
    // load Mean Sigma Medium
    if (auto param = mCustomParameters.find("MeanSigmaMedium"); param != mCustomParameters.end()) {
      mMeanMediumSigmas = std::atof(param->second.c_str());
    } else {
      mMeanMediumSigmas = 3;
      ILOG(Info, Support) << "Chosen check requires MeanSigmaMedium which is not given. Setting to default 3 sigma ." << ENDM;
    }
    // load Mean Sigma Bad
    if (auto param = mCustomParameters.find("MeanSigmaBad"); param != mCustomParameters.end()) {
      mMeanBadSigmas = std::atof(param->second.c_str());
    } else {
      mMeanBadSigmas = 6;
      ILOG(Info, Support) << "Chosen check requires MeanSigmaBad which is not given. Setting to default 6 sigma ." << ENDM;
    }
  }

  if (auto param = mCustomParameters.find("MOsNames2D"); param != mCustomParameters.end()) {
    auto temp = param->second.c_str();
    std::istringstream ss(temp);
    std::string token;
    while (std::getline(ss, token, ',')) {
      mMOsToCheck2D.emplace_back(token);
    }
  }
}

//______________________________________________________________________________
Quality CheckOfPads::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result_EV = Quality::Null;
  Quality result_Mean = Quality::Null;
  Quality result_Global = Quality::Null;
  Quality result_Empty = Quality::Null;
  for (auto const& moObj : *moMap) {
    auto mo = moObj.second;
    if (!mo) {
      continue;
    }
    auto moName = mo->getName();
    std::string histName, histNameS;
    if (auto it = std::find(mMOsToCheck2D.begin(), mMOsToCheck2D.end(), moName); it != mMOsToCheck2D.end()) {
      size_t end = moName.find("_2D");
      auto histSubName = moName.substr(7, end - 7);
      result_EV = Quality::Good;
      result_Mean = Quality::Good;

      auto* canv = (TCanvas*)mo->getObject();
      if (!canv)
        continue;
      // Check all histograms in the canvas

      mTotalMean = 0.;

      for (int tpads = 1; tpads <= 72; tpads++) {
        const auto padName = fmt::format("{:s}_{:d}", moName, tpads);
        const auto histName = fmt::format("h_{:s}_ROC_{:02d}", histSubName, tpads - 1);
        TPad* pad = (TPad*)canv->GetListOfPrimitives()->FindObject(padName.data());
        if (!pad) {
          mSectorsName.push_back("notitle");
          mSectorsQuality_EV.push_back(Quality::Null);
          continue;
        }
        TH2F* h = (TH2F*)pad->GetListOfPrimitives()->FindObject(histName.data());
        if (!h) {
          mSectorsName.push_back("notitle");
          mSectorsQuality_EV.push_back(Quality::Null);
          continue;
        }
        const std::string titleh = h->GetTitle();

        mSectorsName.push_back(titleh);
        // check if we are dealing with IROC or OROC
        float totalPads = 0;
        const int MaximumXBin = h->GetNbinsX();
        const int MaximumYBin = h->GetNbinsY();
        if (titleh.find("IROC") != std::string::npos) {
          totalPads = 5280;
          // MaximumXBin = 62;
          // MaximumYBin = 102;
        } else if (titleh.find("OROC") != std::string::npos) {
          totalPads = 9280;
          // MaximumXBin = 88;
          // MaximumYBin = 140;
        } else {
          return Quality::Null;
        }

        float PadSum = 0.;
        int PadsCount = 0;
        float PadStdev = 0.;
        float PadMean = 0.;

        // Run twice to get the mean and the standard deviation
        for (int RunNo = 1; RunNo <= 2; RunNo++) {
          // Run1: calculate single Pad total->Mean
          // Run2: calculate standardDeviation from mean
          for (int xBin = 1; xBin <= MaximumXBin; xBin++) {
            for (int yBin = 1; yBin <= MaximumYBin; yBin++) {
              float Binvalue = h->GetBinContent(xBin, yBin);
              if (Binvalue != 0) {
                if (RunNo == 1) {
                  PadSum += Binvalue;
                  PadsCount++;
                } else {
                  PadStdev += pow(Binvalue - PadMean, 2);
                }
              }
            }
          }

          if (RunNo == 1) {
            PadMean = PadSum / PadsCount;
          }
        }

        if (mEmptyCheck) {
          if (PadsCount > mMediumQualityLimit * totalPads) {
            result_Empty = Quality::Good;
          } else if (PadsCount < mBadQualityLimit * totalPads) {
            result_Empty = Quality::Bad;
          } else {
            result_Empty = Quality::Medium;
          }
          mSectorsQuality_Empty.push_back(result_Empty);
          mEmptyPadPercent.push_back(1. - (float)PadsCount / totalPads);
        }

        PadStdev = sqrt(PadStdev / (PadsCount - 1));
        mPadMeans.push_back(PadMean);
        mPadStdev.push_back(PadStdev);
      }
      float SumOfWeights = 0.;
      for (size_t it = 0; it < mPadMeans.size(); it++) { // loop over all pads
        // calculate the total mean and standard deviation
        mTotalMean += mPadMeans[it] / mPadStdev[it];
        SumOfWeights += 1 / mPadStdev[it];
      }
      mTotalStdev = sqrt(1 / SumOfWeights); // standard deviation of the weighted average.
      mTotalMean /= SumOfWeights;           // Weighted average (by standard deviation) of the total mean
      // calculate the Qualities:

      for (size_t it = 0; it < mPadMeans.size(); it++) { // loop over all pads

        if (mExpectedValueCheck) {

          if (fabs(mPadMeans[it] - mExpectedValue) < mPadStdev[it] * mExpectedValueMediumSigmas) {
            result_EV = Quality::Good;
          } else if (fabs(mPadMeans[it] - mExpectedValue) >= mPadStdev[it] * mExpectedValueMediumSigmas && fabs(mPadMeans[it] - mExpectedValue) < mPadStdev[it] * mExpectedValueBadSigmas) {
            result_EV = Quality::Medium;
          } else {
            result_EV = Quality::Bad;
          }
        }

        mSectorsQuality_EV.push_back(result_EV);
        if (mMeanCheck) {

          if (fabs(mPadMeans[it] - mTotalMean) < mPadStdev[it] * mMeanMediumSigmas) {
            result_Mean = Quality::Good;
          } else if (fabs(mPadMeans[it] - mTotalMean) >= mPadStdev[it] * mMeanMediumSigmas && fabs(mPadMeans[it] - mTotalMean) < mPadStdev[it] * mMeanBadSigmas) {
            result_Mean = Quality::Medium;
          } else {
            result_Mean = Quality::Bad;
          }
        }

        mSectorsQuality_Mean.push_back(result_Mean);

        std::vector<Quality> Qs;
        if (mMeanCheck) {
          Qs.push_back(result_Mean);
        }

        if (mExpectedValueCheck) {
          Qs.push_back(result_EV);
        }

        if (mEmptyCheck) {
          Qs.push_back(mSectorsQuality_Empty[it]);
        }
        if (Qs.size() > 0) {
          Quality Worst = Quality::Good;
          for (int Quality = 0; Quality < Qs.size(); Quality++) {
            if (Qs[Quality].isWorseThan(Worst)) {
              Worst = Qs[Quality];
            }
          }
          mSectorsQuality.push_back(Worst);
        } else {
          ILOG(Error, Support) << "No Qualities were found!" << ENDM;
        }
        //// Quality aggregation:
        if (mMeanCheck && mExpectedValueCheck) { // compare the total mean to the expected value. This is returned as the quality object
          if (fabs(mTotalMean - mExpectedValue) < mTotalStdev * mExpectedValueMediumSigmas) {
            result_Global = Quality::Good;
          } else if (fabs(mTotalMean - mExpectedValue) > mTotalStdev * mExpectedValueBadSigmas) {
            result_Global = Quality::Bad;
          } else {
            result_Global = Quality::Medium;
          }
        }

      } // for it in vectors (pads)

    } // if MO exists
  }   // MO-map loop
  return result_Global;
} // end of loop over moMap

//______________________________________________________________________________
std::string CheckOfPads::getAcceptedType() { return "TCanvas"; }

//______________________________________________________________________________
void CheckOfPads::beautify(std::shared_ptr<MonitorObject> mo, Quality)
{
  auto moName = mo->getName();
  if (auto it = std::find(mMOsToCheck2D.begin(), mMOsToCheck2D.end(), moName); it != mMOsToCheck2D.end()) {

    int padsTotal = 0, padsstart = 1000;
    auto* tcanv = (TCanvas*)mo->getObject();
    std::string histNameS, histName;
    padsstart = 1;
    padsTotal = 72;
    size_t end = moName.find("_2D");
    auto histSubName = moName.substr(7, end - 7);
    histNameS = fmt::format("h_{}_ROC", histSubName);
    for (int tpads = padsstart; tpads <= padsTotal; tpads++) {

      const std::string padName = fmt::format("{:s}_{:d}", moName, tpads);
      TPad* pad = (TPad*)tcanv->GetListOfPrimitives()->FindObject(padName.data());
      if (!pad) {
        continue;
      }
      pad->cd();
      TH1F* h = nullptr;
      histName = fmt::format("{}_{:02d}", histNameS, tpads - 1);
      h = (TH1F*)pad->GetListOfPrimitives()->FindObject(histName.data());
      if (!h) {
        continue;
      }
      const std::string titleh = h->GetTitle();
      auto it = std::find(mSectorsName.begin(), mSectorsName.end(), titleh);
      if (it == mSectorsName.end()) {
        continue;
      }

      const int index = std::distance(mSectorsName.begin(), it);
      TPaveText* msgQuality = new TPaveText(0.1, 0.85, 0.81, 0.95, "NDC");
      msgQuality->SetBorderSize(1);
      Quality qualitySpecial = mSectorsQuality[index];
      msgQuality->SetName(Form("%s_msg", mo->GetName()));
      if (qualitySpecial == Quality::Good) {
        msgQuality->Clear();
        msgQuality->AddText("Good");
        msgQuality->SetFillColor(kGreen);
      } else if (qualitySpecial == Quality::Bad) {
        msgQuality->Clear();
        msgQuality->AddText("Bad");
        msgQuality->SetFillColor(kRed);
      } else if (qualitySpecial == Quality::Medium) {
        msgQuality->Clear();
        msgQuality->AddText("Medium");
        msgQuality->SetFillColor(kOrange);
      } else if (qualitySpecial == Quality::Null) {
        h->SetFillColor(0);
      }

      if (mMeanCheck) {
        msgQuality->AddText(fmt::format("Pad Mean: {:1.4f}+-{:1.4f}, Global Mean: {:1.4f}", mPadMeans[index], mPadStdev[index], mTotalMean).data());
      }
      if (mExpectedValueCheck) {
        msgQuality->AddText(fmt::format("Pad Mean: {:1.4}+-{:1.4f}, Expected Value: {:1.4f}", mPadMeans[index], mPadStdev[index], mExpectedValue).data());
      }
      if (mEmptyCheck) {
        msgQuality->AddText(fmt::format("{:1.3f} % Empty pads.", mEmptyPadPercent[index] * 100).data());
      }
      h->SetLineColor(kBlack);
      msgQuality->Draw("same");
    }
    mSectorsName.clear();
    mSectorsQuality.clear();
  }
}

} // namespace o2::quality_control_modules::tpc
