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
/// \file   QcMFTReadoutCheck.cxx
/// \author Tomas Herman
/// \author Guillermo Contreras
/// \author Katarina Krizkova Gajdosova
/// \author Diana Maria Krupova

// Fair
#include <fairlogger/Logger.h>
// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TList.h>
#include <TLatex.h>

// Quality Control
#include "MFT/QcMFTReadoutCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"

using namespace std;

namespace o2::quality_control_modules::mft
{

void QcMFTReadoutCheck::configure()
{

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("FaultThresholdMedium"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - FaultThresholdMedium: " << param->second << ENDM;
    mFaultThresholdMedium = stoi(param->second);
  }
  if (auto param = mCustomParameters.find("FaultThresholdBad"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - FaultThresholdBad: " << param->second << ENDM;
    mFaultThresholdBad = stoi(param->second);
  }
  if (auto param = mCustomParameters.find("ErrorThresholdMedium"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - ErrorThresholdMedium: " << param->second << ENDM;
    mErrorThresholdMedium = stoi(param->second);
  }
  if (auto param = mCustomParameters.find("ErrorThresholdBad"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - ErrorThresholdBad: " << param->second << ENDM;
    mErrorThresholdBad = stoi(param->second);
  }
  if (auto param = mCustomParameters.find("WarningThresholdMedium"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - WarningThresholdMedium: " << param->second << ENDM;
    mWarningThresholdMedium = stoi(param->second);
  }
  if (auto param = mCustomParameters.find("WarningThresholdBad"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - WarningThresholdBad: " << param->second << ENDM;
    mWarningThresholdBad = stoi(param->second);
  }
}

Quality QcMFTReadoutCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;

    if (mo->getName() == "mDDWSummary") {
      // get the histogram
      auto* hDDW = dynamic_cast<TH1F*>(mo->getObject());
      // normalise it using the underflow bin as normalisation factor
      float den = hDDW->GetBinContent(0);
      for (int i = 1; i < 4; i++) {
        float num = hDDW->GetBinContent(i);
        float ratio = (den > 0) ? (num / den) : 0.0;
        hDDW->SetBinContent(i, ratio);
      }
    }

    if (mo->getName() == "mSummaryChipOk") {
      auto* hOK = dynamic_cast<TH1F*>(mo->getObject());
      float den = hOK->GetBinContent(0); // normalisation stored in the uderflow bin

      for (int iBin = 0; iBin < hOK->GetNbinsX(); iBin++) {
        float num = hOK->GetBinContent(iBin + 1);
        float ratio = (den > 0) ? (num / den) : 0.0;
        hOK->SetBinContent(iBin + 1, ratio);
      }
    }

    if (mo->getName() == "mSummaryChipFault") {
      resetVector(mVectorOfFaultBins);
      auto* hFault = dynamic_cast<TH1F*>(mo->getObject());
      float den = hFault->GetBinContent(0); // normalisation stored in the uderflow bin

      for (int iBin = 0; iBin < hFault->GetNbinsX(); iBin++) {
        if (hFault->GetBinContent(iBin + 1) != 0) {
          mVectorOfFaultBins.push_back(iBin + 1);
          hFault->Fill(937); // number of chips with problem stored in the overflow bin
        }
        float num = hFault->GetBinContent(iBin + 1);
        float ratio = (den > 0) ? (num / den) : 0.0;
        hFault->SetBinContent(iBin + 1, ratio);
      }
      result = checkQualityStatus(hFault, mVectorOfFaultBins);
    }

    if (mo->getName() == "mSummaryChipError") {
      resetVector(mVectorOfErrorBins);
      auto* hError = dynamic_cast<TH1F*>(mo->getObject());
      float den = hError->GetBinContent(0); // normalisation stored in the uderflow bin

      for (int iBin = 0; iBin < hError->GetNbinsX(); iBin++) {
        if (hError->GetBinContent(iBin + 1) != 0) {
          mVectorOfErrorBins.push_back(iBin + 1);
          hError->Fill(937); // number of chips with problem stored in the overflow bin
        }
        float num = hError->GetBinContent(iBin + 1);
        float ratio = (den > 0) ? (num / den) : 0.0;
        hError->SetBinContent(iBin + 1, ratio);
      }
      result = checkQualityStatus(hError, mVectorOfErrorBins);
    }

    if (mo->getName() == "mSummaryChipWarning") {
      resetVector(mVectorOfWarningBins);
      auto* hWarning = dynamic_cast<TH1F*>(mo->getObject());
      float den = hWarning->GetBinContent(0); // normalisation stored in the uderflow bin

      for (int iBin = 0; iBin < hWarning->GetNbinsX(); iBin++) {
        if (hWarning->GetBinContent(iBin + 1) != 0) {
          mVectorOfWarningBins.push_back(iBin + 1);
          hWarning->Fill(937); // number of chips with problem stored in the overflow bin
        }
        float num = hWarning->GetBinContent(iBin + 1);
        float ratio = (den > 0) ? (num / den) : 0.0;
        hWarning->SetBinContent(iBin + 1, ratio);
      }
      result = checkQualityStatus(hWarning, mVectorOfWarningBins);
    }

    if (mo->getName() == "mZoneSummaryChipWarning") {
      auto* hWarningSummary = dynamic_cast<TH2F*>(mo->getObject());

      float den = hWarningSummary->GetBinContent(0, 0); // normalisation stored in the underflow bin

      for (int iBinX = 0; iBinX < hWarningSummary->GetNbinsX(); iBinX++) {
        for (int iBinY = 0; iBinY < hWarningSummary->GetNbinsY(); iBinY++) {
          float num = hWarningSummary->GetBinContent(iBinX + 1, iBinY + 1);
          float ratio = (den > 0) ? (num / den) : 0.0;
          hWarningSummary->SetBinContent(iBinX + 1, iBinY + 1, ratio);
        }
      }
    }

    if (mo->getName() == "mZoneSummaryChipError") {
      auto* hErrorSummary = dynamic_cast<TH2F*>(mo->getObject());

      float den = hErrorSummary->GetBinContent(0, 0); // normalisation stored in the underflow bin

      for (int iBinX = 0; iBinX < hErrorSummary->GetNbinsX(); iBinX++) {
        for (int iBinY = 0; iBinY < hErrorSummary->GetNbinsY(); iBinY++) {
          float num = hErrorSummary->GetBinContent(iBinX + 1, iBinY + 1);
          float ratio = (den > 0) ? (num / den) : 0.0;
          hErrorSummary->SetBinContent(iBinX + 1, iBinY + 1, ratio);
        }
      }
    }

    if (mo->getName() == "mZoneSummaryChipFault") {
      auto* hFaultSummary = dynamic_cast<TH2F*>(mo->getObject());

      float den = hFaultSummary->GetBinContent(0, 0); // normalisation stored in the underflow bin

      for (int iBinX = 0; iBinX < hFaultSummary->GetNbinsX(); iBinX++) {
        for (int iBinY = 0; iBinY < hFaultSummary->GetNbinsY(); iBinY++) {
          float num = hFaultSummary->GetBinContent(iBinX + 1, iBinY + 1);
          float ratio = (den > 0) ? (num / den) : 0.0;
          hFaultSummary->SetBinContent(iBinX + 1, iBinY + 1, ratio);
        }
      }
    }

    if (mo->getName() == "mRDHSummary") {
      auto* hSLS = dynamic_cast<TH1F*>(mo->getObject());
      float den = hSLS->GetBinContent(0); // normalisation stored in the uderflow bin

      for (int iBin = 0; iBin < hSLS->GetNbinsX(); iBin++) {
        float num = hSLS->GetBinContent(iBin + 1);
        float ratio = (den > 0) ? (num / den) : 0.0;
        hSLS->SetBinContent(iBin + 1, ratio);
      }
    }

  } // end of loop over MO

  return result;
}

std::string QcMFTReadoutCheck::getAcceptedType() { return "TH1, TH2"; }

void QcMFTReadoutCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "mSummaryChipFault") {
    auto* hFault = dynamic_cast<TH1F*>(mo->getObject());
    writeMessages(hFault, mVectorOfFaultBins, checkResult);
  }

  if (mo->getName() == "mSummaryChipError") {
    auto* hError = dynamic_cast<TH1F*>(mo->getObject());
    writeMessages(hError, mVectorOfErrorBins, checkResult);
  }

  if (mo->getName() == "mSummaryChipWarning") {
    auto* hWarning = dynamic_cast<TH1F*>(mo->getObject());
    writeMessages(hWarning, mVectorOfWarningBins, checkResult);
  }
}

void QcMFTReadoutCheck::drawLatex(TH1F* histo, double xmin, double ymin, Color_t color, TString text,
                                  float tsize = 0.025, Font_t tfont = 42)
{

  TLatex* tl = new TLatex(xmin, ymin, Form("%s", text.Data()));
  tl->SetNDC();
  tl->SetTextFont(tfont);
  tl->SetTextSize(tsize);
  tl->SetTextColor(color);

  // add it to the histo
  histo->GetListOfFunctions()->Add(tl);
  tl->Draw();
}

void QcMFTReadoutCheck::resetVector(std::vector<int>& vector)
{
  vector.clear();
}

Quality QcMFTReadoutCheck::checkQualityStatus(TH1F* histo, std::vector<int>& vector)
{
  Quality result = Quality::Good;

  if (strcmp(histo->GetName(), "mSummaryChipFault") == 0) {
    if (vector.size() > mFaultThresholdBad)
      result = Quality::Bad;
    if (vector.size() > mFaultThresholdMedium && vector.size() <= mFaultThresholdBad)
      result = Quality::Medium;
    if (vector.size() <= mFaultThresholdMedium)
      result = Quality::Good;
  }
  if (strcmp(histo->GetName(), "mSummaryChipError") == 0) {
    if (vector.size() > mErrorThresholdBad)
      result = Quality::Bad;
    if (vector.size() > mErrorThresholdMedium && vector.size() <= mErrorThresholdBad)
      result = Quality::Medium;
    if (vector.size() <= mErrorThresholdMedium)
      result = Quality::Good;
  }
  if (strcmp(histo->GetName(), "mSummaryChipWarning") == 0) {
    if (vector.size() > mWarningThresholdBad)
      result = Quality::Bad;
    if (vector.size() > mWarningThresholdMedium && vector.size() <= mWarningThresholdBad)
      result = Quality::Medium;
    if (vector.size() <= mWarningThresholdMedium)
      result = Quality::Good;
  }

  return result;
}

void QcMFTReadoutCheck::writeMessages(TH1F* histo, std::vector<int>& vector, Quality checkResult)
{
  if (checkResult == Quality::Good) {
    histo->SetFillColor(kGreen + 2);
    histo->SetLineColor(kGreen + 2);
    if (strcmp(histo->GetName(), "mSummaryChipFault") == 0) {
      drawLatex(histo, 0.14, 0.87, kGreen + 2, "Quality is good.", 0.04);
      drawLatex(histo, 0.14, 0.87 - 0.025, kGreen + 2, Form("%lu chips in Fault as expected.", vector.size()));
    }
    if (strcmp(histo->GetName(), "mSummaryChipError") == 0) {
      drawLatex(histo, 0.14, 0.87, kGreen + 2, "Quality is good.", 0.04);
      drawLatex(histo, 0.14, 0.87 - 0.025, kGreen + 2, Form("%lu chips in Error as expected.", vector.size()));
    };
    if (strcmp(histo->GetName(), "mSummaryChipWarning") == 0) {
      drawLatex(histo, 0.14, 0.87, kGreen + 2, "Quality is good.", 0.04);
      drawLatex(histo, 0.14, 0.87 - 0.025, kGreen + 2, Form("%lu chips in Warning as expected.", vector.size()));
    }
  } else if (checkResult == Quality::Medium) {
    histo->SetFillColor(kOrange + 7);
    histo->SetLineColor(kOrange + 7);
    if (strcmp(histo->GetName(), "mSummaryChipFault") == 0) {
      drawLatex(histo, 0.14, 0.87, kOrange + 7, "Quality is Medium.", 0.04);
      drawLatex(histo, 0.14, 0.87 - 0.025, kOrange + 7, Form("%lu chips in Fault. Write a logbook entry tagging MFT.", vector.size()));
    }
    if (strcmp(histo->GetName(), "mSummaryChipError") == 0) {
      drawLatex(histo, 0.14, 0.87, kOrange + 7, "Quality is Medium.", 0.04);
      drawLatex(histo, 0.14, 0.87 - 0.025, kOrange + 7, Form("%lu chips in Error. Write a logbook entry tagging MFT.", vector.size()));
    }
    if (strcmp(histo->GetName(), "mSummaryChipWarning") == 0) {
      drawLatex(histo, 0.14, 0.87, kOrange + 7, "Quality is Medium.", 0.04);
      drawLatex(histo, 0.14, 0.87 - 0.025, kOrange + 7, Form("%lu chips in Warning. Write a logbook entry tagging MFT.", vector.size()));
    }
  } else if (checkResult == Quality::Bad) {
    histo->SetFillColor(kRed + 1);
    histo->SetLineColor(kRed + 1);
    if (strcmp(histo->GetName(), "mSummaryChipFault") == 0) {
      drawLatex(histo, 0.14, 0.87, kRed + 1, "Quality is Bad.", 0.04);
      drawLatex(histo, 0.14, 0.87 - 0.025, kRed + 1, Form("%lu chips in Fault. Inform the MFT oncall immediately!.", vector.size()));
    }
    if (strcmp(histo->GetName(), "mSummaryChipError") == 0) {
      drawLatex(histo, 0.14, 0.87, kRed + 1, "Quality is Bad.", 0.04);
      drawLatex(histo, 0.14, 0.87 - 0.025, kRed + 1, Form("%lu chips in Error. Inform the MFT oncall immediately!.", vector.size()));
    }
    if (strcmp(histo->GetName(), "mSummaryChipWarning") == 0) {
      drawLatex(histo, 0.14, 0.87, kRed + 1, "Quality is Bad.", 0.04);
      drawLatex(histo, 0.14, 0.87 - 0.025, kRed + 1, Form("%lu chips in Warning. Inform the MFT oncall immediately!.", vector.size()));
    }
  }
  histo->SetMaximum(histo->GetMaximum() * 1.7);
  int midBinIteration = (vector.size() < 10) ? vector.size() : 10;
  for (int iBin = 0; iBin < midBinIteration; iBin++) {
    drawLatex(histo, 0.15, 0.85 - 0.025 - iBin * 0.025, kBlack, Form("%s", histo->GetXaxis()->GetBinLabel(vector[iBin])));
  }
  int maxBinIteration = (vector.size() < 20) ? vector.size() : 20;
  for (int iBin = midBinIteration; iBin < maxBinIteration; iBin++) {
    drawLatex(histo, 0.55, 0.85 - 0.025 - (iBin - midBinIteration) * 0.025, kBlack, Form("%s", histo->GetXaxis()->GetBinLabel(vector[iBin])));
  }
}

} // namespace o2::quality_control_modules::mft
