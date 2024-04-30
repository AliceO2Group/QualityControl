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
      ILOG(Error, Support) << fmt::format("Value {} not a float", nBadThresholdNumPatchesPerFastOR->second.data()) << ENDM;
    }
  }

  // configure TSpectrum parameters
  auto nSigmaTSpectrum = mCustomParameters.find("TSpecSigma");
  if (nSigmaTSpectrum != mCustomParameters.end()) {
    try {
      mSigmaTSpectrum = std::stof(nSigmaTSpectrum->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << fmt::format("Value {} not a float", nSigmaTSpectrum->second.data()) << ENDM;
    }
  }

  auto nThreshTSpectrum = mCustomParameters.find("TSpecThresh");
  if (nThreshTSpectrum != mCustomParameters.end()) {
    try {
      mThreshTSpectrum = std::stof(nThreshTSpectrum->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << fmt::format("Value {} not a float", nThreshTSpectrum->second.data()) << ENDM;
    }
  }
}

Quality NumPatchesPerFastORCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  auto mo = moMap->begin()->second;
  Quality result = Quality::Good;
  mErrorMessage.str("");
  mNoisyTRUPositions.clear();

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
        double threshold = mBadThresholdNumPatchesPerFastOR * mean;

        for (Int_t n_peak = 0; n_peak < nfound; n_peak++) {
          Int_t bin = h->GetXaxis()->FindBin(xpeaks[n_peak]);
          Double_t peak_val = h->GetBinContent(bin);

          if (peak_val > threshold) {
            result = Quality::Bad;
            Double_t peak_pos = h->GetXaxis()->GetBinCenter(bin);
            auto [truID, fastorTRU] = mTriggerMapping->getTRUFromAbsFastORIndex(peak_pos);
            mErrorMessage << "TRU " << truID << " has a noisy FastOR at position " << fastorTRU << " in TRU." << std::endl;
            ILOG(Error, Support) << "TRU " << truID << " has a noisy FastOR at position " << fastorTRU << " in TRU." << ENDM;
            mNoisyTRUPositions.push_back(std::make_pair(truID, fastorTRU));
          }
        }
      }
    }
  }

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
      if (!mErrorMessage.str().empty()) {
        ILOG(Error, Support) << mErrorMessage.str() << ENDM;
        msg = new TLatex(0.15, 0.84, "#color[2]{Error: Noisy TRU(s)}");
        msg->SetNDC();
        msg->SetTextSize(16);
        msg->SetTextFont(43);
        msg->SetTextColor(kRed);
        h->GetListOfFunctions()->Add(msg);
        msg->Draw();
      }
      for (int iErr = 0; iErr < mNoisyTRUPositions.size(); iErr++) {
        stringstream errorMessageIndiv;
        errorMessageIndiv << "Position " << mNoisyTRUPositions[iErr].second << " in TRU " << mNoisyTRUPositions[iErr].first << " is noisy." << std::endl;
        msg = new TLatex(0.15, 0.8 - iErr / 25., errorMessageIndiv.str().c_str());
        msg->SetNDC();
        msg->SetTextSize(16);
        msg->SetTextFont(43);
        msg->SetTextColor(kRed);
        h->GetListOfFunctions()->Add(msg);
        msg->Draw();
        ILOG(Error, Support) << errorMessageIndiv.str() << ENDM;
      }
    }
  }
}
} // namespace o2::quality_control_modules::emcal
