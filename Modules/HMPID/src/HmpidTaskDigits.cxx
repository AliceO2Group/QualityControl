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
/// \file       HmpidTaskDigits.cxx
/// \author     Antonio Paz, Giacomo Volpe
/// \brief      Class to map data from HMPID detectors
/// \version    0.3.1
/// \date       26/10/2021
///

//! ap  Changes:
//     -  Histograms    hHMPIDdigitmapAvg[numCham]   back to   TH2F
//     -  Histograms    hHMPIDchargeDist[numCham]  back to  TH1F

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TProfile.h>

#include <TMath.h>
#include <Framework/InputRecord.h>

#include "QualityControl/QcInfoLogger.h"
#include "HMPID/HmpidTaskDigits.h"
#include "DataFormatsHMP/Digit.h"
#include "DataFormatsHMP/Trigger.h"

namespace o2::quality_control_modules::hmpid
{

HmpidTaskDigits::~HmpidTaskDigits()
{
  delete hOccupancyAvg;

  for (Int_t i = 0; i < numCham; ++i) {
    delete hHMPIDchargeDist[i];
    delete hHMPIDdigitmapAvg[i];
  }
}

void HmpidTaskDigits::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize HmpidTaskDigits" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  // if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
  //  ILOG(Info, Support) << "Custom parameter - myOwnKey: " << param->second << ENDM;
  // }

  hOccupancyAvg = new TProfile("hOccupancyAvg", "Occupancy per DDL;;Occupancy (%)", 14, 0.5, 14.5);
  hOccupancyAvg->Sumw2();
  hOccupancyAvg->SetOption("P");
  hOccupancyAvg->SetMinimum(0);
  hOccupancyAvg->SetMarkerStyle(20);
  hOccupancyAvg->SetMarkerColor(kBlack);
  hOccupancyAvg->SetLineColor(kBlack);
  for (Int_t iddl = 0; iddl < 14; iddl++) {
    hOccupancyAvg->GetXaxis()->SetBinLabel(iddl + 1, Form("%d", iddl + 1));
  }
  hOccupancyAvg->GetXaxis()->SetLabelSize(0.02);
  hOccupancyAvg->SetStats(0);

  getObjectsManager()->startPublishing(hOccupancyAvg);
  getObjectsManager()->addMetadata(hOccupancyAvg->GetName(), "custom", "34");

  // histos for digit charge distributions
  for (Int_t i = 0; i < numCham; ++i) {

    hHMPIDchargeDist[i] = new TH1F(Form("hHMPIDchargeDist%i", i + 1), Form("Distribution of charges in chamber %i", i + 1), 2000, 0, 2000);
    hHMPIDchargeDist[i]->SetXTitle("Charge (ADC)");
    hHMPIDchargeDist[i]->SetYTitle("Entries/1 ADC");

    hHMPIDdigitmapAvg[i] = new TH2F(Form("hHMPIDdigitmapAvg%i", i + 1), Form("Coordinates of hits in chamber %i", i + 1), 160, 0, 160, 144, 0, 144);
    hHMPIDdigitmapAvg[i]->SetXTitle("padX");
    hHMPIDdigitmapAvg[i]->SetYTitle("padY");

    getObjectsManager()->startPublishing(hHMPIDchargeDist[i]);

    getObjectsManager()->startPublishing(hHMPIDdigitmapAvg[i]);
  }
}

void HmpidTaskDigits::startOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;
  hOccupancyAvg->Reset();
  for (Int_t i = 0; i < numCham; ++i) {
    hHMPIDchargeDist[i]->Reset();
    hHMPIDdigitmapAvg[i]->Reset();
  }
}

void HmpidTaskDigits::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void HmpidTaskDigits::monitorData(o2::framework::ProcessingContext& ctx)
{
  // Get HMPID triggers
  const auto triggers = ctx.inputs().get<std::vector<o2::hmpid::Trigger>>("intrecord");

  // Get HMPID digits
  const auto digits = ctx.inputs().get<std::vector<o2::hmpid::Digit>>("digits");

  int nEvents = triggers.size();

  for (int i = 0; i < nEvents; i++) { // events loop

    double equipEntries[numEquip] = { 0x0 };

    for (int j = triggers[i].getFirstEntry(); j <= triggers[i].getLastEntry(); j++) { // digits loop on the same event

      int padChX = 0, padChY = 0, module = 0;

      o2::hmpid::Digit::pad2Absolute(digits[j].getPadID(), &module, &padChX, &padChY);

      Double_t charge = digits[j].getCharge();

      if (module <= 6 && module >= 0) {
        hHMPIDchargeDist[module]->Fill(charge);
        hHMPIDdigitmapAvg[module]->Fill(padChX, padChY);
      }

      Int_t eqID = 0, col = 0, dlog = 0, gass = 0;

      o2::hmpid::Digit::absolute2Equipment(module, padChX, padChY, &eqID, &col, &dlog, &gass);

      if (eqID < (numEquip + 1)) {
        //! ap   A trap just in case eqID return wrong value
        equipEntries[eqID]++;
      }

    } // digits loop on the same event

    for (Int_t eq = 0; eq < numEquip; eq++) {

      hOccupancyAvg->SetBinContent(eq + 1, 100. * equipEntries[eq] / (nEvents * factor));
    }
  } // events loop
}

void HmpidTaskDigits::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void HmpidTaskDigits::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void HmpidTaskDigits::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Devel) << "Resetting the histogram" << ENDM;
  hOccupancyAvg->Reset();
  for (Int_t i = 0; i < numCham; ++i) {
    hHMPIDchargeDist[i]->Reset();
    hHMPIDdigitmapAvg[i]->Reset();
  }
}

} // namespace o2::quality_control_modules::hmpid
