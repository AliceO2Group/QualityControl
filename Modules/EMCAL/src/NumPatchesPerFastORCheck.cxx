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
#include <TSpectrum.h>
#include <iostream>
#include <vector>
#include <sstream>

using namespace std;

namespace o2::quality_control_modules::emcal
{

void NumPatchesPerFastORCheck::configure()
{
  // configure threshold-based checkers for bad quality
  auto nBadThresholdNumPatchesPerFastOR = mCustomParameters.find("BadThresholdNumPatchesPerFastOR");
  if (nBadThresholdNumPatchesPerFastOR != mCustomParameters.end()) {
    try {
      mBadThresholdNumPatchesPerFastOR = std::stof(nBadThresholdNumPatchesPerFastOR->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << "Value " << nBadThresholdNumPatchesPerFastOR->second.data() << " not a float" << ENDM;
    }
  }

  auto nMediumThresholdNumPatchesPerFastOR = mCustomParameters.find("MediumThresholdNumPatchesPerFastOR");
  if (nMediumThresholdNumPatchesPerFastOR != mCustomParameters.end()) {
    try {
      mMediumThresholdNumPatchesPerFastOR = std::stof(nMediumThresholdNumPatchesPerFastOR->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << "Value " << nMediumThresholdNumPatchesPerFastOR->second.data() << " not a float" << ENDM;
    }
  }

  // configure TSpectrum parameters
  auto nSigmaTSpectrum = mCustomParameters.find("TSpecSigma");
  if (nSigmaTSpectrum != mCustomParameters.end()) {
    try {
      mSigmaTSpectrum = std::stof(nSigmaTSpectrum->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << "Value " << nSigmaTSpectrum->second.data() << " not a float" << ENDM;
    }
  }

  auto nThreshTSpectrum = mCustomParameters.find("TSpecThresh");
  if (nThreshTSpectrum != mCustomParameters.end()) {
    try {
      mThreshTSpectrum = std::stof(nThreshTSpectrum->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << "Value " << nThreshTSpectrum->second.data() << " not a float" << ENDM;
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
  std::stringstream messagebuilder;

  if (mo->getName() == "NumberOfPatchesWithFastOR") {
    auto* h = dynamic_cast<TH1*>(mo->getObject());
    if (h->GetEntries() == 0) {
      result = Quality::Medium;
    } else {
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

        TSpectrum peakfinder;
        Int_t nfound = peakfinder.Search(h, mSigmaTSpectrum, "nobackground", mThreshTSpectrum); // Search for peaks //CHANGE - make config?
        Double_t* xpeaks = peakfinder.GetPositionX();
        Double_t* ypeaks = peakfinder.GetPositionY();
        std::sort(ypeaks, ypeaks + nfound, std::greater<double>()); // sort peaks in descending order to easy pick the y value of max peak
        double thresholdBad = mBadThresholdNumPatchesPerFastOR * mean,
               thresholdMedium = mMediumThresholdNumPatchesPerFastOR * mean;

        for (Int_t n_peak = 0; n_peak < nfound; n_peak++) {
          Int_t bin = h->GetXaxis()->FindBin(xpeaks[n_peak]);
          Double_t peak_val = h->GetBinContent(bin);

          if (peak_val > thresholdBad || peak_val > thresholdMedium) {
            Double_t peak_pos = h->GetXaxis()->GetBinCenter(bin);
            auto [truID, fastorTRU] = mTriggerMapping->getTRUFromAbsFastORIndex(peak_pos);
            auto [truID1, posEta, posPhi] = mTriggerMapping->getPositionInTRUFromAbsFastORIndex(peak_pos);
            FastORNoiseInfo obj{ static_cast<int>(truID), static_cast<int>(fastorTRU), static_cast<int>(posPhi), static_cast<int>(posEta) };
            if (peak_val > thresholdBad) {
              result = Quality::Bad;
              std::string errorMessage = "TRU " + std::to_string(truID) + " has a noisy FastOR at position " + std::to_string(fastorTRU) + " (eta " + std::to_string(posEta) + ", phi " + std::to_string(posPhi) + ") in TRU.";
              messagebuilder << errorMessage << std::endl;
              if (mLogLevelIL > 1) {
                ILOG(Error, Support) << errorMessage << ENDM;
              }
              mNoisyTRUPositions.insert(obj);
            } else if (peak_val > thresholdMedium) {
              if (result != Quality::Bad) {
                result = Quality::Medium;
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
      }
    }
  }
  result.addFlag(o2::quality_control::FlagTypeFactory::BadEMCalorimetry(), messagebuilder.str());

  return result;
}

std::string NumPatchesPerFastORCheck::getAcceptedType() { return "TH1"; }

void NumPatchesPerFastORCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  // To do - good message
  if (mo->getName() == "NumberOfPatchesWithFastOR") {
    auto* h = dynamic_cast<TH1*>(mo->getObject());
    if (checkResult == Quality::Bad) {
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
        errorMessageIndiv << "Position " << noiseinfo.mFastORIndex << " (eta " << noiseinfo.mPosEta << ", phi " << noiseinfo.mPosPhi << ") in TRU " << noiseinfo.mTRUIndex << " has high counts" << std::endl;
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
        errorMessageIndiv << "Position " << noiseinfo.mFastORIndex << " (eta " << noiseinfo.mPosEta << ", phi " << noiseinfo.mPosPhi << ") in TRU " << noiseinfo.mTRUIndex << " has high counts" << std::endl;
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
