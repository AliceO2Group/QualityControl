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
/// \file   PedestalsCheck.cxx
/// \author Andrea Ferrero
///

#include "MCH/PedestalsCheck.h"
#include "MCHMappingInterface/Segmentation.h"
#include "MCHMappingSegContour/CathodeSegmentationContours.h"

#include <fairlogger/Logger.h>
#include <TH1.h>
#include <TH2.h>
#include <TCanvas.h>
#include <TLine.h>
#include <TList.h>
#include <TMath.h>
#include <TPaveText.h>
#include <TColor.h>

using namespace std;

namespace o2::quality_control_modules::muonchambers
{

void PedestalsCheck::configure()
{
  if (auto param = mCustomParameters.find("MaxBadDE_ST12"); param != mCustomParameters.end()) {
    mMaxBadST12 = std::stoi(param->second);
  }
  if (auto param = mCustomParameters.find("MaxBadDE_ST345"); param != mCustomParameters.end()) {
    mMaxBadST345 = std::stoi(param->second);
  }
  if (auto param = mCustomParameters.find("MaxBadFractionPerDE"); param != mCustomParameters.end()) {
    mMaxBadFractionPerDE = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("MaxEmptyFractionPerDE"); param != mCustomParameters.end()) {
    mMaxEmptyFractionPerDE = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("MinStatisticsPerDE"); param != mCustomParameters.end()) {
    mMinStatisticsPerDE = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("PedestalsPlotScaleMin"); param != mCustomParameters.end()) {
    mPedestalsPlotScaleMin = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("PedestalsPlotScaleMax"); param != mCustomParameters.end()) {
    mPedestalsPlotScaleMax = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("NoisePlotScaleMin"); param != mCustomParameters.end()) {
    mNoisePlotScaleMin = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("NoisePlotScaleMax"); param != mCustomParameters.end()) {
    mNoisePlotScaleMax = std::stof(param->second);
  }

  mQualityChecker.mMaxBadST12 = mMaxBadST12;
  mQualityChecker.mMaxBadST345 = mMaxBadST345;
}

Quality PedestalsCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality resultTable = Quality::Null;
  Quality result = Quality::Null;

  mQualityBadChannels = Quality::Null;
  std::array<Quality, getNumDE()> qualityBadChannelsDE;
  QualityChecker qualityCheckerBadChannels;
  qualityCheckerBadChannels.mMaxBadST12 = mMaxBadST12;
  qualityCheckerBadChannels.mMaxBadST345 = mMaxBadST345;

  mQualityEmptyChannels = Quality::Null;
  std::array<Quality, getNumDE()> qualityEmptyChannelsDE;
  QualityChecker qualityCheckerEmptyChannels;
  qualityCheckerEmptyChannels.mMaxBadST12 = mMaxBadST12;
  qualityCheckerEmptyChannels.mMaxBadST345 = mMaxBadST345;

  mQualityStatistics = Quality::Null;
  std::array<Quality, getNumDE()> qualityStatisticsDE;
  QualityChecker qualityCheckerStatistics;
  qualityCheckerStatistics.mMaxBadST12 = mMaxBadST12;
  qualityCheckerStatistics.mMaxBadST345 = mMaxBadST345;

  mQualityChecker.reset();

