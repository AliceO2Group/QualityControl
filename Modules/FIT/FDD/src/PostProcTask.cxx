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
/// \file   PostProcTask.cxx
/// \author Sebastian Bysiak sbysiak@cern.ch
///

#include "FDD/PostProcTask.h"
#include "QualityControl/QcInfoLogger.h"
#include "CommonConstants/LHCConstants.h"
#include "DataFormatsParameters/GRPLHCIFData.h"

#include "FITCommon/HelperHist.h"
#include "FITCommon/HelperCommon.h"
#include "FITCommon/HelperFIT.h"

#include <TH1F.h>
#include <TH2F.h>
#include <TCanvas.h>
#include <TPad.h>
#include <TLegend.h>
#include <TProfile.h>

using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control_modules::fit;

namespace o2::quality_control_modules::fdd
{

PostProcTask::~PostProcTask()
{
  delete mAmpl;
  delete mTime;
}

void PostProcTask::configure(const boost::property_tree::ptree& config)
{
  mCcdbUrl = config.get_child("qc.config.conditionDB.url").get_value<std::string>();

  const char* configPath = Form("qc.postprocessing.%s", getID().c_str());
  const char* configCustom = Form("%s.custom", configPath);
  ILOG(Info, Support) << "configPath = " << configPath << ENDM;

  auto node = config.get_child_optional(Form("%s.custom.pathGrpLhcIf", configPath));
  if (node) {
    mPathGrpLhcIf = node.get_ptr()->get_child("").get_value<std::string>();
    ILOG(Debug, Support) << "configure() : using pathBunchFilling = \"" << mPathGrpLhcIf << "\"" << ENDM;
  } else {
    mPathGrpLhcIf = "GLO/Config/GRPLHCIF";
    ILOG(Debug, Support) << "configure() : using default pathBunchFilling = \"" << mPathGrpLhcIf << "\"" << ENDM;
  }

  node = config.get_child_optional(Form("%s.custom.numOrbitsInTF", configPath));
  if (node) {
    mNumOrbitsInTF = std::stoi(node.get_ptr()->get_child("").get_value<std::string>());
    ILOG(Debug, Support) << "configure() : using numOrbitsInTF = " << mNumOrbitsInTF << ENDM;
  } else {
    mNumOrbitsInTF = 256;
    ILOG(Debug, Support) << "configure() : using default numOrbitsInTF = " << mNumOrbitsInTF << ENDM;
  }

  node = config.get_child_optional(Form("%s.custom.cycleDurationMoName", configPath));
  if (node) {
    mCycleDurationMoName = node.get_ptr()->get_child("").get_value<std::string>();
    ILOG(Debug, Support) << "configure() : using cycleDurationMoName = \"" << mCycleDurationMoName << "\"" << ENDM;
  } else {
    mCycleDurationMoName = "CycleDurationNTF";
    ILOG(Debug, Support) << "configure() : using default cycleDurationMoName = \"" << mCycleDurationMoName << "\"" << ENDM;
  }

  node = config.get_child_optional(Form("%s.custom.pathDigitQcTask", configPath));
  if (node) {
    mPathDigitQcTask = node.get_ptr()->get_child("").get_value<std::string>();
    ILOG(Debug, Support) << "configure() : using pathDigitQcTask = \"" << mPathDigitQcTask << "\"" << ENDM;
  } else {
    mPathDigitQcTask = "FDD/MO/DigitQcTask/";
    ILOG(Debug, Support) << "configure() : using default pathDigitQcTask = \"" << mPathDigitQcTask << "\"" << ENDM;
  }

  node = config.get_child_optional(Form("%s.custom.timestampSourceLhcIf", configPath));
  if (node) {
    mTimestampSourceLhcIf = node.get_ptr()->get_child("").get_value<std::string>();
    if (mTimestampSourceLhcIf == "last" || mTimestampSourceLhcIf == "trigger" || mTimestampSourceLhcIf == "metadata" || mTimestampSourceLhcIf == "validUntil") {
      ILOG(Debug, Support) << "configure() : using timestampSourceLhcIf = \"" << mTimestampSourceLhcIf << "\"" << ENDM;
    } else {
      auto prev = mTimestampSourceLhcIf;
      mTimestampSourceLhcIf = "trigger";
      ILOG(Warning, Support) << "configure() : invalid value for timestampSourceLhcIf = \"" << prev
                             << "\"\n available options are \"last\", \"trigger\" or \"metadata\""
                             << "\n fallback to default: \"" << mTimestampSourceLhcIf << "\"" << ENDM;
    }
  } else {
    mTimestampSourceLhcIf = "trigger";
    ILOG(Debug, Support) << "configure() : using default timestampSourceLhcIf = \"" << mTimestampSourceLhcIf << "\"" << ENDM;
  }
  mLowTimeThreshold = helper::getConfigFromPropertyTree<int>(config, Form("%s.lowTimeThreshold", configCustom), -192);
  mUpTimeThreshold = helper::getConfigFromPropertyTree<int>(config, Form("%s.upTimeThreshold", configCustom), 192);
  mLowAmpSat = helper::getConfigFromPropertyTree<double>(config, Form("%s.lowAmpSat", configCustom), 1.);
  mUpAmpSat = helper::getConfigFromPropertyTree<double>(config, Form("%s.upAmpSat", configCustom), 3600.);
}

void PostProcTask::initialize(Trigger, framework::ServiceRegistryRef services)
{
  mDatabase = &services.get<o2::quality_control::repository::DatabaseInterface>();
  mCcdbApi.init(mCcdbUrl);

  mRateOrA = std::make_unique<TGraph>(0);
  mRateOrC = std::make_unique<TGraph>(0);
  mRateVertex = std::make_unique<TGraph>(0);
  mRateCentral = std::make_unique<TGraph>(0);
  mRateSemiCentral = std::make_unique<TGraph>(0);
  // mRatesCanv = std::make_unique<TCanvas>("cRates", "trigger rates");
  mAmpl = new TProfile("MeanAmplPerChannel", "mean ampl per channel;Channel;Ampl #mu #pm #sigma", sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mTime = new TProfile("MeanTimePerChannel", "mean time per channel;Channel;Time #mu #pm #sigma", sNCHANNELS_PM, 0, sNCHANNELS_PM);

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

  mMapChTrgNames.insert({ o2::fdd::ChannelData::kNumberADC, "NumberADC" });
  mMapChTrgNames.insert({ o2::fdd::ChannelData::kIsDoubleEvent, "IsDoubleEvent" });
  mMapChTrgNames.insert({ o2::fdd::ChannelData::kIsTimeInfoNOTvalid, "IsTimeInfoNOTvalid" });
  mMapChTrgNames.insert({ o2::fdd::ChannelData::kIsCFDinADCgate, "IsCFDinADCgate" });
  mMapChTrgNames.insert({ o2::fdd::ChannelData::kIsTimeInfoLate, "IsTimeInfoLate" });
  mMapChTrgNames.insert({ o2::fdd::ChannelData::kIsAmpHigh, "IsAmpHigh" });
  mMapChTrgNames.insert({ o2::fdd::ChannelData::kIsEventInTVDC, "IsEventInTVDC" });
  mMapChTrgNames.insert({ o2::fdd::ChannelData::kIsTimeInfoLost, "IsTimeInfoLost" });

  mHistChDataNegBits = std::make_unique<TH2F>("ChannelDataNegBits", "ChannelData negative bits per ChannelID;Channel;Negative bit", sNCHANNELS_PM, 0, sNCHANNELS_PM, mMapChTrgNames.size(), 0, mMapChTrgNames.size());
  for (const auto& entry : mMapChTrgNames) {
    std::string stBitName = "! " + entry.second;
    mHistChDataNegBits->GetYaxis()->SetBinLabel(entry.first + 1, stBitName.c_str());
  }
  getObjectsManager()->startPublishing(mHistChDataNegBits.get());
  getObjectsManager()->setDefaultDrawOptions(mHistChDataNegBits.get(), "COLZ");

  mMapDigitTrgNames.insert({ o2::fdd::Triggers::bitA, "OrA" });
  mMapDigitTrgNames.insert({ o2::fdd::Triggers::bitC, "OrC" });
  mMapDigitTrgNames.insert({ o2::fdd::Triggers::bitVertex, "Vertex" });
  mMapDigitTrgNames.insert({ o2::fdd::Triggers::bitCen, "Central" });
  mMapDigitTrgNames.insert({ o2::fdd::Triggers::bitSCen, "SemiCentral" });
  mMapDigitTrgNames.insert({ o2::fdd::Triggers::bitLaser, "Laser" });
  mMapDigitTrgNames.insert({ o2::fdd::Triggers::bitOutputsAreBlocked, "OutputsAreBlocked" });
  mMapDigitTrgNames.insert({ o2::fdd::Triggers::bitDataIsValid, "DataIsValid" });
  mHistTriggers = std::make_unique<TH1F>("Triggers", "Triggers from TCM", mMapDigitTrgNames.size(), 0, mMapDigitTrgNames.size());
  mHistBcPattern = std::make_unique<TH2F>("bcPattern", "BC pattern", sBCperOrbit, 0, sBCperOrbit, mMapDigitTrgNames.size(), 0, mMapDigitTrgNames.size());
  mHistBcTrgOutOfBunchColl = std::make_unique<TH2F>("OutOfBunchColl_BCvsTrg", "BC vs Triggers for out-of-bunch collisions;BC;Triggers", sBCperOrbit, 0, sBCperOrbit, mMapDigitTrgNames.size(), 0, mMapDigitTrgNames.size());

  for (const auto& entry : mMapDigitTrgNames) {
    mHistTriggers->GetXaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());
    mHistBcPattern->GetYaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());
    mHistBcTrgOutOfBunchColl->GetYaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());

