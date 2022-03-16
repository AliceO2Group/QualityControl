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

#include "FV0/BasicPPTask.h"
#include "QualityControl/QcInfoLogger.h"
#include "CommonConstants/LHCConstants.h"

#include <TH1F.h>
#include <TH2F.h>
#include <TGraph.h>
#include <TCanvas.h>
#include <TPad.h>
#include <TLegend.h>
#include <TProfile.h>

using namespace o2::quality_control::postprocessing;

namespace o2::quality_control_modules::fv0
{

BasicPPTask::~BasicPPTask()
{
  delete mAmpl;
  delete mTime;
}

void BasicPPTask::configure(std::string, const boost::property_tree::ptree& config)
{
  const char* configPath = Form("qc.postprocessing.%s", getName().c_str());
  ILOG(Info, Support) << "configPath = " << configPath << ENDM;
  auto node = config.get_child_optional(Form("%s.custom.numOrbitsInTF", configPath));
  if (node) {
    mNumOrbitsInTF = std::stoi(node.get_ptr()->get_child("").get_value<std::string>());
    ILOG(Info, Support) << "configure() : using numOrbitsInTF = " << mNumOrbitsInTF << ENDM;
  } else {
    mNumOrbitsInTF = 256;
    ILOG(Info, Support) << "configure() : using default numOrbitsInTF = " << mNumOrbitsInTF << ENDM;
  }

  node = config.get_child_optional(Form("%s.custom.cycleDurationMoName", configPath));
  if (node) {
    mCycleDurationMoName = node.get_ptr()->get_child("").get_value<std::string>();
    ILOG(Info, Support) << "configure() : using cycleDurationMoName = \"" << mCycleDurationMoName << "\"" << ENDM;
  } else {
    mCycleDurationMoName = "CycleDurationNTF";
    ILOG(Info, Support) << "configure() : using default cycleDurationMoName = \"" << mCycleDurationMoName << "\"" << ENDM;
  }

  node = config.get_child_optional(Form("%s.custom.pathDigitQcTask", configPath));
  if (node) {
    mPathDigitQcTask = node.get_ptr()->get_child("").get_value<std::string>();
    ILOG(Info, Support) << "configure() : using pathDigitQcTask = \"" << mPathDigitQcTask << "\"" << ENDM;
  } else {
    mPathDigitQcTask = "qc/FV0/MO/DigitQcTask/";
    ILOG(Info, Support) << "configure() : using default pathDigitQcTask = \"" << mPathDigitQcTask << "\"" << ENDM;
  }
}

void BasicPPTask::initialize(Trigger, framework::ServiceRegistry& services)
{
  mDatabase = &services.get<o2::quality_control::repository::DatabaseInterface>();

  mRateMinBias = std::make_unique<TGraph>(0);
  mRateOuterRing = std::make_unique<TGraph>(0);
  mRateNChannels = std::make_unique<TGraph>(0);
  mRateCharge = std::make_unique<TGraph>(0);
  mRateInnerRing = std::make_unique<TGraph>(0);
  mRatesCanv = std::make_unique<TCanvas>("cRates", "trigger rates");
  mAmpl = new TProfile("MeanAmplPerChannel", "mean ampl per channel;Channel;Ampl #mu #pm #sigma", sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mTime = new TProfile("MeanTimePerChannel", "mean time per channel;Channel;Time #mu #pm #sigma", sNCHANNELS_PM, 0, sNCHANNELS_PM);

  mRateMinBias->SetNameTitle("rateMinBias", "trg rate: MinBias;cycle;rate [kHz]");
  mRateOuterRing->SetNameTitle("rateOuterRing", "trg rate: OuterRing;cycle;rate [kHz]");
  mRateNChannels->SetNameTitle("rateNChannels", "trg rate: NChannels;cycle;rate [kHz]");
  mRateCharge->SetNameTitle("rateCharge", "trg rate: Charge;cycle;rate [kHz]");
  mRateInnerRing->SetNameTitle("rateInnerRing", "trg rate: InnerRing;cycle;rate [kHz]");

  mRateMinBias->SetMarkerStyle(24);
  mRateOuterRing->SetMarkerStyle(25);
  mRateNChannels->SetMarkerStyle(26);
  mRateCharge->SetMarkerStyle(27);
  mRateInnerRing->SetMarkerStyle(28);
  mRateMinBias->SetMarkerColor(kOrange);
  mRateOuterRing->SetMarkerColor(kMagenta);
  mRateNChannels->SetMarkerColor(kBlack);
  mRateCharge->SetMarkerColor(kBlue);
  mRateInnerRing->SetMarkerColor(kOrange);
  mRateMinBias->SetLineColor(kOrange);
  mRateOuterRing->SetLineColor(kMagenta);
  mRateNChannels->SetLineColor(kBlack);
  mRateCharge->SetLineColor(kBlue);
  mRateInnerRing->SetLineColor(kOrange);

  getObjectsManager()->startPublishing(mRateMinBias.get());
  getObjectsManager()->startPublishing(mRateOuterRing.get());
  getObjectsManager()->startPublishing(mRateNChannels.get());
  getObjectsManager()->startPublishing(mRateCharge.get());
  getObjectsManager()->startPublishing(mRateInnerRing.get());
  getObjectsManager()->startPublishing(mRatesCanv.get());
  getObjectsManager()->startPublishing(mAmpl);
  getObjectsManager()->startPublishing(mTime);
}

void BasicPPTask::update(Trigger t, framework::ServiceRegistry&)
{
  auto mo = mDatabase->retrieveMO(mPathDigitQcTask, "Triggers", t.timestamp, t.activity);
  auto hTriggers = mo ? (TH1F*)mo->getObject() : nullptr;
  if (!hTriggers) {
    ILOG(Error, Support) << "MO \"Triggers\" NOT retrieved!!!" << ENDM;
  }

  auto mo2 = mDatabase->retrieveMO(mPathDigitQcTask, mCycleDurationMoName, t.timestamp, t.activity);
  auto hCycleDuration = mo2 ? (TH1D*)mo2->getObject() : nullptr;
  if (!hCycleDuration) {
    ILOG(Error, Support) << "MO \"" << mCycleDurationMoName << "\" NOT retrieved!!!" << ENDM;
  }

  if (hTriggers && hCycleDuration) {
    double cycleDurationMS = 0;
    if (mCycleDurationMoName == "CycleDuration" || mCycleDurationMoName == "CycleDurationRange")
      // assume MO stores cycle duration in ns
      cycleDurationMS = hCycleDuration->GetBinContent(1) / 1e6; // ns -> ms
    else if (mCycleDurationMoName == "CycleDurationNTF")
      // assume MO stores cycle duration in number of TF
      cycleDurationMS = hCycleDuration->GetBinContent(1) * mNumOrbitsInTF * o2::constants::lhc::LHCOrbitNS / 1e6; // ns ->ms

    int n = mRateMinBias->GetN();

    double eps = 1e-8;
    if (cycleDurationMS < eps) {
      ILOG(Warning, Support) << "cycle duration = " << cycleDurationMS << " ms, almost zero - cannot compute trigger rates!" << ENDM;
    } else {
      mRateMinBias->SetPoint(n, n, hTriggers->GetBinContent(hTriggers->GetXaxis()->FindBin("MinBias")) / cycleDurationMS);
      mRateOuterRing->SetPoint(n, n, hTriggers->GetBinContent(hTriggers->GetXaxis()->FindBin("OuterRing")) / cycleDurationMS);
      mRateNChannels->SetPoint(n, n, hTriggers->GetBinContent(hTriggers->GetXaxis()->FindBin("NChannels")) / cycleDurationMS);
      mRateCharge->SetPoint(n, n, hTriggers->GetBinContent(hTriggers->GetXaxis()->FindBin("Charge")) / cycleDurationMS);
      mRateInnerRing->SetPoint(n, n, hTriggers->GetBinContent(hTriggers->GetXaxis()->FindBin("InnerRing")) / cycleDurationMS);
    }

    mRatesCanv->cd();
    float vmin = std::min({ mRateMinBias->GetYaxis()->GetXmin(), mRateOuterRing->GetYaxis()->GetXmin(), mRateNChannels->GetYaxis()->GetXmin(), mRateCharge->GetYaxis()->GetXmin(), mRateInnerRing->GetYaxis()->GetXmin() });
    float vmax = std::max({ mRateMinBias->GetYaxis()->GetXmax(), mRateOuterRing->GetYaxis()->GetXmax(), mRateNChannels->GetYaxis()->GetXmax(), mRateCharge->GetYaxis()->GetXmax(), mRateInnerRing->GetYaxis()->GetXmax() });

    auto hAxis = mRateMinBias->GetHistogram();
    hAxis->GetYaxis()->SetTitleOffset(1.4);
    hAxis->SetMinimum(vmin);
    hAxis->SetMaximum(vmax * 1.1);
    hAxis->SetTitle("FV0 trigger rates");
    hAxis->SetLineWidth(0);
    hAxis->Draw("AXIS");

    mRateMinBias->Draw("PL,SAME");
    mRateOuterRing->Draw("PL,SAME");
    mRateNChannels->Draw("PL,SAME");
    mRateCharge->Draw("PL,SAME");
    mRateInnerRing->Draw("PL,SAME");
    TLegend* leg = gPad->BuildLegend();
    leg->SetFillStyle(1);
  }

  auto mo3 = mDatabase->retrieveMO(mPathDigitQcTask, "AmpPerChannel", t.timestamp, t.activity);
  auto hAmpPerChannel = mo3 ? (TH2D*)mo3->getObject() : nullptr;
  if (!hAmpPerChannel) {
    ILOG(Error, Support) << "MO \"AmpPerChannel\" NOT retrieved!!!"
                         << ENDM;
  }
  auto mo4 = mDatabase->retrieveMO(mPathDigitQcTask, "TimePerChannel", t.timestamp, t.activity);
  auto hTimePerChannel = mo4 ? (TH2D*)mo4->getObject() : nullptr;
  if (!hTimePerChannel) {
    ILOG(Error, Support) << "MO \"TimePerChannel\" NOT retrieved!!!"
                         << ENDM;
  }
  if (hAmpPerChannel && hTimePerChannel) {
    mAmpl = hAmpPerChannel->ProfileX("MeanAmplPerChannel");
    mTime = hTimePerChannel->ProfileX("MeanTimePerChannel");
    mAmpl->SetErrorOption("s");
    mTime->SetErrorOption("s");
    // for some reason the styling is not preserved after assigning result of ProfileX/Y() to already existing object
    mAmpl->SetMarkerStyle(8);
    mTime->SetMarkerStyle(8);
    mAmpl->SetLineColor(kBlack);
    mTime->SetLineColor(kBlack);
    mAmpl->SetDrawOption("P");
    mTime->SetDrawOption("P");
    mAmpl->GetXaxis()->SetTitleOffset(1);
    mTime->GetXaxis()->SetTitleOffset(1);
    mAmpl->GetYaxis()->SetTitleOffset(1);
    mTime->GetYaxis()->SetTitleOffset(1);
  }
}

void BasicPPTask::finalize(Trigger t, framework::ServiceRegistry&)
{
}

} // namespace o2::quality_control_modules::fv0