  for (auto& [moName, mo] : *moMap) {
    // check if the bad channels table was received from the calibrator
    if (mo->getName().find("BadChannels_Elec") != std::string::npos) {
      auto* h = dynamic_cast<TH2F*>(mo->getObject());
      if (!h) {
        return result;
      }

      if (h->GetEntries() == 0) {
        resultTable = Quality::Bad;
      } else {
        resultTable = Quality::Good;
      }
    }

    // check fraction of bad channels per detection element
    if (mo->getName().find("BadChannelsPerDE") != std::string::npos) {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      if (!h) {
        return result;
      }

      // initialize the DE qualities to Good
      std::fill(qualityBadChannelsDE.begin(), qualityBadChannelsDE.end(), Quality::Good);
      if (h->GetEntries() == 0) {
        // plot is empty, set quality to Null
        std::fill(qualityBadChannelsDE.begin(), qualityBadChannelsDE.end(), Quality::Null);
      } else {
        int nbinsx = h->GetXaxis()->GetNbins();
        for (int i = 1; i <= nbinsx; i++) {
          // set the DE quality to Bad if the fraction of bad channels is above the threshold
          if (h->GetBinContent(i) > mMaxBadFractionPerDE) {
            qualityBadChannelsDE[i - 1] = Quality::Bad;
          }
        }

        // update the quality checkers
        qualityCheckerBadChannels.addCheckResult(qualityBadChannelsDE);
        mQualityChecker.addCheckResult(qualityBadChannelsDE);
      }
    }

    // check fraction of empty channels per detection element
    if (mo->getName().find("EmptyChannelsPerDE") != std::string::npos) {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      if (!h) {
        return result;
      }

      // initialize the DE qualities to Good
      std::fill(qualityEmptyChannelsDE.begin(), qualityEmptyChannelsDE.end(), Quality::Good);
      if (h->GetEntries() == 0) {
        // plot is empty, set quality to Null
        std::fill(qualityEmptyChannelsDE.begin(), qualityEmptyChannelsDE.end(), Quality::Medium);
      } else {
        int nbinsx = h->GetXaxis()->GetNbins();
        for (int i = 1; i <= nbinsx; i++) {
          // set the DE quality to Bad if the fraction of empty channels is above the threshold
          if (h->GetBinContent(i) > mMaxEmptyFractionPerDE) {
            qualityEmptyChannelsDE[i - 1] = Quality::Bad;
          }
        }
      }

      // update the quality checkers
      qualityCheckerEmptyChannels.addCheckResult(qualityEmptyChannelsDE);
      mQualityChecker.addCheckResult(qualityEmptyChannelsDE);
    }

    // check fraction of empty channels per detection element
    if (mo->getName().find("StatisticsPerDE") != std::string::npos) {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      if (!h) {
        return result;
      }

      // initialize the DE qualities to Good
      std::fill(qualityStatisticsDE.begin(), qualityStatisticsDE.end(), Quality::Good);
      if (h->GetEntries() == 0) {
        // plot is empty, set quality to Null
        std::fill(qualityStatisticsDE.begin(), qualityStatisticsDE.end(), Quality::Medium);
      } else {
        int nbinsx = h->GetXaxis()->GetNbins();
        for (int i = 1; i <= nbinsx; i++) {
          // set the DE quality to Bad if the fraction of empty channels is above the threshold
          if (h->GetBinContent(i) < mMinStatisticsPerDE) {
            qualityStatisticsDE[i - 1] = Quality::Bad;
          }
        }
      }

      // update the quality checkers
      qualityCheckerStatistics.addCheckResult(qualityStatisticsDE);
      mQualityChecker.addCheckResult(qualityStatisticsDE);
    }
  }

  // compute the aggregated quality
  mQualityEmptyChannels = qualityCheckerEmptyChannels.getQuality();
  mQualityBadChannels = qualityCheckerBadChannels.getQuality();
  mQualityStatistics = qualityCheckerStatistics.getQuality();
  result = mQualityChecker.getQuality();
  if (resultTable == Quality::Bad) {
    result = Quality::Bad;
  }

  // update the error messages
  mErrorMessages.clear();
  if (result == Quality::Good) {
    mErrorMessages.emplace_back("Quality: GOOD");
  } else if (result == Quality::Medium) {
    mErrorMessages.emplace_back("Quality: MEDIUM");
    mErrorMessages.emplace_back("Add Logbook entry");
  } else if (result == Quality::Bad) {
    mErrorMessages.emplace_back("Quality: BAD");
    mErrorMessages.emplace_back("Contact MCH on-call");
  } else if (result == Quality::Null) {
    mErrorMessages.emplace_back("Quality: NULL\n");
  }
  mErrorMessages.emplace_back("");

  if (resultTable == Quality::Bad) {
    mErrorMessages.emplace_back("Missing Bad Channels Table");
  }

  if (mQualityEmptyChannels == Quality::Null) {
    mErrorMessages.emplace_back("Empty channels plot not filled");
  } else if (mQualityEmptyChannels == Quality::Bad) {
    mErrorMessages.emplace_back("Too many empty channels");
  } else if (mQualityEmptyChannels == Quality::Medium) {
    mErrorMessages.emplace_back("Some detectors have empty channels");
  }

  if (mQualityBadChannels == Quality::Null) {
    mErrorMessages.emplace_back("Bad channels plot not filled");
  } else if (mQualityBadChannels == Quality::Bad) {
    mErrorMessages.emplace_back("Too many bad channels");
  } else if (mQualityBadChannels == Quality::Medium) {
    mErrorMessages.emplace_back("Some detectors have bad channels");
  }