    // depends on triggers set to bits 0-N
    if (entry.first >= mNumTriggers)
      continue;
    auto pairHistBC = mMapTrgHistBC.insert({ entry.first, new TH1D(Form("BC_%s", entry.second.c_str()), Form("BC for %s trigger;BC;counts;", entry.second.c_str()), sBCperOrbit, 0, sBCperOrbit) });
    if (pairHistBC.second) {
      getObjectsManager()->startPublishing(pairHistBC.first->second);
    }
  }

  const auto& lut = o2::fdd::SingleLUT::Instance().getVecMetadataFEE();
  auto lutSorted = lut;
  std::sort(lutSorted.begin(), lutSorted.end(), [](const auto& first, const auto& second) { return first.mModuleName < second.mModuleName; });
  uint8_t binPos{ 0 };
  for (const auto& lutEntry : lutSorted) {
    const auto& moduleName = lutEntry.mModuleName;
    const auto& moduleType = lutEntry.mModuleType;
    const auto& strChID = lutEntry.mChannelID;
    const auto& pairIt = mMapFEE2hash.insert({ moduleName, binPos });
    if (pairIt.second) {
      binPos++;
    }
    if (std::regex_match(strChID, std::regex("[[\\d]{1,3}"))) {
      int chID = std::stoi(strChID);
      if (chID < sNCHANNELS_PM) {
        mChID2PMhash[chID] = mMapFEE2hash[moduleName];
      } else {
        ILOG(Error, Support) << "Incorrect LUT entry: chID " << strChID << " | " << moduleName << ENDM;
      }
    } else if (moduleType != "TCM") {
      ILOG(Error, Support) << "Non-TCM module w/o numerical chID: chID " << strChID << " | " << moduleName << ENDM;
    } else if (moduleType == "TCM") {
      uint8_t mTCMhash = mMapFEE2hash[moduleName];
    }
  }
  mHistBcFeeOutOfBunchCollForVtxTrg = std::make_unique<TH2F>("OutOfBunchColl_BCvsFeeModulesForVtxTrg", "BC vs FEE Modules for out-of-bunch collisions for Vertex trg;BC;FEE Modules", sBCperOrbit, 0, sBCperOrbit, mMapFEE2hash.size(), 0, mMapFEE2hash.size());
  mHistBcPatternFee = std::make_unique<TH2F>("bcPatternForFeeModules", "BC pattern", sBCperOrbit, 0, sBCperOrbit, mMapFEE2hash.size(), 0, mMapFEE2hash.size());
  for (const auto& entry : mMapFEE2hash) {
    // ILOG(Warning, Support) << "============= mMapFEE2hash.second + 1: " << entry.second + 1
    //                        << " mMapFEE2hash.first.c_str(): " << entry.first.c_str() << ENDM;

    mHistBcPatternFee->GetYaxis()->SetBinLabel(entry.second + 1, entry.first.c_str());
    mHistBcFeeOutOfBunchCollForVtxTrg->GetYaxis()->SetBinLabel(entry.second + 1, entry.first.c_str());
  }

  getObjectsManager()->startPublishing(mHistBcFeeOutOfBunchCollForVtxTrg.get());
  getObjectsManager()->setDefaultDrawOptions(mHistBcFeeOutOfBunchCollForVtxTrg.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistBcPatternFee.get());
  getObjectsManager()->setDefaultDrawOptions(mHistBcPatternFee.get(), "COLZ");

  getObjectsManager()->startPublishing(mHistTriggers.get());
  getObjectsManager()->startPublishing(mHistBcPattern.get());
  getObjectsManager()->setDefaultDrawOptions(mHistBcPattern.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistBcTrgOutOfBunchColl.get());
  getObjectsManager()->setDefaultDrawOptions(mHistBcTrgOutOfBunchColl.get(), "COLZ");

  getObjectsManager()->startPublishing(mRateOrA.get());
  getObjectsManager()->startPublishing(mRateOrC.get());
  getObjectsManager()->startPublishing(mRateVertex.get());
  getObjectsManager()->startPublishing(mRateCentral.get());
  getObjectsManager()->startPublishing(mRateSemiCentral.get());
  // getObjectsManager()->startPublishing(mRatesCanv.get());
  getObjectsManager()->startPublishing(mAmpl);
  getObjectsManager()->startPublishing(mTime);

  for (int i = 0; i < getObjectsManager()->getNumberPublishedObjects(); i++) {
    TH1* obj = dynamic_cast<TH1*>(getObjectsManager()->getMonitorObject(i)->getObject());
    if (obj != nullptr) {
      obj->SetTitle((string("FDD ") + obj->GetTitle()).c_str());
    }
  }

  mMapBasicTrgBits = HelperTrgFIT::sMapBasicTrgBitsFDD;
  mHistTrgValidation = helper::registerHist<TH1F>(getObjectsManager(), "", "TrgValidation", "FDD SW + HW only to validated triggers fraction", mMapBasicTrgBits);
  mHistTimeInWindow = helper::registerHist<TH1F>(getObjectsManager(), "", "TimeInWindowFraction", Form("FDD Fraction of events with CFD in time gate(%i,%i) vs ChannelID;ChannelID;Event fraction with CFD in time gate", mLowTimeThreshold, mUpTimeThreshold), sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mHistCFDEff = helper::registerHist<TH1F>(getObjectsManager(), "", "CFD_efficiency", "FDD Fraction of events with CFD in ADC gate vs ChannelID;ChannelID;Event fraction with CFD in ADC gate;", sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mHistAmpSaturation = helper::registerHist<TH1F>(getObjectsManager(), "", "AmpSaturation", Form("FDD Fraction of charge in [%d, %d] ADC;ChannelID;Fraction", mLowAmpSat, mUpAmpSat), sNCHANNELS_PM, 0, sNCHANNELS_PM);
}

void PostProcTask::update(Trigger t, framework::ServiceRegistryRef)
{
  auto mo = mDatabase->retrieveMO(mPathDigitQcTask, "TriggersCorrelation", t.timestamp, t.activity);
  auto hTrgCorr = mo ? dynamic_cast<TH2F*>(mo->getObject()) : nullptr;
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
  auto hChDataBits = moChDataBits ? dynamic_cast<TH2F*>(moChDataBits->getObject()) : nullptr;
  if (!hChDataBits) {
    ILOG(Error) << "MO \"ChannelDataBits\" NOT retrieved!!!" << ENDM;
  }
  auto moStatChannelID = mDatabase->retrieveMO(mPathDigitQcTask, "StatChannelID", t.timestamp, t.activity);
  auto hStatChannelID = moStatChannelID ? dynamic_cast<TH1F*>(moStatChannelID->getObject()) : nullptr;
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
  auto hCycleDuration = mo2 ? dynamic_cast<TH1D*>(mo2->getObject()) : nullptr;
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
      mRateOrC->SetPoint(n, n, getBinContent2Ddiag(hTrgCorr, "OrC") / cycleDurationMS);
      mRateVertex->SetPoint(n, n, getBinContent2Ddiag(hTrgCorr, "Vertex") / cycleDurationMS);
      mRateCentral->SetPoint(n, n, getBinContent2Ddiag(hTrgCorr, "Central") / cycleDurationMS);
      mRateSemiCentral->SetPoint(n, n, getBinContent2Ddiag(hTrgCorr, "SemiCentral") / cycleDurationMS);
    }
    /*
        mRatesCanv->cd();
        float vmin = std::min({ mRateOrA->GetYaxis()->GetXmin(), mRateOrC->GetYaxis()->GetXmin(), mRateVertex->GetYaxis()->GetXmin(), mRateCentral->GetYaxis()->GetXmin(), mRateSemiCentral->GetYaxis()->GetXmin() });
        float vmax = std::max({ mRateOrA->GetYaxis()->GetXmax(), mRateOrC->GetYaxis()->GetXmax(), mRateVertex->GetYaxis()->GetXmax(), mRateCentral->GetYaxis()->GetXmax(), mRateSemiCentral->GetYaxis()->GetXmax() });

        auto hAxis = mRateOrA->GetHistogram();
        hAxis->GetYaxis()->SetTitleOffset(1.4);
        hAxis->SetMinimum(vmin);
        hAxis->SetMaximum(vmax * 1.1);
        hAxis->SetTitle("FDD trigger rates");
        hAxis->SetLineWidth(0);
        hAxis->Draw("AXIS");

        mRateOrA->Draw("PL,SAME");
        mRateOrC->Draw("PL,SAME");
        mRateVertex->Draw("PL,SAME");
        mRateCentral->Draw("PL,SAME");
        mRateSemiCentral->Draw("PL,SAME");
        TLegend* leg = gPad->BuildLegend();
        leg->SetFillStyle(1);
        mRatesCanv->Modified();
        mRatesCanv->Update();
        */
  }

  auto mo3 = mDatabase->retrieveMO(mPathDigitQcTask, "AmpPerChannel", t.timestamp, t.activity);
  auto hAmpPerChannel = mo3 ? dynamic_cast<TH2F*>(mo3->getObject()) : nullptr;
  if (!hAmpPerChannel) {
    ILOG(Error) << "MO \"AmpPerChannel\" NOT retrieved!!!"
                << ENDM;
  } else {
    {
      std::unique_ptr<TH1D> projNom(hAmpPerChannel->ProjectionX("projNom", hAmpPerChannel->GetYaxis()->FindBin(1.0), -1));
      std::unique_ptr<TH1D> projDen(hAmpPerChannel->ProjectionX("projDen"));
      mHistCFDEff->Divide(projNom.get(), projDen.get());
    }
    {
      std::unique_ptr<TH1D> projNom(hAmpPerChannel->ProjectionX("projNom", hAmpPerChannel->GetYaxis()->FindBin(mLowAmpSat), hAmpPerChannel->GetYaxis()->FindBin(mUpAmpSat)));
      std::unique_ptr<TH1D> projDen(hAmpPerChannel->ProjectionX("projDen", hAmpPerChannel->GetYaxis()->FindBin(mLowAmpSat), hAmpPerChannel->GetNbinsY()));
      mHistAmpSaturation->Divide(projNom.get(), projDen.get());
    }
  }
  auto mo4 = mDatabase->retrieveMO(mPathDigitQcTask, "TimePerChannel", t.timestamp, t.activity);
  auto hTimePerChannel = mo4 ? dynamic_cast<TH2F*>(mo4->getObject()) : nullptr;
  if (!hTimePerChannel) {
    ILOG(Error) << "MO \"TimePerChannel\" NOT retrieved!!!"
                << ENDM;
  } else {
    std::unique_ptr<TH1D> projInWindow(hTimePerChannel->ProjectionX("projInWindow", hTimePerChannel->GetYaxis()->FindBin(mLowTimeThreshold), hTimePerChannel->GetYaxis()->FindBin(mUpTimeThreshold)));
    std::unique_ptr<TH1D> projFull(hTimePerChannel->ProjectionX("projFull"));
    mHistTimeInWindow->Divide(projInWindow.get(), projFull.get());
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

  auto moBCvsTriggers = mDatabase->retrieveMO(mPathDigitQcTask, "BCvsTriggers", t.timestamp, t.activity);
  auto hBcVsTrg = moBCvsTriggers ? dynamic_cast<TH2F*>(moBCvsTriggers->getObject()) : nullptr;
  if (!hBcVsTrg) {
    ILOG(Error, Support) << "MO \"BCvsTriggers\" NOT retrieved!!!" << ENDM;
    return;
  }

  for (const auto& entry : mMapTrgHistBC) {
    hBcVsTrg->ProjectionX(entry.second->GetName(), entry.first + 1, entry.first + 1);
  }

  long ts = 999;
  if (mTimestampSourceLhcIf == "last") {
    ts = -1;
  } else if (mTimestampSourceLhcIf == "trigger") {
    ts = t.timestamp;
  } else if (mTimestampSourceLhcIf == "validUntil") {
    ts = t.activity.mValidity.getMax();
  } else if (mTimestampSourceLhcIf == "metadata") {
    for (auto metainfo : moBCvsTriggers->getMetadataMap()) {
      if (metainfo.first == "TFcreationTime")
        ts = std::stol(metainfo.second);
    }
    if (ts > 1651500000000 && ts < 1651700000000)
      ILOG(Warning, Support) << "timestamp (read from TF via metadata) points to 02-04 May 2022"
                                " - make sure this is the data we are processing and not the default timestamp "
                                "(it may appear when running on digits w/o providing \"--hbfutils-config o2_tfidinfo.root\")"
                             << ENDM;
    if (ts == 999) {
      ILOG(Error) << "\"TFcreationTime\" not found in metadata, fallback to ts from trigger " << ENDM;
      ts = t.timestamp;
    }
  }

  std::map<std::string, std::string> metadata;
  std::map<std::string, std::string> headers;
  auto* lhcIf = mCcdbApi.retrieveFromTFileAny<o2::parameters::GRPLHCIFData>(mPathGrpLhcIf, metadata, ts, &headers);
  if (!lhcIf) {
    ILOG(Error, Support) << "object \"" << mPathGrpLhcIf << "\" NOT retrieved. OutOfBunchColTask will not produce valid QC plots." << ENDM;
    return;
  }
  const std::string bcName = lhcIf->getInjectionScheme();
  if (bcName.size() == 8) {
    if (bcName.compare("no_value")) {
      ILOG(Error, Support) << "Filling scheme not set. OutOfBunchColTask will not produce valid QC plots." << ENDM;
    }
  } else {
    ILOG(Info, Support) << "Filling scheme: " << bcName.c_str() << ENDM;
  }
  auto bcPattern = lhcIf->getBunchFilling();

  mHistBcPattern->Reset();
  for (int i = 0; i < sBCperOrbit + 1; i++) {
    for (int j = 0; j < mMapDigitTrgNames.size() + 1; j++) {
      mHistBcPattern->SetBinContent(i + 1, j + 1, bcPattern.testBC(i));
    }
  }

  // Create histogram with bc pattern for FEE modules
  mHistBcPatternFee->Reset();
  for (int i = 0; i < sBCperOrbit; i++) {
    for (int j = 0; j < mMapFEE2hash.size(); j++) {
      mHistBcPatternFee->SetBinContent(i + 1, j + 1, bcPattern.testBC(i));
    }
  }

  mHistBcTrgOutOfBunchColl->Reset();
  float vmax = hBcVsTrg->GetBinContent(hBcVsTrg->GetMaximumBin());
  mHistBcTrgOutOfBunchColl->Add(hBcVsTrg, mHistBcPattern.get(), 1, -1 * vmax);
  for (int i = 0; i < sBCperOrbit + 1; i++) {
    for (int j = 0; j < mMapDigitTrgNames.size() + 1; j++) {
      if (mHistBcTrgOutOfBunchColl->GetBinContent(i + 1, j + 1) < 0) {
        mHistBcTrgOutOfBunchColl->SetBinContent(i + 1, j + 1, 0); // is it too slow?
      }
    }
  }
  mHistBcTrgOutOfBunchColl->SetEntries(mHistBcTrgOutOfBunchColl->Integral(1, sBCperOrbit, 1, mMapDigitTrgNames.size()));
  for (int iBin = 1; iBin < mMapDigitTrgNames.size() + 1; iBin++) {
    const std::string metadataKey = "BcVsTrgIntegralBin" + std::to_string(iBin);
    const std::string metadataValue = std::to_string(hBcVsTrg->Integral(1, sBCperOrbit, iBin, iBin));
    getObjectsManager()->getMonitorObject(mHistBcTrgOutOfBunchColl->GetName())->addOrUpdateMetadata(metadataKey, metadataValue);
    ILOG(Info, Support) << metadataKey << ":" << metadataValue << ENDM;
  }

  auto moTriggersSoftwareVsTCM = mDatabase->retrieveMO(mPathDigitQcTask, "TriggersSoftwareVsTCM", t.timestamp, t.activity);
  auto hTriggersSoftwareVsTCM = moTriggersSoftwareVsTCM ? dynamic_cast<TH2F*>(moTriggersSoftwareVsTCM->getObject()) : nullptr;
  if (hTriggersSoftwareVsTCM != nullptr) {
    std::unique_ptr<TH1D> projOnlyHWorSW(hTriggersSoftwareVsTCM->ProjectionX("projOnlyHWorSW", 1, 2));
    std::unique_ptr<TH1D> projValidatedSWandHW(hTriggersSoftwareVsTCM->ProjectionX("projValidatedSWandHW", 4, 4));
    projOnlyHWorSW->LabelsDeflate();
    projValidatedSWandHW->LabelsDeflate();
    mHistTrgValidation->Divide(projOnlyHWorSW.get(), projValidatedSWandHW.get());
  }
  /*
    // Download histogram BCvsFEEmodulesForVtxTrg from database
    auto moBcVsFeeModulesForVtxTrg = mDatabase->retrieveMO(mPathDigitQcTask, "BCvsFEEmodulesForVtxTrg", t.timestamp, t.activity);
    auto hBcVsFeeModulesForVtxTrg = moBcVsFeeModulesForVtxTrg ? dynamic_cast<TH2F*>(moBcVsFeeModulesForVtxTrg->getObject()) : nullptr;

    if (!hBcVsFeeModulesForVtxTrg) {
      ILOG(Error, Support) << "MO \"BCvsFEEmodulesForVtxTrg\" NOT retrieved!!!" << ENDM;
      return;
    } else {

      mHistBcFeeOutOfBunchCollForVtxTrg->Reset();
      float vmax = hBcVsFeeModulesForVtxTrg->GetBinContent(hBcVsFeeModulesForVtxTrg->GetMaximumBin());
      mHistBcFeeOutOfBunchCollForVtxTrg->Add(hBcVsFeeModulesForVtxTrg, mHistBcPatternFee.get(), 1, -1 * vmax);

      for (int i = 0; i < sBCperOrbit; i++) {
        for (int j = 0; j < mMapFEE2hash.size() ; j++) {
          if (mHistBcFeeOutOfBunchCollForVtxTrg->GetBinContent(i + 1, j + 1) < 0) {
            mHistBcFeeOutOfBunchCollForVtxTrg->SetBinContent(i + 1, j + 1, 0);
          }
        }
      }

      // Add metadata to histogram OutOfBunchColl_BCvsFeeModulesForOrATrg
      mHistBcFeeOutOfBunchCollForVtxTrg->SetEntries(mHistBcFeeOutOfBunchCollForVtxTrg->Integral(1, sBCperOrbit, 1, mMapFEE2hash.size()));
      for (int iBin = 1; iBin <= mMapFEE2hash.size(); iBin++) {
        const std::string metadataKey = std::to_string(iBin);
        const std::string metadataValue = std::to_string(hBcVsFeeModulesForVtxTrg->Integral(1, sBCperOrbit, iBin, iBin));
        getObjectsManager()->getMonitorObject(mHistBcFeeOutOfBunchCollForVtxTrg->GetName())->addOrUpdateMetadata(metadataKey, metadataValue);
      }
    }*/
}

void PostProcTask::finalize(Trigger t, framework::ServiceRegistryRef)
{
}

} // namespace o2::quality_control_modules::fdd
