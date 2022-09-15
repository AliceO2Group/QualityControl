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
#include "Common/Utils.h"

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
  mMediumQualityLimit = common::getFromConfig<float>(mCustomParameters, "mediumQualityPercentageOfWorkingPads", 0.7);
  mBadQualityLimit = common::getFromConfig<float>(mCustomParameters, "badQualityPercentageOfWorkingPads", 0.3);
  const std::string checkChoiceString = common::getFromConfig<std::string>(mCustomParameters, "CheckChoice", "Mean");

  if (size_t finder = checkChoiceString.find("ExpectedValue"); finder != std::string::npos) {
    mExpectedValueCheck = true;
  }
  if (size_t finder = checkChoiceString.find("Mean"); finder != std::string::npos) {
    mMeanCheck = true;
  }
  if (size_t finder = checkChoiceString.find("Empty"); finder != std::string::npos) {
    mEmptyCheck = true;
  }

  if (mExpectedValueCheck) {
    // load expectedValue and allowed standard deviations for medium/bad quality
    mExpectedValue = common::getFromConfig<float>(mCustomParameters, "ExpectedValue", 1.0);
    mExpectedValueMediumSigmas = common::getFromConfig<float>(mCustomParameters, "ExpectedValueSigmaMedium", 3.0);
    mExpectedValueBadSigmas = common::getFromConfig<float>(mCustomParameters, "ExpectedValueSigmaBad", 6.0);
  }
  // Check if Mean comparison is wished for:
  if (mMeanCheck) {
    // load Mean Sigma Medium
    mMeanMediumSigmas = common::getFromConfig<float>(mCustomParameters, "MeanSigmaMedium", 3.0);
    mMeanBadSigmas = common::getFromConfig<float>(mCustomParameters, "MeanSigmaBad", 6.0);
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
  Quality resultEV = Quality::Null;
  Quality resultMean = Quality::Null;
  Quality resultGlobal = Quality::Null;
  Quality resultEmpty = Quality::Null;
  for (auto const& moObj : *moMap) {
    auto mo = moObj.second;
    if (!mo) {
      continue;
    }
    const auto moName = mo->getName();
    if (auto it = std::find(mMOsToCheck2D.begin(), mMOsToCheck2D.end(), moName); it != mMOsToCheck2D.end()) {
      const size_t end = moName.find("_2D");
      const auto histSubName = moName.substr(7, end - 7);
      resultEV = Quality::Good;
      resultMean = Quality::Good;

      auto* canv = (TCanvas*)mo->getObject();
      if (!canv) {
        continue;
      }
      // Check all histograms in the canvas

      mTotalMean = 0.;

      for (int tpads = 1; tpads <= 72; tpads++) {
        const auto padName = fmt::format("{:s}_{:d}", moName, tpads);
        const auto histName = fmt::format("h_{:s}_ROC_{:02d}", histSubName, tpads - 1);
        TPad* pad = (TPad*)canv->GetListOfPrimitives()->FindObject(padName.data());
        if (!pad) {
          mSectorsName.push_back("notitle");
          mSectorsQualityEV.push_back(Quality::Null);
          continue;
        }
        TH2F* h = (TH2F*)pad->GetListOfPrimitives()->FindObject(histName.data());
        if (!h) {
          mSectorsName.push_back("notitle");
          mSectorsQualityEV.push_back(Quality::Null);
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
        } else if (titleh.find("OROC") != std::string::npos) {
          totalPads = 9280;
        } else {
          return Quality::Null;
        }

        float padSum = 0.;
        int padsCount = 0;
        float padStdev = 0.;
        float padMean = 0.;

        // Run twice to get the mean and the standard deviation
        for (int runNo = 1; runNo <= 2; runNo++) {
          // Run1: calculate single Pad total->Mean
          // Run2: calculate standardDeviation from mean
          for (int xBin = 1; xBin <= MaximumXBin; xBin++) {
            for (int yBin = 1; yBin <= MaximumYBin; yBin++) {
              const float binvalue = h->GetBinContent(xBin, yBin);
              if (binvalue != 0) {
                if (runNo == 1) {
                  padSum += binvalue;
                  padsCount++;
                } else {
                  padStdev += pow(binvalue - padMean, 2);
                }
              }
            }
          }

          if (runNo == 1 && padSum > 0.) { // no need for div by 0 check, because if padSum > 0 -> padsCount>0
            padMean = padSum / padsCount;
          }
        }

        if (padsCount == 0) {
          ILOG(Error, Support) << "The Pad provided (Pad: " << tpads << ") is empty." << ENDM;
          return Quality::Null;
        }

        if (mEmptyCheck) {
          if (padsCount > mMediumQualityLimit * totalPads) {
            resultEmpty = Quality::Good;
          } else if (padsCount < mBadQualityLimit * totalPads) {
            resultEmpty = Quality::Bad;
          } else {
            resultEmpty = Quality::Medium;
          }
          mSectorsQualityEmpty.push_back(resultEmpty);
          mEmptyPadPercent.push_back(1. - (float)padsCount / totalPads);
        }

        padStdev = sqrt(padStdev / (padsCount - 1));
        mPadMeans.push_back(padMean);
        mPadStdev.push_back(padStdev);
      }
      float sumOfWeights = 0.;
      for (size_t it = 0; it < mPadMeans.size(); it++) { // loop over all pads
        // calculate the total mean and standard deviation
        mTotalMean += mPadMeans[it] / mPadStdev[it];
        sumOfWeights += 1 / mPadStdev[it];
      }
      mTotalStdev = sqrt(1 / sumOfWeights); // standard deviation of the weighted average.
      mTotalMean /= sumOfWeights;           // Weighted average (by standard deviation) of the total mean
      // calculate the Qualities:

      for (size_t it = 0; it < mPadMeans.size(); it++) { // loop over all pads

        if (mExpectedValueCheck) {
          if (std::abs(mPadMeans[it] - mExpectedValue) < mPadStdev[it] * mExpectedValueMediumSigmas) {
            resultEV = Quality::Good;
          } else if (std::abs(mPadMeans[it] - mExpectedValue) >= mPadStdev[it] * mExpectedValueMediumSigmas && std::abs(mPadMeans[it] - mExpectedValue) < mPadStdev[it] * mExpectedValueBadSigmas) {
            resultEV = Quality::Medium;
          } else {
            resultEV = Quality::Bad;
          }
        }

        mSectorsQualityEV.push_back(resultEV);
        if (mMeanCheck) {
          if (std::abs(mPadMeans[it] - mTotalMean) < mPadStdev[it] * mMeanMediumSigmas) {
            resultMean = Quality::Good;
          } else if (std::abs(mPadMeans[it] - mTotalMean) >= mPadStdev[it] * mMeanMediumSigmas && std::abs(mPadMeans[it] - mTotalMean) < mPadStdev[it] * mMeanBadSigmas) {
            resultMean = Quality::Medium;
          } else {
            resultMean = Quality::Bad;
          }
        }

        mSectorsQualityMean.push_back(resultMean);

        std::vector<Quality> qualities;
        if (mMeanCheck) {
          qualities.push_back(resultMean);
        }

        if (mExpectedValueCheck) {
          qualities.push_back(resultEV);
        }

        if (mEmptyCheck) {
          qualities.push_back(mSectorsQualityEmpty[it]);
        }

        if (qualities.size() > 0) {
          Quality worst = Quality::Good;
          for (int Quality = 0; Quality < qualities.size(); Quality++) {
            if (qualities[Quality].isWorseThan(worst)) {
              worst = qualities[Quality];
            }
          }
          mSectorsQuality.push_back(worst);
        } else {
          ILOG(Error, Support) << "No Qualities were found!" << ENDM;
        }
        //// Quality aggregation:
        if (mMeanCheck && mExpectedValueCheck) { // compare the total mean to the expected value. This is returned as the quality object
          if (std::abs(mTotalMean - mExpectedValue) < mTotalStdev * mExpectedValueMediumSigmas) {
            resultGlobal = Quality::Good;
          } else if (std::abs(mTotalMean - mExpectedValue) > mTotalStdev * mExpectedValueBadSigmas) {
            resultGlobal = Quality::Bad;
          } else {
            resultGlobal = Quality::Medium;
          }
        }

      } // for it in vectors (pads)

    } // if MO exists
  }   // MO-map loop
  return resultGlobal;
} // end of loop over moMap

//______________________________________________________________________________
std::string CheckOfPads::getAcceptedType() { return "TCanvas"; }

//______________________________________________________________________________
void CheckOfPads::beautify(std::shared_ptr<MonitorObject> mo, Quality)
{
  auto moName = mo->getName();
  if (auto it = std::find(mMOsToCheck2D.begin(), mMOsToCheck2D.end(), moName); it != mMOsToCheck2D.end()) {

    auto* tcanv = (TCanvas*)mo->getObject();
    const int padsstart = 1;
    const int padsTotal = 72;
    const size_t end = moName.find("_2D");
    const auto histSubName = moName.substr(7, end - 7);
    const auto histNameS = fmt::format("h_{}_ROC", histSubName);
    for (int tpads = padsstart; tpads <= padsTotal; tpads++) {

      const std::string padName = fmt::format("{:s}_{:d}", moName, tpads);
      TPad* pad = (TPad*)tcanv->GetListOfPrimitives()->FindObject(padName.data());
      if (!pad) {
        continue;
      }
      pad->cd();
      const auto histName = fmt::format("{}_{:02d}", histNameS, tpads - 1);
      const auto h = (TH1F*)pad->GetListOfPrimitives()->FindObject(histName.data());
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