  if (mQualityStatistics == Quality::Null) {
    mErrorMessages.emplace_back("Statistics plot not filled");
  } else if (mQualityStatistics == Quality::Bad) {
    mErrorMessages.emplace_back("Statistics too small");
  } else if (mQualityStatistics == Quality::Medium) {
    mErrorMessages.emplace_back("Some detectors have small statistics");
  }

  return result;
}

std::string PedestalsCheck::getAcceptedType() { return "TH1"; }

static void updateTitle(TH1* hist, std::string suffix)
{
  if (!hist) {
    return;
  }
  TString title = hist->GetTitle();
  title.Append(" ");
  title.Append(suffix.c_str());
  hist->SetTitle(title);
}

static std::string getCurrentTime()
{
  time_t t;
  time(&t);

  struct tm* tmp;
  tmp = localtime(&t);

  char timestr[500];
  strftime(timestr, sizeof(timestr), "(%x - %X)", tmp);

  std::string result = timestr;
  return result;
}

void PedestalsCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  auto currentTime = getCurrentTime();
  updateTitle(dynamic_cast<TH1*>(mo->getObject()), currentTime);

  if (mo->getName().find("CheckerMessages") != std::string::npos) {
    auto* canvas = dynamic_cast<TCanvas*>(mo->getObject());
    if (!canvas) {
      return;
    }
    canvas->cd();

    TPaveText* msg = new TPaveText(0.1, 0.1, 0.9, 0.9, "NDC");
    for (auto s : mErrorMessages) {
      msg->AddText(s.c_str());
    }
    if (checkResult == Quality::Good) {
      msg->SetTextColor(kGreen + 2);
    }
    if (checkResult == Quality::Medium) {
      msg->SetTextColor(kOrange);
    }
    if (checkResult == Quality::Bad) {
      msg->SetTextColor(kRed);
    }
    if (checkResult == Quality::Null) {
      msg->SetTextColor(kBlack);
    }
    msg->SetBorderSize(0);
    msg->SetFillColor(kWhite);
    msg->Draw();
  }

  if (mo->getName().find("EmptyChannelsPerDE") != std::string::npos) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());

    h->SetMinimum(0);
    h->SetMaximum(1.1);

    addChamberDelimiters(h, 0, 1.1);

    TLine* delimiter = new TLine(h->GetXaxis()->GetXmin(), mMaxEmptyFractionPerDE, h->GetXaxis()->GetXmax(), mMaxEmptyFractionPerDE);
    delimiter->SetLineColor(kBlack);
    delimiter->SetLineStyle(kDashed);
    h->GetListOfFunctions()->Add(delimiter);

    if (mQualityEmptyChannels == Quality::Good) {
      h->SetFillColor(kGreen);
    } else if (mQualityEmptyChannels == Quality::Bad) {
      h->SetFillColor(kRed);
    } else if (mQualityEmptyChannels == Quality::Medium) {
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }

  if (mo->getName().find("BadChannelsPerDE") != std::string::npos) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());

    h->SetMinimum(0);
    h->SetMaximum(1.1);

    addChamberDelimiters(h, 0, 1.1);

    TLine* delimiter = new TLine(h->GetXaxis()->GetXmin(), mMaxBadFractionPerDE, h->GetXaxis()->GetXmax(), mMaxBadFractionPerDE);
    delimiter->SetLineColor(kBlack);
    delimiter->SetLineStyle(kDashed);
    h->GetListOfFunctions()->Add(delimiter);

    if (mQualityBadChannels == Quality::Good) {
      h->SetFillColor(kGreen);
    } else if (mQualityBadChannels == Quality::Bad) {
      h->SetFillColor(kRed);
    } else if (mQualityBadChannels == Quality::Medium) {
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }

  if (mo->getName().find("StatisticsPerDE") != std::string::npos) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());

    auto min = h->GetMinimum();
    auto max = 1.05 * h->GetMaximum();

    if (max < mMinStatisticsPerDE) {
      max = 1.05 * mMinStatisticsPerDE;
    }
    h->SetMinimum(min);
    h->SetMaximum(max);

    addChamberDelimiters(h, min, max);

    TLine* delimiter = new TLine(h->GetXaxis()->GetXmin(), mMinStatisticsPerDE, h->GetXaxis()->GetXmax(), mMinStatisticsPerDE);
    delimiter->SetLineColor(kBlack);
    delimiter->SetLineStyle(kDashed);
    h->GetListOfFunctions()->Add(delimiter);

    if (mQualityStatistics == Quality::Good) {
      h->SetFillColor(kGreen);
    } else if (mQualityStatistics == Quality::Bad) {
      h->SetFillColor(kRed);
    } else if (mQualityStatistics == Quality::Medium) {
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }

  if (mo->getName().find("PedestalsPerDE") != std::string::npos ||
      mo->getName().find("NoisePerDE") != std::string::npos) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());

    auto min = 0;
    auto max = 1.05 * h->GetMaximum();
    h->SetMinimum(min);
    h->SetMaximum(max);

    addChamberDelimiters(h, min, max);
  }

  if (mo->getName().find("Pedestals_Elec") != std::string::npos) {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());

    h->SetMinimum(mPedestalsPlotScaleMin);
    h->SetMaximum(mPedestalsPlotScaleMax);
  }

  if (mo->getName().find("Noise_Elec") != std::string::npos) {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());

    h->SetMinimum(mNoisePlotScaleMin);
    h->SetMaximum(mNoisePlotScaleMax);
  }

  if ((mo->getName().find("Pedestals_ST12") != std::string::npos) ||
      (mo->getName().find("Pedestals_ST345") != std::string::npos)) {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    h->SetMinimum(mPedestalsPlotScaleMin);
    h->SetMaximum(mPedestalsPlotScaleMax);
    h->GetXaxis()->SetTickLength(0.0);
    h->GetXaxis()->SetLabelSize(0.0);
    h->GetYaxis()->SetTickLength(0.0);
    h->GetYaxis()->SetLabelSize(0.0);
  }

  if ((mo->getName().find("Noise_ST12") != std::string::npos) ||
      (mo->getName().find("Noise_ST345") != std::string::npos)) {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    h->SetMinimum(mNoisePlotScaleMin);
    h->SetMaximum(mNoisePlotScaleMax);
    h->GetXaxis()->SetTickLength(0.0);
    h->GetXaxis()->SetLabelSize(0.0);
    h->GetYaxis()->SetTickLength(0.0);
    h->GetYaxis()->SetLabelSize(0.0);
  }

  if ((mo->getName().find("BadChannels_ST12") != std::string::npos) ||
      (mo->getName().find("BadChannels_ST345") != std::string::npos)) {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    double min = 1;
    int nbinsx = h->GetXaxis()->GetNbins();
    int nbinsy = h->GetYaxis()->GetNbins();
    for (int i = 1; i <= nbinsx; i++) {
      for (int j = 1; j <= nbinsy; j++) {
        auto value = h->GetBinContent(i, j);
        if (value > 0 && value < min) {
          min = value;
        }
      }
    }
    h->SetMinimum(0.99 * min);
    h->GetXaxis()->SetTickLength(0.0);
    h->GetXaxis()->SetLabelSize(0.0);
    h->GetYaxis()->SetTickLength(0.0);
    h->GetYaxis()->SetLabelSize(0.0);
  }

  if (mo->getName().find("Pedestals_XY") != std::string::npos) {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    h->SetMinimum(mPedestalsPlotScaleMin);
    h->SetMaximum(mPedestalsPlotScaleMax);
    h->GetXaxis()->SetTickLength(0.0);
    h->GetXaxis()->SetLabelSize(0.0);
    h->GetYaxis()->SetTickLength(0.0);
    h->GetYaxis()->SetLabelSize(0.0);
  }

  if (mo->getName().find("Noise_XY") != std::string::npos) {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    h->SetMinimum(mNoisePlotScaleMin);
    h->SetMaximum(mNoisePlotScaleMax);
    h->GetXaxis()->SetTickLength(0.0);
    h->GetXaxis()->SetLabelSize(0.0);
    h->GetYaxis()->SetTickLength(0.0);
    h->GetYaxis()->SetLabelSize(0.0);
  }

  if (mo->getName().find("BadChannels_XY") != std::string::npos) {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    h->SetMinimum(0);
    h->SetMaximum(1);
    h->GetXaxis()->SetTickLength(0.0);
    h->GetXaxis()->SetLabelSize(0.0);
    h->GetYaxis()->SetTickLength(0.0);
    h->GetYaxis()->SetLabelSize(0.0);
  }
}

} // namespace o2::quality_control_modules::muonchambers
