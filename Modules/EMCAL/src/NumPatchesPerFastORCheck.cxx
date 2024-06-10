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
/// \file   NumPatchesPerFastORCheck.cxx
/// \author Sierra Cantway
///

#include "EMCAL/NumPatchesPerFastORCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include <DataFormatsQualityControl/FlagType.h>
#include <DataFormatsQualityControl/FlagTypeFactory.h>
#include "EMCALBase/Geometry.h"
#include <fairlogger/Logger.h>
// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TPaveText.h>
#include <TLatex.h>
#include <TList.h>
#include <TRobustEstimator.h>
#include <TMath.h>
#include <ROOT/TSeq.hxx>
#include <iostream>
#include <vector>
#include <sstream>

using namespace std;

namespace o2::quality_control_modules::emcal
{

bool compareDescending(const NumPatchesPerFastORCheck::FastORNoiseLevel& noise1, const NumPatchesPerFastORCheck::FastORNoiseLevel& noise2)
{
  if (noise1.mCounts == noise2.mCounts) {
    return noise1.mFastORID > noise2.mFastORID; // Sort by mFastORID in descending order if mCounts are equal
  }
  return noise1.mCounts > noise2.mCounts; // Used to sort by count in descending order
}

void NumPatchesPerFastORCheck::configure()
{
  // configure nsigma-based checkers
  auto nBadSigmaNumPatchesPerFastOR = mCustomParameters.find("BadSigmaNumPatchesPerFastOR");
  if (nBadSigmaNumPatchesPerFastOR != mCustomParameters.end()) {
    try {
      mBadSigmaNumPatchesPerFastOR = std::stof(nBadSigmaNumPatchesPerFastOR->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << "Value " << nBadSigmaNumPatchesPerFastOR->second << " not a float" << ENDM;
    }
  }
  auto nMedSigmaNumPatchesPerFastOR = mCustomParameters.find("MedSigmaNumPatchesPerFastOR");
  if (nMedSigmaNumPatchesPerFastOR != mCustomParameters.end()) {
    try {
      mMedSigmaNumPatchesPerFastOR = std::stof(nMedSigmaNumPatchesPerFastOR->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << "Value " << nMedSigmaNumPatchesPerFastOR->second << " not a float" << ENDM;
    }
  }

  auto logLevelIL = mCustomParameters.find("LogLevelIL");
  if (logLevelIL != mCustomParameters.end()) {
    try {
      mLogLevelIL = std::stoi(logLevelIL->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << "Value " << logLevelIL->second << " not an integer" << ENDM;
    }
  }
}

Quality NumPatchesPerFastORCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  auto mo = moMap->begin()->second;
  Quality result = Quality::Good;
  mNoisyTRUPositions.clear();
  mHighCountTRUPositions.clear();
  std::stringstream messagebuilder;

  std::vector<FastORNoiseLevel> candBadFastORs;
  std::vector<FastORNoiseLevel> finalBadFastORs;
  std::vector<FastORNoiseLevel> candMedFastORs;
  std::vector<FastORNoiseLevel> finalMedFastORs;

  if (mo->getName() == "NumberOfPatchesWithFastOR") {
    auto* h = dynamic_cast<TH1*>(mo->getObject());
    if (h->GetEntries() == 0) {
      result = Quality::Medium;
    } else {

      // Determine the threshold for bad and medium FastOR candidates
      std::vector<double> smcounts;
      for (auto ib : ROOT::TSeqI(0, h->GetXaxis()->GetNbins())) {
        auto countSM = h->GetBinContent(ib + 1);
        if (countSM > 0) {
          smcounts.emplace_back(countSM);
        }
      }
      if (!smcounts.size()) {
        result = Quality::Medium;
      } else {
        TRobustEstimator meanfinder;
        double mean, sigma;
        meanfinder.EvaluateUni(smcounts.size(), smcounts.data(), mean, sigma);

        double thresholdBad = mean + mBadSigmaNumPatchesPerFastOR * sigma,
               thresholdMedium = mean + mMedSigmaNumPatchesPerFastOR * sigma;

        // Find the noisy FastOR candidates
        for (auto ib : ROOT::TSeqI(0, h->GetXaxis()->GetNbins())) {
          if (h->GetBinContent(ib + 1) > thresholdMedium) {
            if (result != Quality::Bad) {
              result = Quality::Medium;
            }
            FastORNoiseLevel cand{ static_cast<int>(h->GetBinContent(ib + 1)), static_cast<int>(h->GetXaxis()->GetBinCenter(ib + 1)) };
            candMedFastORs.push_back(cand);
          }
          if (h->GetBinContent(ib + 1) > thresholdBad) {
            result = Quality::Bad;
            FastORNoiseLevel cand{ static_cast<int>(h->GetBinContent(ib + 1)), static_cast<int>(h->GetXaxis()->GetBinCenter(ib + 1)) };
            candBadFastORs.push_back(cand);
          }
        }

        // Sort the noisy FastOR candidates in descending counts order
        std::sort(candBadFastORs.begin(), candBadFastORs.end(), compareDescending);
        std::sort(candMedFastORs.begin(), candMedFastORs.end(), compareDescending);

        // Iterate over the candidate Bad FastORs using a while loop to remove the smaller nearby FastORs to the noisy ones
        std::vector<FastORNoiseLevel>::iterator itBad = candBadFastORs.begin();
        while (itBad != candBadFastORs.end()) {
          // Get all the information of the highest remaining FastOR
          auto [high_truID, high_fastorTRU] = mTriggerMapping->getTRUFromAbsFastORIndex(itBad->mFastORID);
          auto [high_truID1, high_posEta, high_posPhi] = mTriggerMapping->getPositionInTRUFromAbsFastORIndex(itBad->mFastORID);
          FastORNoiseInfo high_obj{ static_cast<int>(high_truID), static_cast<int>(high_fastorTRU), static_cast<int>(high_posPhi), static_cast<int>(high_posEta) };

          std::vector<FastORNoiseLevel> toErase; // Vector to store elements to be erased

          // Loop over rest of candidate Bad FastORs to find the smaller nearby FastORs
          for (std::vector<FastORNoiseLevel>::iterator i = itBad; i != candBadFastORs.end(); i++) {
            // Get all the information of the next FastOR
            auto [truID, fastorTRU] = mTriggerMapping->getTRUFromAbsFastORIndex(i->mFastORID);
            auto [truID1, posEta, posPhi] = mTriggerMapping->getPositionInTRUFromAbsFastORIndex(i->mFastORID);
            FastORNoiseInfo obj{ static_cast<int>(truID), static_cast<int>(fastorTRU), static_cast<int>(posPhi), static_cast<int>(posEta) };

            // Check if it is near the current highest FastOR
            if ((high_posEta == posEta) || (high_posEta == posEta + 1) || (high_posEta == posEta - 1)) {
              if ((high_posPhi == posPhi) || (high_posPhi == posPhi + 1) || (high_posPhi == posPhi - 1)) {
                if (!((high_posEta == posEta) && (high_posPhi == posPhi))) {
                  FastORNoiseLevel close{ i->mCounts, i->mFastORID };
                  toErase.push_back(close); // Store the FastOR to be erased
                }
              }
            }
          }

          // Erase the smaller nearby FastORs
          for (auto itErase : toErase) {
            auto it = std::find(candBadFastORs.begin(), candBadFastORs.end(), itErase);
            while (it != candBadFastORs.end()) {
              candBadFastORs.erase(it);
              it = std::find(candBadFastORs.begin(), candBadFastORs.end(), itErase);
            }
          }

          // Clear the vector
          toErase.clear();

          // Save the high FastOR as bad
          FastORNoiseLevel final{ itBad->mCounts, itBad->mFastORID };
          finalBadFastORs.push_back(final);

          // Move to the next highest FastOR remaining in the vector
          ++itBad;
        }

        // Save the positions of the final Bad FastORs and display the error message
        for (std::vector<FastORNoiseLevel>::iterator itFinal = finalBadFastORs.begin(); itFinal != finalBadFastORs.end(); itFinal++) {
          auto [truID, fastorTRU] = mTriggerMapping->getTRUFromAbsFastORIndex(itFinal->mFastORID);
          auto [truID1, posEta, posPhi] = mTriggerMapping->getPositionInTRUFromAbsFastORIndex(itFinal->mFastORID);
          FastORNoiseInfo obj{ static_cast<int>(truID), static_cast<int>(fastorTRU), static_cast<int>(posPhi), static_cast<int>(posEta) };
          std::string errorMessage = "TRU " + std::to_string(truID) + " has a noisy FastOR at position " + std::to_string(fastorTRU) + " (eta " + std::to_string(posEta) + ", phi " + std::to_string(posPhi) + ") in TRU.";
          messagebuilder << errorMessage << std::endl;
          if (mLogLevelIL > 1) {
            ILOG(Error, Support) << errorMessage << ENDM;
          }
          mNoisyTRUPositions.insert(obj);
        }

        // Iterate over the candidate Med FastORs using a while loop to remove the smaller nearby FastORs to the noisy ones
        std::vector<FastORNoiseLevel>::iterator itMed = candMedFastORs.begin();
        while (itMed != candMedFastORs.end()) {
          // Get all the information of the highest remaining FastOR
          auto [high_truID, high_fastorTRU] = mTriggerMapping->getTRUFromAbsFastORIndex(itMed->mFastORID);
          auto [high_truID1, high_posEta, high_posPhi] = mTriggerMapping->getPositionInTRUFromAbsFastORIndex(itMed->mFastORID);
          FastORNoiseInfo high_obj{ static_cast<int>(high_truID), static_cast<int>(high_fastorTRU), static_cast<int>(high_posPhi), static_cast<int>(high_posEta) };

          std::vector<FastORNoiseLevel> toErase; // Vector to store elements to be erased

          /// Loop over rest of candidate Med FastORs to find the smaller nearby FastORs
          for (std::vector<FastORNoiseLevel>::iterator i = itMed; i != candMedFastORs.end(); i++) {
            // Get all the information of the next FastOR
            auto [truID, fastorTRU] = mTriggerMapping->getTRUFromAbsFastORIndex(i->mFastORID);
            auto [truID1, posEta, posPhi] = mTriggerMapping->getPositionInTRUFromAbsFastORIndex(i->mFastORID);
            FastORNoiseInfo obj{ static_cast<int>(truID), static_cast<int>(fastorTRU), static_cast<int>(posPhi), static_cast<int>(posEta) };

            // Check if it is near the current highest FastOR
            if ((high_posEta == posEta) || (high_posEta == posEta + 1) || (high_posEta == posEta - 1)) {
              if ((high_posPhi == posPhi) || (high_posPhi == posPhi + 1) || (high_posPhi == posPhi - 1)) {
                if (!((high_posEta == posEta) && (high_posPhi == posPhi))) {
                  FastORNoiseLevel close{ i->mCounts, i->mFastORID };
                  toErase.push_back(close); // Store the FastOR to be erased
                }
              }
            }
          }

          // Erase the smaller nearby FastORs
          for (auto itErase : toErase) {
            auto it = std::find(candMedFastORs.begin(), candMedFastORs.end(), itErase);
            while (it != candMedFastORs.end()) {
              candMedFastORs.erase(it);
              it = std::find(candMedFastORs.begin(), candMedFastORs.end(), itErase);
            }
          }

          // Clear the vector
          toErase.clear();

          // Save the high FastOR as medium
          FastORNoiseLevel final{ itMed->mCounts, itMed->mFastORID };
          finalMedFastORs.push_back(final);

          // Move to the next highest FastOR remaining in the vector
          ++itMed;
        }

        // Save the positions of the final Med FastORs and display the error message
        for (std::vector<FastORNoiseLevel>::iterator itFinal = finalMedFastORs.begin(); itFinal != finalMedFastORs.end(); itFinal++) {
          auto [truID, fastorTRU] = mTriggerMapping->getTRUFromAbsFastORIndex(itFinal->mFastORID);
          auto [truID1, posEta, posPhi] = mTriggerMapping->getPositionInTRUFromAbsFastORIndex(itFinal->mFastORID);
          FastORNoiseInfo obj{ static_cast<int>(truID), static_cast<int>(fastorTRU), static_cast<int>(posPhi), static_cast<int>(posEta) };
          std::string errorMessage = "TRU " + std::to_string(truID) + " has a high rate in FastOR at position " + std::to_string(fastorTRU) + " (eta " + std::to_string(posEta) + ", phi " + std::to_string(posPhi) + ") in TRU.";
          messagebuilder << errorMessage << std::endl;
          if (mLogLevelIL > 2) {
            ILOG(Warning, Support) << errorMessage << ENDM;
          }
          mHighCountTRUPositions.insert(obj);
        }
      }
    }
  }
  result.addFlag(o2::quality_control::FlagTypeFactory::BadEMCalorimetry(), messagebuilder.str());

  return result;
}

std::string NumPatchesPerFastORCheck::getAcceptedType() { return "TH1"; }

void NumPatchesPerFastORCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "NumberOfPatchesWithFastOR") {
    auto* h = dynamic_cast<TH1*>(mo->getObject());
    if (checkResult == Quality::Good) {
      TPaveText* msg = new TPaveText(0.12, 0.84, 0.88, 0.94, "NDC");
      h->GetListOfFunctions()->Add(msg);
      msg->SetName(Form("%s_msg", mo->GetName()));
      msg->Clear();
      msg->AddText("Data OK: No Outlier Noisy FastORs");
      msg->SetFillColor(kGreen);
      msg->Draw();
    } else if (checkResult == Quality::Bad) {
      TLatex* msg;
      msg = new TLatex(0.15, 0.84, "#color[2]{Error: Noisy TRU(s)}");
      msg->SetNDC();
      msg->SetTextSize(16);
      msg->SetTextFont(43);
      msg->SetTextColor(kRed);
      h->GetListOfFunctions()->Add(msg);
      msg->Draw();
      int iErr = 0;
      for (const auto& noiseinfo : mNoisyTRUPositions) {
        stringstream errorMessageIndiv;
        errorMessageIndiv << "Position " << noiseinfo.mFastORIndex << " (eta " << noiseinfo.mPosEta << ", phi " << noiseinfo.mPosPhi << ") in TRU " << noiseinfo.mTRUIndex << " is noisy." << std::endl;
        msg = new TLatex(0.15, 0.8 - iErr / 25., errorMessageIndiv.str().c_str());
        msg->SetNDC();
        msg->SetTextSize(16);
        msg->SetTextFont(43);
        msg->SetTextColor(kRed);
        h->GetListOfFunctions()->Add(msg);
        msg->Draw();
        if (mLogLevelIL > 0) {
          ILOG(Error, Support) << errorMessageIndiv.str() << ENDM;
        }
        iErr++;
      }
      for (const auto& noiseinfo : mHighCountTRUPositions) {
        stringstream errorMessageIndiv;
        errorMessageIndiv << "Position " << noiseinfo.mFastORIndex << " (eta " << noiseinfo.mPosEta << ", phi " << noiseinfo.mPosPhi << ") in TRU " << noiseinfo.mTRUIndex << " has high counts." << std::endl;
        msg = new TLatex(0.15, 0.8 - iErr / 25., errorMessageIndiv.str().c_str());
        msg->SetNDC();
        msg->SetTextSize(16);
        msg->SetTextFont(43);
        msg->SetTextColor(kOrange);
        h->GetListOfFunctions()->Add(msg);
        msg->Draw();
        if (mLogLevelIL > 0) {
          ILOG(Warning, Support) << errorMessageIndiv.str() << ENDM;
        }
        iErr++;
      }
    } else if (checkResult == Quality::Medium) {
      TLatex* msg;
      msg = new TLatex(0.15, 0.84, "#color[2]{Error: High rate TRU(s)}");
      msg->SetNDC();
      msg->SetTextSize(16);
      msg->SetTextFont(43);
      msg->SetTextColor(kRed);
      h->GetListOfFunctions()->Add(msg);
      int iErr = 0;
      for (const auto& noiseinfo : mHighCountTRUPositions) {
        stringstream errorMessageIndiv;
        errorMessageIndiv << "Position " << noiseinfo.mFastORIndex << " (eta " << noiseinfo.mPosEta << ", phi " << noiseinfo.mPosPhi << ") in TRU " << noiseinfo.mTRUIndex << " has high counts." << std::endl;
        msg = new TLatex(0.15, 0.8 - iErr / 25., errorMessageIndiv.str().c_str());
        msg->SetNDC();
        msg->SetTextSize(16);
        msg->SetTextFont(43);
        msg->SetTextColor(kOrange);
        h->GetListOfFunctions()->Add(msg);
        msg->Draw();
        if (mLogLevelIL > 0) {
          ILOG(Warning, Support) << errorMessageIndiv.str() << ENDM;
        }
        iErr++;
      }
    }
  }
}
} // namespace o2::quality_control_modules::emcal
