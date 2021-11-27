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

// Quality Control
#include "MFT/QcMFTReadoutCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

using namespace std;

namespace o2::quality_control_modules::mft
{

void QcMFTReadoutCheck::configure(std::string) {}

Quality QcMFTReadoutCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;
  
    if (mo->getName() == "mSummaryChipFault") {
      resetVector(mVectorOfFaultBins);
      auto* hFault = dynamic_cast<TH1F*>(mo->getObject());

      for(int iBin=0; iBin<hFault->GetNbinsX(); iBin++) {
        if (hFault->GetBinContent(iBin+1) != 0) {
          mVectorOfFaultBins.push_back(iBin+1);
        }
      }
      result = checkQualityStatus(hFault, mVectorOfFaultBins);
    }

    if (mo->getName() == "mSummaryChipError") {
      resetVector(mVectorOfErrorBins);
      auto* hError = dynamic_cast<TH1F*>(mo->getObject());

      for(int iBin=0; iBin<hError->GetNbinsX(); iBin++) {
        if (hError->GetBinContent(iBin+1) != 0) {
          mVectorOfErrorBins.push_back(iBin+1);
        }
      }
      result = checkQualityStatus(hError, mVectorOfErrorBins);
    }

    if (mo->getName() == "mSummaryChipWarning") {
      resetVector(mVectorOfWarningBins);
      auto* hWarning = dynamic_cast<TH1F*>(mo->getObject());

      for(int iBin=0; iBin<hWarning->GetNbinsX(); iBin++) {
        if (hWarning->GetBinContent(iBin+1) != 0) {
          mVectorOfWarningBins.push_back(iBin+1);
        }
      }
      result = checkQualityStatus(hWarning, mVectorOfWarningBins);
    }

  }   // end of loop over MO

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

TLatex* QcMFTReadoutCheck::drawLatex(double xmin, double ymin, Color_t color, TString text)
{

  TLatex *tl = new TLatex(xmin, ymin, Form("%s", text.Data()));
  tl->SetNDC();
  tl->SetTextFont(42);
  tl->SetTextSize(0.025);
  tl->SetTextColor(color);
  
  return tl;

}

void QcMFTReadoutCheck::resetVector(std::vector<int> vector)
{
  vector.clear();
}

Quality QcMFTReadoutCheck::checkQualityStatus(TH1F* histo, std::vector<int> vector) 
{
  Quality result = Quality::Good;

  if (vector.size() > 20)
    result = Quality::Bad;
  if (vector.size() > 0 && vector.size() <= 20) 
    result = Quality::Medium;
  if (vector.size() == 0) 
    result = Quality::Good;

  return result;
}

void QcMFTReadoutCheck::writeMessages(TH1F* histo, std::vector<int> vector, Quality checkResult)
{
  if (checkResult == Quality::Good) { 
    TLatex* tlGood;
    if (strcmp(histo->GetName(), "mSummaryChipFault") == 0)   tlGood = drawLatex(0.15, 0.85, kGreen+2, "No lanes in Fault.");
    if (strcmp(histo->GetName(), "mSummaryChipError") == 0)   tlGood = drawLatex(0.15, 0.85, kGreen+2, "No lanes in Error.");
    if (strcmp(histo->GetName(), "mSummaryChipWarning") == 0) tlGood = drawLatex(0.15, 0.85, kGreen+2, "No lanes in Warning.");
    histo->GetListOfFunctions()->Add(tlGood);
    tlGood->Draw();
  } else if (checkResult == Quality::Medium) {
    histo->SetFillColor(kOrange+7);
    histo->SetLineColor(kOrange+7);
    histo->SetMaximum(histo->GetMaximum()*(3./2.));

    int midBinIteration = (vector.size() < 10) ? vector.size() : 10;
    for (int iBin=0; iBin<midBinIteration; iBin++) {
      TLatex *tlMedium = drawLatex(0.15, 0.875-iBin*0.025, kBlack, Form("%s", histo->GetXaxis()->GetBinLabel(vector[iBin])));
      histo->GetListOfFunctions()->Add(tlMedium);
      tlMedium->Draw();
    }
    int maxBinIteration = (vector.size() < 20) ? vector.size() : 20;
    for (int iBin=midBinIteration; iBin<maxBinIteration; iBin++) {
      TLatex *tlMedium = drawLatex(0.55, 0.875-(iBin-midBinIteration)*0.025, kBlack, Form("%s", histo->GetXaxis()->GetBinLabel(vector[iBin])));
      histo->GetListOfFunctions()->Add(tlMedium);
      tlMedium->Draw();
    }
  } else if (checkResult == Quality::Bad) {
    histo->SetFillColor(kRed+1);
    histo->SetLineColor(kRed+1);
    histo->SetMaximum(histo->GetMaximum()*(6./5.));
    TLatex* tlBad;
    if (strcmp(histo->GetName(), "mSummaryChipFault") == 0)    tlBad = drawLatex(0.15, 0.85, kRed+1, Form("%i lanes in Fault.", vector.size()));
    if (strcmp(histo->GetName(), "mSummaryChipError") == 0)    tlBad = drawLatex(0.15, 0.85, kRed+1, Form("%i lanes in Error.", vector.size()));
    if (strcmp(histo->GetName(), "mSummaryChipWarning") == 0)  tlBad = drawLatex(0.15, 0.85, kRed+1, Form("%i lanes in Warning.", vector.size()));
    histo->GetListOfFunctions()->Add(tlBad);
    tlBad->Draw();
  }
}

} // namespace o2::quality_control_modules::mft

