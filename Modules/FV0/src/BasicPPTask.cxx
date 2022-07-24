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
    mPathDigitQcTask = "FV0/MO/DigitQcTask/";
    ILOG(Info, Support) << "configure() : using default pathDigitQcTask = \"" << mPathDigitQcTask << "\"" << ENDM;
  }
}

void BasicPPTask::initialize(Trigger, framework::ServiceRegistry& services)
{
  mDatabase = &services.get<o2::quality_control::repository::DatabaseInterface>();

  mRateOrA = std::make_unique<TGraph>(0);
  mRateOrAout = std::make_unique<TGraph>(0);
  mRateOrAin = std::make_unique<TGraph>(0);
  mRateTrgCharge = std::make_unique<TGraph>(0);
  mRateTrgNchan = std::make_unique<TGraph>(0);
  mRatesCanv = std::make_unique<TCanvas>("cRates", "trigger rates");
  mAmpl = new TProfile("MeanAmplPerChannel", "mean ampl per channel;Channel;Ampl #mu #pm #sigma", sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mTime = new TProfile("MeanTimePerChannel", "mean time per channel;Channel;Time #mu #pm #sigma", sNCHANNELS_PM, 0, sNCHANNELS_PM);

  mRateOrA->SetNameTitle("rateOrA", "trg rate: OrA;cycle;rate [kHz]");
  mRateOrAout->SetNameTitle("rateOrAout", "trg rate: OrAout;cycle;rate [kHz]");
  mRateOrAin->SetNameTitle("rateOrAin", "trg rate: OrAin;cycle;rate [kHz]");
  mRateTrgCharge->SetNameTitle("rateTrgCharge", "trg rate: TrgCharge;cycle;rate [kHz]");
  mRateTrgNchan->SetNameTitle("rateTrgNchan", "trg rate: TrgNchan;cycle;rate [kHz]");

  mRateOrA->SetMarkerStyle(24);
  mRateOrAout->SetMarkerStyle(25);
  mRateOrAin->SetMarkerStyle(26);
  mRateTrgCharge->SetMarkerStyle(27);
  mRateTrgNchan->SetMarkerStyle(28);
  mRateOrA->SetMarkerColor(kOrange);
  mRateOrAout->SetMarkerColor(kMagenta);
  mRateOrAin->SetMarkerColor(kBlack);
  mRateTrgCharge->SetMarkerColor(kBlue);
  mRateTrgNchan->SetMarkerColor(kOrange);
  mRateOrA->SetLineColor(kOrange);
  mRateOrAout->SetLineColor(kMagenta);
  mRateOrAin->SetLineColor(kBlack);
  mRateTrgCharge->SetLineColor(kBlue);
  mRateTrgNchan->SetLineColor(kOrange);

  mMapChTrgNames.insert({ o2::fv0::ChannelData::kNumberADC, "NumberADC" });
  mMapChTrgNames.insert({ o2::fv0::ChannelData::kIsDoubleEvent, "IsDoubleEvent" });
  mMapChTrgNames.insert({ o2::fv0::ChannelData::kIsTimeInfoNOTvalid, "IsTimeInfoNOTvalid" });
  mMapChTrgNames.insert({ o2::fv0::ChannelData::kIsCFDinADCgate, "IsCFDinADCgate" });
  mMapChTrgNames.insert({ o2::fv0::ChannelData::kIsTimeInfoLate, "IsTimeInfoLate" });
  mMapChTrgNames.insert({ o2::fv0::ChannelData::kIsAmpHigh, "IsAmpHigh" });
  mMapChTrgNames.insert({ o2::fv0::ChannelData::kIsEventInTVDC, "IsEventInTVDC" });
  mMapChTrgNames.insert({ o2::fv0::ChannelData::kIsTimeInfoLost, "IsTimeInfoLost" });

  mHistChDataNegBits = std::make_unique<TH2F>("ChannelDataNegBits", "ChannelData negative bits per ChannelID;Channel;Negative bit", sNCHANNELS_PM, 0, sNCHANNELS_PM, mMapChTrgNames.size(), 0, mMapChTrgNames.size());
  for (const auto& entry : mMapChTrgNames) {
    std::string stBitName = "! " + entry.second;
    mHistChDataNegBits->GetYaxis()->SetBinLabel(entry.first + 1, stBitName.c_str());
  }
  getObjectsManager()->startPublishing(mHistChDataNegBits.get());
  getObjectsManager()->setDefaultDrawOptions(mHistChDataNegBits.get(), "COLZ");

  mMapDigitTrgNames.insert({ o2::fit::Triggers::bitA, "OrA" });
  mMapDigitTrgNames.insert({ o2::fit::Triggers::bitAOut, "OrAout" });
  mMapDigitTrgNames.insert({ o2::fit::Triggers::bitAIn, "OrAin" });
  mMapDigitTrgNames.insert({ o2::fit::Triggers::bitTrgCharge, "TrgCharge" });
  mMapDigitTrgNames.insert({ o2::fit::Triggers::bitTrgNchan, "TrgNchan" });
  mMapDigitTrgNames.insert({ o2::fit::Triggers::bitLaser, "Laser" });
  mMapDigitTrgNames.insert({ o2::fit::Triggers::bitOutputsAreBlocked, "OutputsAreBlocked" });
  mMapDigitTrgNames.insert({ o2::fit::Triggers::bitDataIsValid, "DataIsValid" });
  mHistTriggers = std::make_unique<TH1F>("Triggers", "Triggers from TCM", mMapDigitTrgNames.size(), 0, mMapDigitTrgNames.size());
  for (const auto& entry : mMapDigitTrgNames) {
    mHistTriggers->GetXaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());
  }
  getObjectsManager()->startPublishing(mHistTriggers.get());

  mHistTimeUpperFraction = std::make_unique<TH1F>("TimeUpperFraction", "Fraction of events under time window(-+190 channels);ChID;Fraction", sNCHANNELS_PM, 0, sNCHANNELS_PM);
  getObjectsManager()->startPublishing(mHistTimeUpperFraction.get());

  mHistTimeLowerFraction = std::make_unique<TH1F>("TimeLowerFraction", "Fraction of events below time window(-+190 channels);ChID;Fraction", sNCHANNELS_PM, 0, sNCHANNELS_PM);
  getObjectsManager()->startPublishing(mHistTimeLowerFraction.get());

  mHistTimeInWindow = std::make_unique<TH1F>("TimeInWindowFraction", "Fraction of events within time window(-+190 channels);ChID;Fraction", sNCHANNELS_PM, 0, sNCHANNELS_PM);
  getObjectsManager()->startPublishing(mHistTimeInWindow.get());

  getObjectsManager()->startPublishing(mRateOrA.get());
  getObjectsManager()->startPublishing(mRateOrAout.get());
  getObjectsManager()->startPublishing(mRateOrAin.get());
  getObjectsManager()->startPublishing(mRateTrgCharge.get());
  getObjectsManager()->startPublishing(mRateTrgNchan.get());
  getObjectsManager()->startPublishing(mRatesCanv.get());
  getObjectsManager()->startPublishing(mAmpl);
  getObjectsManager()->startPublishing(mTime);
}

