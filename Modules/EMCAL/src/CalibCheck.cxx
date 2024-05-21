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
#include <TMath.h>
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

  auto nBadThresholdChannelsFEC = mCustomParameters.find("BadThresholdChannelsFEC");
  if (nBadThresholdChannelsFEC != mCustomParameters.end()) {
    try {
      mBadThresholdChannelsFEC = std::stof(nBadThresholdChannelsFEC->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << fmt::format("Value {} not a float", nBadThresholdChannelsFEC->second.data()) << ENDM;
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

  auto nMedThresholdChannelsFEC = mCustomParameters.find("MedThresholdChannelsFEC");
  if (nMedThresholdChannelsFEC != mCustomParameters.end()) {
    try {
      mMedThresholdChannelsFEC = std::stof(nMedThresholdChannelsFEC->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << fmt::format("Value {} not a float", nMedThresholdChannelsFEC->second.data()) << ENDM;
    }
  }

  // configure nsigma-based checkers
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

  if (mo->getName() == "MaskStatsAllHisto" || mo->getName() == "MaskStatsEMCALHisto" || mo->getName() == "MaskStatsDCALHisto") {
    auto* h = dynamic_cast<TH1*>(mo->getObject());
    if (h->GetEntries() == 0) {
      result = Quality::Medium;
    } else {
      int bad_bins_total = 0;
      if (h->GetNbinsX() >= 3) {
        bad_bins_total = h->GetBinContent(2) + h->GetBinContent(3);
      }

      Float_t bad_bins_fraction = (float)bad_bins_total / (float)(h->GetEntries());

      if (bad_bins_fraction > mBadThresholdMaskStatsAll) {
        result = Quality::Bad;
      } else if (bad_bins_fraction > mMedThresholdMaskStatsAll) {
        result = Quality::Medium;
      }
    }
  }

  if (mo->getName() == "timeCalibCoeff") {
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
        for (auto ib : ROOT::TSeqI(0, h->GetXaxis()->GetNbins())) {
          if (h->GetBinContent(ib + 1) > (mean + mSigmaTimeCalibCoeff * sigma) || h->GetBinContent(ib + 1) < (mean - mSigmaTimeCalibCoeff * sigma)) {
            outside_counts += 1;
          }
        }
        Float_t out_counts_frac = (float)(outside_counts) / (float)(h->GetEntries());

        if (out_counts_frac > mBadThresholdTimeCalibCoeff) {
          result = Quality::Bad;
        } else if (out_counts_frac > mMedThresholdTimeCalibCoeff) {
          result = Quality::Medium;
        }
      }
    }
  }

  if (mo->getName() == "fractionGoodCellsEvent") {
    auto* h = dynamic_cast<TH2*>(mo->getObject());
    if (h->GetEntries() == 0) {
      result = Quality::Medium;
    } else {
      for (int j = 1; j <= h->GetNbinsX(); j++) {
        std::unique_ptr<TH1> h_det(h->ProjectionY(Form("h_det_%d", j), j, j));
        if (h_det->GetEntries() == 0) {
          continue;
        }

        Float_t avg_frac = 0.0;
        for (int i = 1; i <= h_det->GetNbinsX(); i++) {
          avg_frac += h_det->GetBinContent(i) * h_det->GetBinCenter(i);
        }
        avg_frac = avg_frac / (h_det->GetEntries());

        if (avg_frac < mBadThresholdFractionGoodCellsEvent) {
          result = Quality::Bad;
          break;
        } else if (avg_frac < mMedThresholdFractionGoodCellsEvent) {
          result = Quality::Medium;
        }
      }
    }
  }

  if (mo->getName() == "fractionGoodCellsSupermodule") {
    auto* h = dynamic_cast<TH2*>(mo->getObject());
    if (h->GetEntries() == 0) {
      result = Quality::Medium;
    } else {
      for (int j = 1; j <= h->GetNbinsX(); j++) {
        std::unique_ptr<TH1> h_supermod(h->ProjectionY(Form("h_supermod_%d", j), j, j));
        if (h_supermod->GetEntries() == 0) {
          continue;
        }

        Float_t avg_frac = 0.0;
        for (int i = 1; i <= h_supermod->GetNbinsX(); i++) {
          avg_frac += h_supermod->GetBinContent(i) * h_supermod->GetBinCenter(i);
        }
        avg_frac = avg_frac / (h_supermod->GetEntries());

        if (avg_frac < mBadThresholdFractionGoodCellsSupermodule) {
          result = Quality::Bad;
          break;
        } else if (avg_frac < mMedThresholdFractionGoodCellsSupermodule) {
          result = Quality::Medium;
        }
      }
    }
  }

  if (mo->getName() == "cellAmplitudeSupermoduleCalib_PHYS") {
    auto* h = dynamic_cast<TH2*>(mo->getObject());
    if (h->GetEntries() == 0) {
      result = Quality::Medium;
    } else {
      std::unique_ptr<TH1> h_allsupermod_proj(h->ProjectionX("h_allsupermod_proj"));
      h_allsupermod_proj->Scale(1.0 / h_allsupermod_proj->Integral());
      for (int i = 1; i <= h->GetNbinsY(); i++) {
        std::unique_ptr<TH1> h_supermod(h->ProjectionX(Form("h_supermod_%d", i), i, i));
        if (h_supermod->GetEntries() == 0) {
          result = Quality::Medium;
        } else {
          h_supermod->Scale(1.0 / h_supermod->Integral());

          Double_t chi2_SM = h_supermod->Chi2Test(h_allsupermod_proj.get(), "UU NORM CHI2/NDF");
          if (chi2_SM > mBadThresholdCellAmplitudeSupermoduleCalibPHYS) {
            result = Quality::Bad;
            break;
          } else if (chi2_SM > mMedThresholdCellAmplitudeSupermoduleCalibPHYS) {
            result = Quality::Medium;
          }
        }
      }
    }
  }

  if (mo->getName() == "cellTimeSupermoduleCalib_PHYS") {
    auto* h = dynamic_cast<TH2*>(mo->getObject());
    if (h->GetEntries() == 0) {
      result = Quality::Medium;
    } else {
      std::unique_ptr<TH1> h_allsupermod_proj(h->ProjectionX("h_supermod_proj"));
      if (h_allsupermod_proj->GetEntries() == 0) {
        result = Quality::Medium;
      } else {
        Double_t mean_allsupermod = h_allsupermod_proj->GetMean();
        if (TMath::Abs(mean_allsupermod) > mBadThresholdCellTimeSupermoduleCalibPHYS) {
          result = Quality::Bad;
        } else if (TMath::Abs(mean_allsupermod) > mMedThresholdCellTimeSupermoduleCalibPHYS) {
          result = Quality::Medium;
        }

        for (int j = 1; j <= h->GetNbinsY(); j++) {
          std::unique_ptr<TH1> h_supermod(h->ProjectionY(Form("h_supermod_%d", j), j, j));
          if (h_supermod->GetEntries() == 0) {
            result = Quality::Medium;
          } else {
            Double_t mean_isupermod = h_supermod->GetMean();
            if (TMath::Abs(mean_isupermod) > mBadThresholdCellTimeSupermoduleCalibPHYS) {
              result = Quality::Bad;
              break;
            } else if (TMath::Abs(mean_isupermod) > mMedThresholdCellTimeSupermoduleCalibPHYS) {
              result = Quality::Medium;
            }
          }
        }
      }
    }
  }

  if (mo->getName() == "NumberNonGoodChannelsFEC" || mo->getName() == "NumberDeadChannelsFEC" || mo->getName() == "NumberBadChannelsFEC") {
    auto* h = dynamic_cast<TH2*>(mo->getObject());
    if (h->GetEntries() == 0) {
      result = Quality::Medium;
    } else {
      std::unique_ptr<TH1> h_allsupermod_proj(h->ProjectionX("h_allsupermod_proj"));
      for (int iFECId = 1; iFECId <= h->GetNbinsX(); iFECId++) {
        Float_t numChannelsAllSMs = h_allsupermod_proj->GetBinContent(iFECId);
        if (numChannelsAllSMs > mBadThresholdChannelsFEC) {
          result = Quality::Bad;
          break;
        } else if (numChannelsAllSMs > mMedThresholdChannelsFEC) {
          result = Quality::Medium;
        }
      }
    }
  }

  return result;
}

std::string CalibCheck::getAcceptedType() { return "TH1"; }

void CalibCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "MaskStatsAllHisto" || mo->getName() == "MaskStatsEMCALHisto" || mo->getName() == "MaskStatsDCALHisto") {
    auto* h = dynamic_cast<TH1*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.12, 0.84, 0.88, 0.94, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));
    if (checkResult == Quality::Good) {
      msg->Clear();
      msg->AddText("Data OK: Fraction of Bad+Dead Channels Low");
      msg->SetFillColor(kGreen);
      msg->Draw();
    } else if (checkResult == Quality::Bad) {
      ILOG(Debug, Devel) << "Quality::Bad, setting to red";
      stringstream msg_bad;
      msg_bad << "Error: Fraction of Bad+Dead Channels Above " << mBadThresholdMaskStatsAll << std::endl;
      msg->Clear();
      msg->AddText(msg_bad.str().c_str());
      msg->AddText("Call EMCAL oncall");
      msg->SetFillColor(kRed);
      msg->Draw();

    } else if (checkResult == Quality::Medium) {
      ILOG(Debug, Devel) << "Quality::medium, setting to orange";
      stringstream msg_med;
      msg_med << "Warning: Plot Empty or Fraction of Bad+Dead Channels Above " << mMedThresholdMaskStatsAll << std::endl;
      msg->Clear();
      msg->AddText(msg_med.str().c_str());
      msg->SetFillColor(kOrange);
      msg->Draw();
    }
  }

  if (mo->getName() == "timeCalibCoeff") {
    auto* h = dynamic_cast<TH1*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.12, 0.84, 0.88, 0.94, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));
    if (checkResult == Quality::Good) {
      msg->Clear();
      msg->AddText("Data OK: Fraction of Times Outside Mean Low");
      msg->SetFillColor(kGreen);
      msg->Draw();
    } else if (checkResult == Quality::Bad) {
      ILOG(Debug, Devel) << "Quality::Bad, setting to red";
      stringstream msg_bad;
      msg_bad << "Error: Fraction of Times Outside Mean Above " << mBadThresholdTimeCalibCoeff << std::endl;
      msg->Clear();
      msg->AddText(msg_bad.str().c_str());
      msg->AddText("Call EMCAL oncall");
      msg->SetFillColor(kRed);
      msg->Draw();
    } else if (checkResult == Quality::Medium) {
      ILOG(Debug, Devel) << "Quality::medium, setting to orange";
      stringstream msg_med;
      msg_med << "Warning: Plot Empty or Fraction of Times Outside Mean Above " << mMedThresholdTimeCalibCoeff << std::endl;
      msg->Clear();
      msg->AddText(msg_med.str().c_str());
      msg->SetFillColor(kOrange);
      msg->Draw();
    }
  }

  if (mo->getName() == "fractionGoodCellsEvent") {
    auto* h = dynamic_cast<TH2*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.12, 0.84, 0.88, 0.94, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));
    if (checkResult == Quality::Good) {
      msg->Clear();
      msg->AddText("Data OK: Fraction of Good Cells High for All Detectors");
      msg->SetFillColor(kGreen);
      msg->Draw();
    } else if (checkResult == Quality::Bad) {
      ILOG(Debug, Devel) << "Quality::Bad, setting to red";
      stringstream msg_bad;
      msg_bad << "Error: 1+ Detectors Have Fraction of Good Cells Below " << mBadThresholdFractionGoodCellsEvent << std::endl;
      msg->Clear();
      msg->AddText(msg_bad.str().c_str());
      msg->AddText("Call EMCAL oncall");
      msg->SetFillColor(kRed);
      msg->Draw();
    } else if (checkResult == Quality::Medium) {
      ILOG(Debug, Devel) << "Quality::medium, setting to orange";
      stringstream msg_med;
      msg_med << "Warning: Plot Empty or 1+ Detectors Have Fraction of Good Cells Below " << mMedThresholdFractionGoodCellsEvent << std::endl;
      msg->Clear();
      msg->AddText(msg_med.str().c_str());
      msg->SetFillColor(kOrange);
      msg->Draw();
    }
  }

  if (mo->getName() == "fractionGoodCellsSupermodule") {
    auto* h = dynamic_cast<TH2*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.12, 0.84, 0.88, 0.94, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));
    if (checkResult == Quality::Good) {
      msg->Clear();
      msg->AddText("Data OK: Fraction of Good Cells High for All Supermodules");
      msg->SetFillColor(kGreen);
      msg->Draw();
    } else if (checkResult == Quality::Bad) {
      ILOG(Debug, Devel) << "Quality::Bad, setting to red";
      stringstream msg_bad;
      msg_bad << "Error: 1+ Supermodules Have Fraction of Good Cells Below " << mBadThresholdFractionGoodCellsSupermodule << std::endl;
      msg->Clear();
      msg->AddText(msg_bad.str().c_str());
      msg->AddText("Call EMCAL oncall");
      msg->SetFillColor(kRed);
      msg->Draw();
    } else if (checkResult == Quality::Medium) {
      ILOG(Debug, Devel) << "Quality::medium, setting to orange";
      stringstream msg_med;
      msg_med << "Warning: Plot Empty or 1+ Supermodules Have Fraction of Good Cells Below " << mMedThresholdFractionGoodCellsSupermodule << std::endl;
      msg->Clear();
      msg->AddText(msg_med.str().c_str());
      msg->SetFillColor(kOrange);
      msg->Draw();
    }
  }

  if (mo->getName() == "cellAmplitudeSupermoduleCalib_PHYS") {
    auto* h = dynamic_cast<TH2*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.12, 0.84, 0.88, 0.94, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));
    if (checkResult == Quality::Good) {
      msg->Clear();
      msg->AddText("Data OK: Cell Amplitude Distributions Similar for All Supermodules");
      msg->SetFillColor(kGreen);
      msg->Draw();
    } else if (checkResult == Quality::Bad) {
      ILOG(Debug, Devel) << "Quality::Bad, setting to red";
      stringstream msg_bad;
      msg_bad << "Error: 1+ Supermodules Have Anomalous Cell Amplitude Distributions With Chi2/NDF Above " << mBadThresholdCellAmplitudeSupermoduleCalibPHYS << std::endl;
      msg->Clear();
      msg->AddText(msg_bad.str().c_str());
      msg->AddText("Call EMCAL oncall");
      msg->SetFillColor(kRed);
      msg->Draw();
    } else if (checkResult == Quality::Medium) {
      ILOG(Debug, Devel) << "Quality::medium, setting to orange";
      stringstream msg_med;
      msg_med << "Warning: 1+ Supermodules Empty or 1+ Supermodules Have Anomalous Cell Amplitude Distributions With Chi2/NDF Above " << mMedThresholdCellAmplitudeSupermoduleCalibPHYS << std::endl;
      msg->Clear();
      msg->AddText(msg_med.str().c_str());
      msg->SetFillColor(kOrange);
      msg->Draw();
    }
  }

  if (mo->getName() == "cellTimeSupermoduleCalib_PHYS") {
    auto* h = dynamic_cast<TH2*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.12, 0.84, 0.88, 0.94, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));
    if (checkResult == Quality::Good) {
      msg->Clear();
      msg->AddText("Data OK: Mean Cell Time Within Limits");
      msg->SetFillColor(kGreen);
      msg->Draw();
    } else if (checkResult == Quality::Bad) {
      ILOG(Debug, Devel) << "Quality::Bad, setting to red";
      stringstream msg_bad;
      msg_bad << "Error: 1+ Supermodules Have |Mean Cell Time| Above " << mBadThresholdCellTimeSupermoduleCalibPHYS << std::endl;
      msg->Clear();
      msg->AddText(msg_bad.str().c_str());
      msg->AddText("Call EMCAL oncall");
      msg->SetFillColor(kRed);
      msg->Draw();
    } else if (checkResult == Quality::Medium) {
      ILOG(Debug, Devel) << "Quality::medium, setting to orange";
      stringstream msg_med;
      msg_med << "Warning: 1+ Supermodules Empty or 1+ Supermodules Have |Mean Cell Time| Above " << mMedThresholdCellTimeSupermoduleCalibPHYS << std::endl;
      msg->Clear();
      msg->AddText(msg_med.str().c_str());
      msg->SetFillColor(kOrange);
      msg->Draw();
    }
  }

  if (mo->getName() == "NumberNonGoodChannelsFEC" || mo->getName() == "NumberDeadChannelsFEC" || mo->getName() == "NumberBadChannelsFEC") {
    auto* h = dynamic_cast<TH2*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.12, 0.84, 0.88, 0.94, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));
    if (checkResult == Quality::Good) {
      msg->Clear();
      msg->AddText("Data OK: Number of Dead/Bad/Dead+Bad Channels Within Limit");
      msg->SetFillColor(kGreen);
      msg->Draw();
    } else if (checkResult == Quality::Bad) {
      ILOG(Debug, Devel) << "Quality::Bad, setting to red";
      stringstream msg_bad;
      msg_bad << "Error: 1+ FECs Have Number of Dead/Bad/Dead+Bad Channels Above " << mBadThresholdChannelsFEC << std::endl;
      msg->Clear();
      msg->AddText(msg_bad.str().c_str());
      msg->AddText("Call EMCAL oncall");
      msg->SetFillColor(kRed);
      msg->Draw();
    } else if (checkResult == Quality::Medium) {
      ILOG(Debug, Devel) << "Quality::medium, setting to orange";
      stringstream msg_med;
      msg_med << "Warning: Plot Empty or 1+ FECs Have Number of Dead/Bad/Dead+Bad Channels Above " << mMedThresholdChannelsFEC << std::endl;
      msg->Clear();
      msg->AddText(msg_med.str().c_str());
      msg->SetFillColor(kOrange);
      msg->Draw();
    }
  }
}

} // namespace o2::quality_control_modules::emcal
