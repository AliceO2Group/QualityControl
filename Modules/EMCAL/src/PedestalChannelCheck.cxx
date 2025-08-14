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
/// \file   PedestalChannelCheck.cxx
/// \author Sierra Cantway
///

#include "EMCAL/PedestalChannelCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include <fairlogger/Logger.h>
#include <DataFormatsQualityControl/FlagType.h>
#include <DataFormatsQualityControl/FlagTypeFactory.h>
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

using namespace std;

namespace o2::quality_control_modules::emcal
{

void PedestalChannelCheck::configure()
{
  // configure threshold-based checkers for bad quality
  auto nBadPedestalChannelADCRangeLow = mCustomParameters.find("BadPedestalChannelADCRangeLow");
  if (nBadPedestalChannelADCRangeLow != mCustomParameters.end()) {
    try {
      mBadPedestalChannelADCRangeLow = std::stof(nBadPedestalChannelADCRangeLow->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << fmt::format("Value {} not a float", nBadPedestalChannelADCRangeLow->second.data()) << ENDM;
    }
  }

  auto nBadPedestalChannelADCRangeHigh = mCustomParameters.find("BadPedestalChannelADCRangeHigh");
  if (nBadPedestalChannelADCRangeHigh != mCustomParameters.end()) {
    try {
      mBadPedestalChannelADCRangeHigh = std::stof(nBadPedestalChannelADCRangeHigh->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << fmt::format("Value {} not a float", nBadPedestalChannelADCRangeHigh->second.data()) << ENDM;
    }
  }

  auto nBadPedestalChannelOverMeanThreshold = mCustomParameters.find("BadPedestalChannelOverMeanThreshold");
  if (nBadPedestalChannelOverMeanThreshold != mCustomParameters.end()) {
    try {
      mBadPedestalChannelOverMeanThreshold = std::stof(nBadPedestalChannelOverMeanThreshold->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << fmt::format("Value {} not a float", nBadPedestalChannelOverMeanThreshold->second.data()) << ENDM;
    }
  }

  // configure threshold-based checkers for medium quality
  auto nMedPedestalChannelADCRangeLow = mCustomParameters.find("MedPedestalChannelADCRangeLow");
  if (nMedPedestalChannelADCRangeLow != mCustomParameters.end()) {
    try {
      mMedPedestalChannelADCRangeLow = std::stof(nMedPedestalChannelADCRangeLow->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << fmt::format("Value {} not a float", nMedPedestalChannelADCRangeLow->second.data()) << ENDM;
    }
  }

  auto nMedPedestalChannelADCRangeHigh = mCustomParameters.find("MedPedestalChannelADCRangeHigh");
  if (nMedPedestalChannelADCRangeHigh != mCustomParameters.end()) {
    try {
      mMedPedestalChannelADCRangeHigh = std::stof(nMedPedestalChannelADCRangeHigh->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << fmt::format("Value {} not a float", nMedPedestalChannelADCRangeHigh->second.data()) << ENDM;
    }
  }

  auto nMedPedestalChannelOverMeanThreshold = mCustomParameters.find("MedPedestalChannelOverMeanThreshold");
  if (nMedPedestalChannelOverMeanThreshold != mCustomParameters.end()) {
    try {
      mMedPedestalChannelOverMeanThreshold = std::stof(nMedPedestalChannelOverMeanThreshold->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << fmt::format("Value {} not a float", nMedPedestalChannelOverMeanThreshold->second.data()) << ENDM;
    }
  }

  // configure nsigma-based checkers
  auto nSigmaPedestalChannel = mCustomParameters.find("SigmaPedestalChannel");
  if (nSigmaPedestalChannel != mCustomParameters.end()) {
    try {
      mSigmaPedestalChannel = std::stof(nSigmaPedestalChannel->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << fmt::format("Value {} not a float", nSigmaPedestalChannel->second.data()) << ENDM;
    }
  }
}

Quality PedestalChannelCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  auto mo = moMap->begin()->second;
  Quality result = Quality::Good;
  std::stringstream messagebuilder;

  std::array<std::string, 4> channelHists = { { "mPedestalChannelFECHG", "mPedestalChannelFECLG", "mPedestalChannelLEDMONHG", "mPedestalChannelLEDMONLG" } };
  if (std::find(channelHists.begin(), channelHists.end(), mo->getName()) != channelHists.end()) {
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
        int outside_counts = 0.0;
        if (mean < mBadPedestalChannelADCRangeLow || mean > mBadPedestalChannelADCRangeHigh) {
          result = Quality::Bad;
          messagebuilder << "Error: Average Pedestal outside limits (" << mBadPedestalChannelADCRangeLow << "-" << mBadPedestalChannelADCRangeHigh << ") " << std::endl;
        } else if (mean < mMedPedestalChannelADCRangeLow || mean > mMedPedestalChannelADCRangeHigh) {
          result = Quality::Medium;
          messagebuilder << "Warning: Average Pedestal outside limits (" << mMedPedestalChannelADCRangeLow << "-" << mMedPedestalChannelADCRangeHigh << ") " << std::endl;
        }

        for (auto ib : ROOT::TSeqI(0, h->GetXaxis()->GetNbins())) {
          if (h->GetBinContent(ib + 1) > (mean + mSigmaPedestalChannel * sigma)) {
            outside_counts += 1;
          }
        }
        Float_t out_counts_frac = (float)(outside_counts) / (float)(h->GetXaxis()->GetNbins());

        if (out_counts_frac > mBadPedestalChannelOverMeanThreshold) {
          result = Quality::Bad;
          messagebuilder << "Error: Fraction of Outliers Above " << mMedPedestalChannelOverMeanThreshold << std::endl;
        } else if (out_counts_frac > mMedPedestalChannelOverMeanThreshold) {
          result = Quality::Medium;
          messagebuilder << "Warning: Fraction of Outliers Above " << mMedPedestalChannelOverMeanThreshold << std::endl;
        }
      }
    }
  }

  result.addFlag(o2::quality_control::FlagTypeFactory::BadEMCalorimetry(), messagebuilder.str());

  return result;
}



void PedestalChannelCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  std::array<std::string, 4> channelHists = { { "mPedestalChannelFECHG", "mPedestalChannelFECLG", "mPedestalChannelLEDMONHG", "mPedestalChannelLEDMONLG" } };
  if (std::find(channelHists.begin(), channelHists.end(), mo->getName()) != channelHists.end()) {
    auto* h = dynamic_cast<TH1*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.12, 0.84, 0.88, 0.94, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));
    if (checkResult == Quality::Good) {
      msg->Clear();
      msg->AddText("Data OK: Average Pedestal & Fraction of Outliers Within Limits");
      msg->SetFillColor(kGreen);
      msg->Draw();
    } else if (checkResult == Quality::Bad) {
      ILOG(Debug, Devel) << "Quality::Bad, setting to red";
      stringstream msg_bad;
      msg_bad << "Error: Average Pedestal outside limits (" << mBadPedestalChannelADCRangeLow << "-" << mBadPedestalChannelADCRangeHigh << ") or Fraction of Outliers Above " << mBadPedestalChannelOverMeanThreshold << std::endl;
      msg->Clear();
      msg->AddText(msg_bad.str().c_str());
      msg->AddText("Call EMCAL oncall");
      msg->SetFillColor(kRed);
      msg->Draw();
    } else if (checkResult == Quality::Medium) {
      ILOG(Debug, Devel) << "Quality::medium, setting to orange";
      stringstream msg_med;
      msg_med << "Warning: Average Pedestal outside limits (" << mMedPedestalChannelADCRangeLow << "-" << mMedPedestalChannelADCRangeHigh << ") or Fraction of Outliers Above " << mMedPedestalChannelOverMeanThreshold << std::endl;
      msg->Clear();
      msg->AddText(msg_med.str().c_str());
      msg->SetFillColor(kOrange);
      msg->Draw();
    }
  }
}
} // namespace o2::quality_control_modules::emcal
