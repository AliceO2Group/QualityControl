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
/// \file   CalibCheck.cxx
/// \author Sierra Cantway
///

#include "EMCAL/CalibCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include <fairlogger/Logger.h>
// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TPaveText.h>
#include <TLatex.h>
#include <TList.h>
#include <TRobustEstimator.h>
#include <ROOT/TSeq.hxx>
#include <iostream>
#include <vector>

using namespace std;

namespace o2::quality_control_modules::emcal
{

void CalibCheck::configure()
{
  // configure threshold-based checkers for bad quality
  auto nBadThresholdMaskStatsAll = mCustomParameters.find("BadThresholdMaskStatsAll");
  if (nBadThresholdMaskStatsAll != mCustomParameters.end()) {
    try {
      mBadThresholdMaskStatsAll = std::stof(nBadThresholdMaskStatsAll->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << fmt::format("Value {} not a float", nBadThresholdMaskStatsAll->second.data()) << ENDM;
    }
  }

  auto nBadThresholdTimeCalibCoeff = mCustomParameters.find("BadThresholdTimeCalibCoeff");
  if (nBadThresholdTimeCalibCoeff != mCustomParameters.end()) {
    try {
      mBadThresholdTimeCalibCoeff = std::stof(nBadThresholdTimeCalibCoeff->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << fmt::format("Value {} not a float", nBadThresholdTimeCalibCoeff->second.data()) << ENDM;
    }
  }

  auto nBadThresholdCellAmplitudeSupermoduleCalibPHYS = mCustomParameters.find("BadThresholdCellAmplitudeSupermoduleCalibPHYS");
  if (nBadThresholdCellAmplitudeSupermoduleCalibPHYS != mCustomParameters.end()) {
    try {
      mBadThresholdCellAmplitudeSupermoduleCalibPHYS = std::stof(nBadThresholdCellAmplitudeSupermoduleCalibPHYS->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << fmt::format("Value {} not a float", nBadThresholdCellAmplitudeSupermoduleCalibPHYS->second.data()) << ENDM;
    }
  }

  auto nBadThresholdFractionGoodCellsEvent = mCustomParameters.find("BadThresholdFractionGoodCellsEvent");
  if (nBadThresholdFractionGoodCellsEvent != mCustomParameters.end()) {
    try {
      mBadThresholdFractionGoodCellsEvent = std::stof(nBadThresholdFractionGoodCellsEvent->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << fmt::format("Value {} not a float", nBadThresholdFractionGoodCellsEvent->second.data()) << ENDM;
    }
  }

  auto nBadThresholdFractionGoodCellsSupermodule = mCustomParameters.find("BadThresholdFractionGoodCellsSupermodule");
  if (nBadThresholdFractionGoodCellsSupermodule != mCustomParameters.end()) {
    try {
      mBadThresholdFractionGoodCellsSupermodule = std::stof(nBadThresholdFractionGoodCellsSupermodule->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << fmt::format("Value {} not a float", nBadThresholdFractionGoodCellsSupermodule->second.data()) << ENDM;
    }
  }

  auto nBadThresholdCellTimeSupermoduleCalibPHYS = mCustomParameters.find("BadThresholdCellTimeSupermoduleCalibPHYS");
  if (nBadThresholdCellTimeSupermoduleCalibPHYS != mCustomParameters.end()) {
    try {
      mBadThresholdCellTimeSupermoduleCalibPHYS = std::stof(nBadThresholdCellTimeSupermoduleCalibPHYS->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << fmt::format("Value {} not a float", nBadThresholdCellTimeSupermoduleCalibPHYS->second.data()) << ENDM;
    }
  }

  // configure threshold-based checkers for medium quality
  auto nMedThresholdMaskStatsAll = mCustomParameters.find("MedThresholdMaskStatsAll");
  if (nMedThresholdMaskStatsAll != mCustomParameters.end()) {
    try {
      mMedThresholdMaskStatsAll = std::stof(nMedThresholdMaskStatsAll->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << fmt::format("Value {} not a float", nMedThresholdMaskStatsAll->second.data()) << ENDM;
    }
  }

  auto nMedThresholdTimeCalibCoeff = mCustomParameters.find("MedThresholdTimeCalibCoeff");
  if (nMedThresholdTimeCalibCoeff != mCustomParameters.end()) {
    try {
      mMedThresholdTimeCalibCoeff = std::stof(nMedThresholdTimeCalibCoeff->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << fmt::format("Value {} not a float", nMedThresholdTimeCalibCoeff->second.data()) << ENDM;
    }
  }

  auto nMedThresholdCellAmplitudeSupermoduleCalibPHYS = mCustomParameters.find("MedThresholdCellAmplitudeSupermoduleCalibPHYS");
  if (nMedThresholdCellAmplitudeSupermoduleCalibPHYS != mCustomParameters.end()) {
    try {
      mMedThresholdCellAmplitudeSupermoduleCalibPHYS = std::stof(nMedThresholdCellAmplitudeSupermoduleCalibPHYS->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << fmt::format("Value {} not a float", nMedThresholdCellAmplitudeSupermoduleCalibPHYS->second.data()) << ENDM;
    }
  }

  auto nMedThresholdFractionGoodCellsEvent = mCustomParameters.find("MedThresholdFractionGoodCellsEvent");
  if (nMedThresholdFractionGoodCellsEvent != mCustomParameters.end()) {
    try {
      mMedThresholdFractionGoodCellsEvent = std::stof(nMedThresholdFractionGoodCellsEvent->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << fmt::format("Value {} not a float", nMedThresholdFractionGoodCellsEvent->second.data()) << ENDM;
    }
  }

  auto nMedThresholdFractionGoodCellsSupermodule = mCustomParameters.find("MedThresholdFractionGoodCellsSupermodule");
  if (nMedThresholdFractionGoodCellsSupermodule != mCustomParameters.end()) {
    try {
      mMedThresholdFractionGoodCellsSupermodule = std::stof(nMedThresholdFractionGoodCellsSupermodule->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << fmt::format("Value {} not a float", nMedThresholdFractionGoodCellsSupermodule->second.data()) << ENDM;
    }
  }

  auto nMedThresholdCellTimeSupermoduleCalibPHYS = mCustomParameters.find("MedThresholdCellTimeSupermoduleCalibPHYS");
  if (nMedThresholdCellTimeSupermoduleCalibPHYS != mCustomParameters.end()) {
    try {
      mMedThresholdCellTimeSupermoduleCalibPHYS = std::stof(nMedThresholdCellTimeSupermoduleCalibPHYS->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << fmt::format("Value {} not a float", nMedThresholdCellTimeSupermoduleCalibPHYS->second.data()) << ENDM;
    }
  }

  //configure nsigma-based checkers
  auto nSigmaTimeCalibCoeff = mCustomParameters.find("SigmaTimeCalibCoeff");
  if (nSigmaTimeCalibCoeff != mCustomParameters.end()) {
    try {
      mSigmaTimeCalibCoeff = std::stof(nSigmaTimeCalibCoeff->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << fmt::format("Value {} not a float", nSigmaTimeCalibCoeff->second.data()) << ENDM;
    }
  }

}

Quality CalibCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  auto mo = moMap->begin()->second;
  Quality result = Quality::Good;
  std::cout << "mo->getName() is " << mo->getName() << std::endl; //CHANGE REMOVE ALL COUTS BEFORE PUSHING

  if (mo->getName() == "MaskStatsAllHisto") {
    std::cout << "WOW IT WORKS!" << std::endl;
    auto* h = dynamic_cast<TH1*>(mo->getObject());
    if (h->GetEntries() == 0) {
      result = Quality::Medium; //CHECK IF 0 ENTRIES SHOULD ALWAYS BE MEDIUM
    }
    else {
      int bad_bins_total = 0;
      if (h->GetNbinsX() >= 3) bad_bins_total = h->GetBinContent(2)+h->GetBinContent(3); 

      Float_t bad_bins_fraction = (float)bad_bins_total/(float)(h->GetEntries());
      std::cout << "bad_bins_fraction is " << bad_bins_fraction << std::endl;

      std::cout << "mBadThresholdMaskStatsAll is " << mBadThresholdMaskStatsAll << std::endl;
      if (bad_bins_fraction > mBadThresholdMaskStatsAll) result = Quality::Bad;
      else if (bad_bins_fraction > mMedThresholdMaskStatsAll) result = Quality::Medium;
    }
  }

  if (mo->getName() == "timeCalibCoeff") {
    std::cout << "WOW IT WORKS 4!" << std::endl;
    auto* h = dynamic_cast<TH1*>(mo->getObject());
    if (h->GetEntries() == 0) {
      result = Quality::Medium; //CHECK IF 0 ENTRIES SHOULD ALWAYS BE MEDIUM
    }
    else {
      std::vector<double> smcounts;
      for (auto ib : ROOT::TSeqI(0, h->GetXaxis()->GetNbins())) {
        auto countSM = h->GetBinContent(ib + 1);
        if (countSM > 0)
          smcounts.emplace_back(countSM);
      }
      if (!smcounts.size()) {
        result = Quality::Medium; //CHECK IF 0 ENTRIES SHOULD ALWAYS BE MEDIUM
      } else {
        TRobustEstimator meanfinder;
        double mean, sigma;
        meanfinder.EvaluateUni(smcounts.size(), smcounts.data(), mean, sigma);
        int outside_counts = 0.0;
        for (auto ib : ROOT::TSeqI(0, h->GetXaxis()->GetNbins())) {
          if (h->GetBinContent(ib + 1) > (mean + mSigmaTimeCalibCoeff * sigma) || h->GetBinContent(ib + 1) < (mean - mSigmaTimeCalibCoeff * sigma)) {
            outside_counts += 1;
          }
        }
        Float_t out_counts_frac = (float)(outside_counts)/(float)(h->GetEntries());

        std::cout << "out_counts_frac is " << out_counts_frac << std::endl;
        std::cout << "mBadThresholdTimeCalibCoeff is " << mBadThresholdTimeCalibCoeff << std::endl;
        std::cout << "mSigmaTimeCalibCoeff is " << mSigmaTimeCalibCoeff << std::endl;

        if (out_counts_frac > mBadThresholdTimeCalibCoeff) result = Quality::Bad;
        else if (out_counts_frac > mMedThresholdTimeCalibCoeff) result = Quality::Medium;
      }
    }
  }

  if (mo->getName() == "fractionGoodCellsEvent") {
    std::cout << "WOW IT WORKS 2!" << std::endl;
    auto* h = dynamic_cast<TH2*>(mo->getObject());
    if (h->GetEntries() == 0) {
      result = Quality::Medium; //CHECK IF 0 ENTRIES SHOULD ALWAYS BE MEDIUM
    }
    else {
      for (int j=1; j<=h->GetNbinsX(); j++){
        auto* h_det = h->ProjectionY(Form("h_det_%d",j),j,j);
        if (h_det->GetEntries() == 0)
          continue; //CHECK IF 0 ENTRIES MEANS JUST SKIP
        
        Float_t avg_frac = 0.0;
        for (int i=1; i<=h_det->GetNbinsX(); i++){
          Float_t weight = h_det->GetBinContent(i)*h_det->GetBinCenter(i);
          avg_frac += h_det->GetBinContent(i)*h_det->GetBinCenter(i);
        }
        avg_frac = avg_frac/(h_det->GetEntries());
        cout << "avg_frac is " << avg_frac << endl;
        cout << "mBadThresholdFractionGoodCellsEvent is " << mBadThresholdFractionGoodCellsEvent << endl;

        if (avg_frac < mBadThresholdFractionGoodCellsEvent) {
          result = Quality::Bad; //CHECK IF ANY ARE BAD THEN THE HISTO IS BAD
          break;
        }
        else if (avg_frac < mMedThresholdFractionGoodCellsEvent) result = Quality::Medium;
      }
    }
  }

  if (mo->getName() == "fractionGoodCellsSupermodule") {
    std::cout << "WOW IT WORKS 3!" << std::endl;
    auto* h = dynamic_cast<TH2*>(mo->getObject());
    if (h->GetEntries() == 0) {
      result = Quality::Medium; //CHECK IF 0 ENTRIES SHOULD ALWAYS BE MEDIUM
    }
    else {
      for (int j=1; j<=h->GetNbinsX(); j++){
        auto* h_supermod = h->ProjectionY(Form("h_supermod_%d",j),j,j);
        if (h_supermod->GetEntries() == 0)
          continue; //CHECK IF 0 ENTRIES MEANS JUST SKIP
        
        Float_t avg_frac = 0.0;
        for (int i=1; i<=h_supermod->GetNbinsX(); i++){
          Float_t weight = h_supermod->GetBinContent(i)*h_supermod->GetBinCenter(i);
          avg_frac += h_supermod->GetBinContent(i)*h_supermod->GetBinCenter(i);
        }
        avg_frac = avg_frac/(h_supermod->GetEntries());
        cout << "avg_frac is " << avg_frac << endl;
        cout << "mBadThresholdFractionGoodCellsSupermodule is " << mBadThresholdFractionGoodCellsSupermodule << endl;

        if (avg_frac < mBadThresholdFractionGoodCellsSupermodule) {
          result = Quality::Bad;
          break;
        }
        else if (avg_frac < mMedThresholdFractionGoodCellsSupermodule) {
          result = Quality::Medium;
        }
      }
    }
  }

  if (mo->getName() == "cellAmplitudeSupermoduleCalib_PHYS"){
    std::cout << "WOW IT WORKS 5!" << std::endl;
    auto* h = dynamic_cast<TH2*>(mo->getObject());
    if (h->GetEntries() == 0) {
      result = Quality::Medium;
    }
    else {
      auto* h_allsupermod_proj = h->ProjectionX("h_allsupermod_proj");
      h_allsupermod_proj->Scale(1.0/h_allsupermod_proj->Integral());
      for (int i=1; i<=h->GetNbinsY(); i++){
        auto* h_supermod = h->ProjectionX(Form("h_supermod_%d",i),i,i);
        if (h_supermod->GetEntries() == 0) {
          result = Quality::Medium; //CHECK IF 0 ENTRIES MEANS MEDIUM QUALITY
        }
        else {
          h_supermod->Scale(1.0/h_supermod->Integral());
          std::cout << "ACTUAL Entires is " << h_supermod->GetEntries() << std::endl;

          Double_t chi2_SM = -1000;
          chi2_SM = h_supermod->Chi2Test(h_allsupermod_proj,"UU NORM P CHI2/NDF"); //CHECK IF COUT WHEN EMPTY IS FINE
          std::cout << "chi2_SM is " << chi2_SM << std::endl;
          std::cout << "mBadThresholdCellAmplitudeSupermoduleCalibPHYS is " << mBadThresholdCellAmplitudeSupermoduleCalibPHYS << std::endl;
          if (chi2_SM > mBadThresholdCellAmplitudeSupermoduleCalibPHYS) {
            result = Quality::Bad;
            break;
          }
          else if (chi2_SM > mMedThresholdCellAmplitudeSupermoduleCalibPHYS) {
            result = Quality::Medium;
          }
        }
      }
    }
  }

  if (mo->getName() == "cellTimeSupermoduleCalib_PHYS") {
    std::cout << "WOW IT WORKS 6!" << std::endl;
    auto* h = dynamic_cast<TH2*>(mo->getObject());
    if (h->GetEntries() == 0) {
      result = Quality::Medium;
    }
    else {
      auto* h_allsupermod_proj = h->ProjectionX("h_supermod_proj");
      if (h_allsupermod_proj->GetEntries() == 0) {
        result = Quality::Medium; //CHECK IF 0 ENTRIES MEANS MEDIUM QUALITY
      }
      else {
        Double_t mean_allsupermod = h_allsupermod_proj->GetMean();
        std::cout << "mean_allsupermod is " << mean_allsupermod << std::endl;
        std::cout << "mBadThresholdCellTimeSupermoduleCalibPHYS is " << mBadThresholdCellTimeSupermoduleCalibPHYS << endl;
        if (mean_allsupermod > mBadThresholdCellTimeSupermoduleCalibPHYS) {
          result = Quality::Bad;
        }
        else if (mean_allsupermod > mMedThresholdCellTimeSupermoduleCalibPHYS) {
          result = Quality::Medium;
        }

        for (int j=1; j<=h->GetNbinsY(); j++){
          auto* h_supermod = h->ProjectionY(Form("h_supermod_%d",j),j,j);
          if (h_supermod->GetEntries() == 0) {
            result = Quality::Medium; //CHECK IF 0 ENTRIES MEANS MEDIUM QUALITY
          }
          else {
            Double_t mean_isupermod = h_supermod->GetMean();
            std::cout << "mean_isupermod is " << mean_isupermod << std::endl;
            if (mean_isupermod > mBadThresholdCellTimeSupermoduleCalibPHYS) {
              result = Quality::Bad;
              break;
            }
            else if (mean_isupermod > mMedThresholdCellTimeSupermoduleCalibPHYS) {
              result = Quality::Medium;
            }
          }
        }
      }
    }
  }
  return result;
}

std::string CalibCheck::getAcceptedType() { return "TH1"; } //CHECK IF THERE SHOULD BE DIFFERENT CHECKER FILES FOR TH1 v TH2

void CalibCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  //To do
}
} // namespace o2::quality_control_modules::emcal