void BasicPPTask::update(Trigger t, framework::ServiceRegistry&)
{
  auto mo = mDatabase->retrieveMO(mPathDigitQcTask, "TriggersCorrelation", t.timestamp, t.activity);
  auto hTrgCorr = mo ? (TH2F*)mo->getObject() : nullptr;
  mHistTriggers->Reset();
  auto getBinContent2Ddiag = [](TH2* hist, const std::string& binName) {
    return hist->GetBinContent(hist->GetXaxis()->FindBin(binName.c_str()), hist->GetYaxis()->FindBin(binName.c_str()));
  };
  if (!hTrgCorr) {
    ILOG(Error) << "MO \"TriggersCorrelation\" NOT retrieved!!!" << ENDM;
  } else {
    double totalStat{ 0 };
    for (int iBin = 1; iBin < mHistTriggers->GetXaxis()->GetNbins() + 1; iBin++) {
      std::string binName{ mHistTriggers->GetXaxis()->GetBinLabel(iBin) };
      const auto binContent = getBinContent2Ddiag(hTrgCorr, binName);
      mHistTriggers->SetBinContent(iBin, getBinContent2Ddiag(hTrgCorr, binName));
      totalStat += binContent;
    }
    mHistChDataNegBits->SetEntries(totalStat);
  }

  auto moChDataBits = mDatabase->retrieveMO(mPathDigitQcTask, "ChannelDataBits", t.timestamp, t.activity);
  auto hChDataBits = moChDataBits ? (TH2F*)moChDataBits->getObject() : nullptr;
  if (!hChDataBits) {
    ILOG(Error) << "MO \"ChannelDataBits\" NOT retrieved!!!" << ENDM;
  }
  auto moStatChannelID = mDatabase->retrieveMO(mPathDigitQcTask, "StatChannelID", t.timestamp, t.activity);
  auto hStatChannelID = moStatChannelID ? (TH2F*)moStatChannelID->getObject() : nullptr;
  if (!hStatChannelID) {
    ILOG(Error) << "MO \"StatChannelID\" NOT retrieved!!!" << ENDM;
  }
  mHistChDataNegBits->Reset();
  if (hChDataBits != nullptr && hStatChannelID != nullptr) {
    double totalStat{ 0 };
    for (int iBinX = 1; iBinX < hChDataBits->GetXaxis()->GetNbins() + 1; iBinX++) {
      for (int iBinY = 1; iBinY < hChDataBits->GetYaxis()->GetNbins() + 1; iBinY++) {
        const double nStatTotal = hStatChannelID->GetBinContent(iBinX);
        const double nStatPMbit = hChDataBits->GetBinContent(iBinX, iBinY);
        const double nStatNegPMbit = nStatTotal - nStatPMbit;
        totalStat += nStatNegPMbit;
        mHistChDataNegBits->SetBinContent(iBinX, iBinY, nStatNegPMbit);
      }
    }
    mHistChDataNegBits->SetEntries(totalStat);
  }

  auto mo2 = mDatabase->retrieveMO(mPathDigitQcTask, mCycleDurationMoName, t.timestamp, t.activity);
  auto hCycleDuration = mo2 ? (TH1D*)mo2->getObject() : nullptr;
  if (!hCycleDuration) {
    ILOG(Error) << "MO \"" << mCycleDurationMoName << "\" NOT retrieved!!!" << ENDM;
  }

  if (hTrgCorr && hCycleDuration) {
    double cycleDurationMS = 0;
    if (mCycleDurationMoName == "CycleDuration" || mCycleDurationMoName == "CycleDurationRange")
      // assume MO stores cycle duration in ns
      cycleDurationMS = hCycleDuration->GetBinContent(1) / 1e6; // ns -> ms
    else if (mCycleDurationMoName == "CycleDurationNTF")
      // assume MO stores cycle duration in number of TF
      cycleDurationMS = hCycleDuration->GetBinContent(1) * mNumOrbitsInTF * o2::constants::lhc::LHCOrbitNS / 1e6; // ns ->ms

    int n = mRateOrA->GetN();

    double eps = 1e-8;
    if (cycleDurationMS < eps) {
      ILOG(Warning) << "cycle duration = " << cycleDurationMS << " ms, almost zero - cannot compute trigger rates!" << ENDM;
    } else {
      mRateOrA->SetPoint(n, n, getBinContent2Ddiag(hTrgCorr, "OrA") / cycleDurationMS);
      mRateOrAout->SetPoint(n, n, getBinContent2Ddiag(hTrgCorr, "OrAout") / cycleDurationMS);
      mRateOrAin->SetPoint(n, n, getBinContent2Ddiag(hTrgCorr, "OrAin") / cycleDurationMS);
      mRateTrgCharge->SetPoint(n, n, getBinContent2Ddiag(hTrgCorr, "TrgCharge") / cycleDurationMS);
      mRateTrgNchan->SetPoint(n, n, getBinContent2Ddiag(hTrgCorr, "TrgNchan") / cycleDurationMS);
    }

    mRatesCanv->cd();
    float vmin = std::min({ mRateOrA->GetYaxis()->GetXmin(), mRateOrAout->GetYaxis()->GetXmin(), mRateOrAin->GetYaxis()->GetXmin(), mRateTrgCharge->GetYaxis()->GetXmin(), mRateTrgNchan->GetYaxis()->GetXmin() });
    float vmax = std::max({ mRateOrA->GetYaxis()->GetXmax(), mRateOrAout->GetYaxis()->GetXmax(), mRateOrAin->GetYaxis()->GetXmax(), mRateTrgCharge->GetYaxis()->GetXmax(), mRateTrgNchan->GetYaxis()->GetXmax() });

    auto hAxis = mRateOrA->GetHistogram();
    hAxis->GetYaxis()->SetTitleOffset(1.4);
    hAxis->SetMinimum(vmin);
    hAxis->SetMaximum(vmax * 1.1);
    hAxis->SetTitle("FV0 trigger rates");
    hAxis->SetLineWidth(0);
    hAxis->Draw("AXIS");

    mRateOrA->Draw("PL,SAME");
    mRateOrAout->Draw("PL,SAME");
    mRateOrAin->Draw("PL,SAME");
    mRateTrgCharge->Draw("PL,SAME");
    mRateTrgNchan->Draw("PL,SAME");
    TLegend* leg = gPad->BuildLegend();
    leg->SetFillStyle(1);
  }

  auto mo3 = mDatabase->retrieveMO(mPathDigitQcTask, "AmpPerChannel", t.timestamp, t.activity);
  auto hAmpPerChannel = mo3 ? (TH2D*)mo3->getObject() : nullptr;
  if (!hAmpPerChannel) {
    ILOG(Error) << "MO \"AmpPerChannel\" NOT retrieved!!!"
                << ENDM;
  }
  auto mo4 = mDatabase->retrieveMO(mPathDigitQcTask, "TimePerChannel", t.timestamp, t.activity);
  auto hTimePerChannel = mo4 ? (TH2D*)mo4->getObject() : nullptr;
  if (!hTimePerChannel) {
    ILOG(Error) << "MO \"TimePerChannel\" NOT retrieved!!!"
                << ENDM;
  } else {
    auto projLower = hTimePerChannel->ProjectionX("projLower", 0, hTimePerChannel->GetYaxis()->FindBin(-190.));
    auto projUpper = hTimePerChannel->ProjectionX("projUpper", hTimePerChannel->GetYaxis()->FindBin(190.), -1);
    auto projInWindow = hTimePerChannel->ProjectionX("projInWindow", hTimePerChannel->GetYaxis()->FindBin(-190.), hTimePerChannel->GetYaxis()->FindBin(190.));
    auto projFull = hTimePerChannel->ProjectionX("projFull");
    mHistTimeUpperFraction->Divide(projUpper, projFull);
    mHistTimeLowerFraction->Divide(projLower, projFull);
    mHistTimeInWindow->Divide(projInWindow, projFull);
    delete projLower;
    delete projUpper;
    delete projInWindow;
    delete projFull;
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
