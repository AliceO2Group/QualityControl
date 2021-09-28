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
/// \file   BasicPPTask.cxx
/// \author Sebastian Bysiak sbysiak@cern.ch
///

#include "FT0/BasicPPTask.h"
#include "QualityControl/QcInfoLogger.h"

#include <TH1F.h>
#include <TGraph.h>
#include <TCanvas.h>
#include <TPad.h>
#include <TLegend.h>

using namespace o2::quality_control::postprocessing;

namespace o2::quality_control_modules::ft0
{

BasicPPTask::~BasicPPTask()
{
  delete mRateOrA;
  delete mRateOrC;
  delete mRateVertex;
  delete mRateCentral;
  delete mRateSemiCentral;
  delete mRatesCanv;
}

void BasicPPTask::initialize(Trigger, framework::ServiceRegistry& services)
{
  mDatabase = &services.get<o2::quality_control::repository::DatabaseInterface>();

  mRateOrA = new TGraph();
  mRateOrC = new TGraph();
  mRateVertex = new TGraph();
  mRateCentral = new TGraph();
  mRateSemiCentral = new TGraph();
  mRatesCanv = new TCanvas("cRates", "trigger rates");

  mRateOrA->SetNameTitle("rateOrA", "trg rate: OrA;cycle;rate [kHz]");
  mRateOrC->SetNameTitle("rateOrC", "trg rate: OrC;cycle;rate [kHz]");
  mRateVertex->SetNameTitle("rateVertex", "trg rate: Vertex;cycle;rate [kHz]");
  mRateCentral->SetNameTitle("rateCentral", "trg rate: Central;cycle;rate [kHz]");
  mRateSemiCentral->SetNameTitle("rateSemiCentral", "trg rate: SemiCentral;cycle;rate [kHz]");

  mRateOrA->SetMarkerStyle(24);
  mRateOrC->SetMarkerStyle(25);
  mRateVertex->SetMarkerStyle(26);
  mRateCentral->SetMarkerStyle(27);
  mRateSemiCentral->SetMarkerStyle(28);
  mRateOrA->SetMarkerColor(kOrange);
  mRateOrC->SetMarkerColor(kMagenta);
  mRateVertex->SetMarkerColor(kBlack);
  mRateCentral->SetMarkerColor(kBlue);
  mRateSemiCentral->SetMarkerColor(kOrange);
  mRateOrA->SetLineColor(kOrange);
  mRateOrC->SetLineColor(kMagenta);
  mRateVertex->SetLineColor(kBlack);
  mRateCentral->SetLineColor(kBlue);
  mRateSemiCentral->SetLineColor(kOrange);

  mRateOrA->GetYaxis()->SetTitleOffset(1.4);

  getObjectsManager()->startPublishing(mRateOrA);
  getObjectsManager()->startPublishing(mRateOrC);
  getObjectsManager()->startPublishing(mRateVertex);
  getObjectsManager()->startPublishing(mRateCentral);
  getObjectsManager()->startPublishing(mRateSemiCentral);
  getObjectsManager()->startPublishing(mRatesCanv);
}

void BasicPPTask::update(Trigger, framework::ServiceRegistry&)
{
  auto mo = mDatabase->retrieveMO("qc/FT0/MO/DigitQcTask/", "Triggers");
  auto hTriggers = (TH1F*)mo->getObject();
  if (!hTriggers) {
    ILOG(Error) << "\nMO \"Triggers\" NOT retrieved!!!\n"
                << ENDM;
  }

  auto mo2 = mDatabase->retrieveMO("qc/FT0/MO/DigitQcTask/", "CycleDuration");
  auto hCycleDuration = (TH1D*)mo2->getObject();
  if (!hCycleDuration) {
    ILOG(Error) << "\nMO \"CycleDuration\" NOT retrieved!!!\n"
                << ENDM;
  }
  double cycleDurationMS = hCycleDuration->GetBinContent(1) / 1e6; // ns -> ms

  int n = mRateOrA->GetN();

  double eps = 1e-8;
  if (cycleDurationMS < eps) {
    ILOG(Warning) << "cycle duration = " << cycleDurationMS << " ms, almost zero - cannot compute trigger rates!" << ENDM;
  } else {
    mRateOrA->SetPoint(n, n, hTriggers->GetBinContent(hTriggers->GetXaxis()->FindBin("OrA")) / cycleDurationMS);
    mRateOrC->SetPoint(n, n, hTriggers->GetBinContent(hTriggers->GetXaxis()->FindBin("OrC")) / cycleDurationMS);
    mRateVertex->SetPoint(n, n, hTriggers->GetBinContent(hTriggers->GetXaxis()->FindBin("Vertex")) / cycleDurationMS);
    mRateCentral->SetPoint(n, n, hTriggers->GetBinContent(hTriggers->GetXaxis()->FindBin("Central")) / cycleDurationMS);
    mRateSemiCentral->SetPoint(n, n, hTriggers->GetBinContent(hTriggers->GetXaxis()->FindBin("SemiCentral")) / cycleDurationMS);
  }

  mRatesCanv->cd();
  float vmin = std::min({ mRateOrA->GetYaxis()->GetXmin(), mRateOrC->GetYaxis()->GetXmin(), mRateVertex->GetYaxis()->GetXmin(), mRateCentral->GetYaxis()->GetXmin(), mRateSemiCentral->GetYaxis()->GetXmin() });
  float vmax = std::max({ mRateOrA->GetYaxis()->GetXmax(), mRateOrC->GetYaxis()->GetXmax(), mRateVertex->GetYaxis()->GetXmax(), mRateCentral->GetYaxis()->GetXmax(), mRateSemiCentral->GetYaxis()->GetXmax() });

  auto hAxis = mRateOrA->GetHistogram();
  hAxis->GetYaxis()->SetTitleOffset(1.4);
  hAxis->SetMinimum(vmin);
  hAxis->SetMaximum(vmax * 1.1);
  hAxis->SetTitle("FT0 trigger rates");
  hAxis->SetLineWidth(0);
  hAxis->Draw("AXIS");

  mRateOrA->Draw("PL,SAME");
  mRateOrC->Draw("PL,SAME");
  mRateVertex->Draw("PL,SAME");
  mRateCentral->Draw("PL,SAME");
  mRateSemiCentral->Draw("PL,SAME");
  TLegend* leg = gPad->BuildLegend();
  leg->SetFillStyle(1);
}

void BasicPPTask::finalize(Trigger, framework::ServiceRegistry&)
{
}

} // namespace o2::quality_control_modules::ft0
