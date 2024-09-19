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
/// \author Artur Furs afurs@cern.ch
///

#include "FT0/PostProcTask.h"
#include "QualityControl/QcInfoLogger.h"
#include "CommonConstants/LHCConstants.h"
#include "DataFormatsParameters/GRPLHCIFData.h"
#include "DataFormatsFT0/Digit.h"
#include "DataFormatsFT0/ChannelData.h"

#include <TH1F.h>
#include <TH2F.h>
#include <TCanvas.h>
#include <TPad.h>
#include <TLegend.h>
#include <TProfile.h>

#include "FITCommon/HelperHist.h"
#include "FITCommon/HelperCommon.h"

#include <iostream>

using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control_modules::fit;

namespace o2::quality_control_modules::ft0
{

PostProcTask::~PostProcTask()
{
}

void PostProcTask::configure(const boost::property_tree::ptree& config)
{
  const char* configPath = Form("qc.postprocessing.%s", getID().c_str());
  const char* configCustom = Form("%s.custom", configPath);
  auto cfgPath = [&configCustom](const std::string& entry) {
    return Form("%s.%s", configCustom, entry.c_str());
  };
  mPostProcHelper.configure(config, configPath, "FT0");

  // Detector related configs
  mLowTimeThreshold = helper::getConfigFromPropertyTree<int>(config, cfgPath("lowTimeThreshold"), -192);
  mUpTimeThreshold = helper::getConfigFromPropertyTree<int>(config, cfgPath("upTimeThreshold"), 192);
  mAsynchChannelLogic = helper::getConfigFromPropertyTree<std::string>(config, cfgPath("asynchChannelLogic"), "standard");
  mIsFirstIter = true; // to be sure

  // TO REMOVE
  // VERY BAD SOLUTION, YOU SHOULDN'T USE IT
  const std::string del = ",";
  const std::string strChannelIDs = helper::getConfigFromPropertyTree<std::string>(config, cfgPath("channelIDs"), "");
  const std::string strHistsToDecompose = helper::getConfigFromPropertyTree<std::string>(config, cfgPath("histsToDecompose"), "");
  if (strChannelIDs.size() > 0 && strHistsToDecompose.size() > 0) {
    mVecChannelIDs = helper::parseParameters<unsigned int>(strChannelIDs, del);
    mVecHistsToDecompose = helper::parseParameters<std::string>(strHistsToDecompose, del);
  }
}

void PostProcTask::reset()
{
  mHistChDataNOTbits.reset();
  mHistTriggers.reset();
  mHistTriggerRates.reset();
  mHistTimeInWindow.reset();
  mHistCFDEff.reset();
  mHistChannelID_outOfBC.reset();
  mHistTrg_outOfBC.reset();
  mHistTrgValidation.reset();
  mHistBcPattern.reset();
  mHistBcTrgOutOfBunchColl.reset();
  mAmpl.reset();
  mTime.reset();
  mMapHistsToDecompose.clear();
}

void PostProcTask::initialize(Trigger trg, framework::ServiceRegistryRef services)
{
  reset(); // for SSS procedure
  mPostProcHelper.initialize(trg, services);
  mIsFirstIter = true; // to be sure

  mHistChDataNOTbits = helper::registerHist<TH2F>(getObjectsManager(), quality_control::core::PublicationPolicy::ThroughStop, "COLZ", "ChannelDataNegBits", "ChannelData NOT PM bits per ChannelID;Channel;Negative bit", sNCHANNELS_PM, 0, sNCHANNELS_PM, mMapPMbits);
  mHistTriggers = helper::registerHist<TH1F>(getObjectsManager(), quality_control::core::PublicationPolicy::ThroughStop, "", "Triggers", "Triggers from TCM", mMapTechTrgBitsExtra);
  mHistTriggerRates = helper::registerHist<TH1F>(getObjectsManager(), quality_control::core::PublicationPolicy::ThroughStop, "HIST", "TriggerRates", "Trigger rates; Triggers; Rate [kHz]", mMapTechTrgBitsExtra);
  mHistBcTrgOutOfBunchColl = helper::registerHist<TH2F>(getObjectsManager(), quality_control::core::PublicationPolicy::ThroughStop, "COLZ", "OutOfBunchColl_BCvsTrg", "BC vs Triggers for out-of-bunch collisions;BC;Triggers", sBCperOrbit, 0, sBCperOrbit, mMapTechTrgBitsExtra);
  mHistBcPattern = helper::registerHist<TH2F>(getObjectsManager(), quality_control::core::PublicationPolicy::ThroughStop, "COLZ", "bcPattern", "BC pattern", sBCperOrbit, 0, sBCperOrbit, mMapTechTrgBitsExtra);
  mHistTimeInWindow = helper::registerHist<TH1F>(getObjectsManager(), quality_control::core::PublicationPolicy::ThroughStop, "", "TimeInWindowFraction", Form("Fraction of events with CFD in time gate(%i,%i) vs ChannelID;ChannelID;Event fraction with CFD in time gate", mLowTimeThreshold, mUpTimeThreshold), sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mHistCFDEff = helper::registerHist<TH1F>(getObjectsManager(), quality_control::core::PublicationPolicy::ThroughStop, "", "CFD_efficiency", "Fraction of events with CFD in ADC gate vs ChannelID;ChannelID;Event fraction with CFD in ADC gate;", sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mHistChannelID_outOfBC = helper::registerHist<TH1F>(getObjectsManager(), quality_control::core::PublicationPolicy::ThroughStop, "", "ChannelID_outOfBC", "ChannelID, out of bunch", sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mHistTrg_outOfBC = helper::registerHist<TH1F>(getObjectsManager(), quality_control::core::PublicationPolicy::ThroughStop, "", "Triggers_outOfBC", "Trigger fraction, out of bunch", mMapTechTrgBitsExtra);
  mHistTrgValidation = helper::registerHist<TH1F>(getObjectsManager(), quality_control::core::PublicationPolicy::ThroughStop, "", "TrgValidation", "SW + HW only to validated triggers fraction", mMapTrgBits);
  mAmpl = helper::registerHist<TProfile>(getObjectsManager(), quality_control::core::PublicationPolicy::ThroughStop, "", "MeanAmplPerChannel", "mean ampl per channel;Channel;Ampl #mu #pm #sigma", o2::ft0::Constants::sNCHANNELS_PM, 0, o2::ft0::Constants::sNCHANNELS_PM);
  mTime = helper::registerHist<TProfile>(getObjectsManager(), quality_control::core::PublicationPolicy::ThroughStop, "", "MeanTimePerChannel", "mean time per channel;Channel;Time #mu #pm #sigma", o2::ft0::Constants::sNCHANNELS_PM, 0, o2::ft0::Constants::sNCHANNELS_PM);
}

void PostProcTask::update(Trigger trg, framework::ServiceRegistryRef serviceReg)
{
  mPostProcHelper.update(trg, serviceReg);

  auto getBinContent2Ddiag = [](TH2F* hist, const std::string& binName) {
    const auto xBin = hist->GetXaxis()->FindBin(binName.c_str());
    const auto yBin = hist->GetYaxis()->FindBin(binName.c_str());
    return hist->GetBinContent(xBin, yBin);
  };

  // Trigger correlation
  auto hTrgCorr = mPostProcHelper.template getObject<TH2F>("TriggersCorrelation");
  mHistTriggers->Reset();
  if (hTrgCorr) {
    double totalStat{ 0 };
    for (int iBin = 1; iBin < mHistTriggers->GetXaxis()->GetNbins() + 1; iBin++) {
      const std::string binName{ mHistTriggers->GetXaxis()->GetBinLabel(iBin) };
      const auto binContent = getBinContent2Ddiag(hTrgCorr.get(), binName);
      mHistTriggers->SetBinContent(iBin, binContent);
      totalStat += binContent;
    }
    mHistTriggers->SetEntries(totalStat);
  }

  // Trigger rates
  mHistTriggerRates->Reset();
  if (mPostProcHelper.IsNonEmptySample()) {
    mHistTriggerRates->Add(mHistTriggers.get());
    constexpr double factor = 1e3;                                                    // Hz -> kHz
    const double samplePeriod = 1. / (factor * mPostProcHelper.mCurrSampleLengthSec); // in sec^-1 units
    mHistTriggerRates->Scale(samplePeriod);
  }
  // PM bits
  auto hChDataBits = mPostProcHelper.template getObject<TH2F>("ChannelDataBits");
  auto hStatChannelID = mPostProcHelper.template getObject<TH1F>("StatChannelID");
  mHistChDataNOTbits->Reset();
  if (hChDataBits && hStatChannelID) {
    double totalStat{ 0 };
    for (int iBinX = 1; iBinX < hChDataBits->GetXaxis()->GetNbins() + 1; iBinX++) {
      for (int iBinY = 1; iBinY < hChDataBits->GetYaxis()->GetNbins() + 1; iBinY++) {
        const double nStatTotal = hStatChannelID->GetBinContent(iBinX);
        const double nStatPMbit = hChDataBits->GetBinContent(iBinX, iBinY);
        const double nStatNegPMbit = nStatTotal - nStatPMbit;
        totalStat += nStatNegPMbit;
        mHistChDataNOTbits->SetBinContent(iBinX, iBinY, nStatNegPMbit);
      }
    }
    mHistChDataNOTbits->SetEntries(totalStat);
  }
  // Amplitudes
  auto hAmpPerChannel = mPostProcHelper.template getObject<TH2F>("AmpPerChannel");
  if (hAmpPerChannel) {
    std::unique_ptr<TH1D> projNom(hAmpPerChannel->ProjectionX("projNom", hAmpPerChannel->GetYaxis()->FindBin(1.0), -1));
    std::unique_ptr<TH1D> projDen(hAmpPerChannel->ProjectionX("projDen"));
    mHistCFDEff->Divide(projNom.get(), projDen.get());
  }
  // Times
  auto hTimePerChannel = mPostProcHelper.template getObject<TH2F>("TimePerChannel");
  if (hTimePerChannel) {
    std::unique_ptr<TH1D> projInWindow(hTimePerChannel->ProjectionX("projInWindow", hTimePerChannel->GetYaxis()->FindBin(mLowTimeThreshold), hTimePerChannel->GetYaxis()->FindBin(mUpTimeThreshold)));
    std::unique_ptr<TH1D> projFull(hTimePerChannel->ProjectionX("projFull"));
    mHistTimeInWindow->Divide(projInWindow.get(), projFull.get());
  }

  if (hAmpPerChannel && hTimePerChannel) {
    hAmpPerChannel->ProfileX("MeanAmplPerChannel");
    hTimePerChannel->ProfileX("MeanTimePerChannel");
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

  auto hBcVsTrg = mPostProcHelper.template getObject<TH2F>("BCvsTriggers");
  const auto& bcPattern = mPostProcHelper.getGRPLHCIFData().getBunchFilling();

  // Should be removed
  // BC pattern
  mHistBcPattern->Reset();
  for (int i = 0; i < sBCperOrbit + 1; i++) {
    for (int j = 0; j < mMapTechTrgBitsExtra.size() + 1; j++) {
      mHistBcPattern->SetBinContent(i + 1, j + 1, bcPattern.testBC(i));
    }
  }
  // Should be removed
  // Triggers in non-collision BCs
  mHistBcTrgOutOfBunchColl->Reset();
  float vmax = hBcVsTrg->GetBinContent(hBcVsTrg->GetMaximumBin());
  mHistBcTrgOutOfBunchColl->Add(hBcVsTrg.get(), mHistBcPattern.get(), 1, -1 * vmax);
  for (int i = 0; i < sBCperOrbit + 1; i++) {
    for (int j = 0; j < mMapTechTrgBitsExtra.size() + 1; j++) {
      if (mHistBcTrgOutOfBunchColl->GetBinContent(i + 1, j + 1) < 0) {
        mHistBcTrgOutOfBunchColl->SetBinContent(i + 1, j + 1, 0); // is it too slow?
      }
    }
  }
  mHistBcTrgOutOfBunchColl->SetEntries(mHistBcTrgOutOfBunchColl->Integral(1, sBCperOrbit, 1, mMapTechTrgBitsExtra.size()));
  for (int iBin = 1; iBin < mMapTechTrgBitsExtra.size() + 1; iBin++) {
    const std::string metadataKey = "BcVsTrgIntegralBin" + std::to_string(iBin);
    const std::string metadataValue = std::to_string(hBcVsTrg->Integral(1, sBCperOrbit, iBin, iBin));
    getObjectsManager()->getMonitorObject(mHistBcTrgOutOfBunchColl->GetName())->addOrUpdateMetadata(metadataKey, metadataValue);
    ILOG(Info, Support) << metadataKey << ":" << metadataValue << ENDM;
  }
  // Functor for in/out colliding BC fraction calculation
  auto calcFraction = [&pattern = bcPattern](const auto& histSrc, auto& histDst) {
    for (int iBin = 0; iBin < histSrc->GetYaxis()->GetNbins(); iBin++) {
      double cntInBC{ 0 };
      double cntOutOfBC{ 0 };
      const auto binPos = iBin + 1;
      std::unique_ptr<TH1D> proj(histSrc->ProjectionX("proj", binPos, binPos));
      for (int iBC = 0; iBC < proj->GetXaxis()->GetNbins(); iBC++) {
        if (pattern.testBC(iBC)) {
          cntInBC += proj->GetBinContent(iBC + 1);
        } else {
          cntOutOfBC += proj->GetBinContent(iBC + 1);
        }
      }
      const auto val = cntInBC > 0 ? cntOutOfBC / cntInBC : 0;
      histDst->SetBinContent(binPos, val);
    }
  };

  // New version for trigger fraction in non colliding BCa
  mHistTrg_outOfBC->Reset();
  calcFraction(hBcVsTrg, mHistTrg_outOfBC);

  // Channel stats in non-collision BCs
  auto hChIDvsBC = mPostProcHelper.template getObject<TH2F>("ChannelIDperBC");
  if (hChIDvsBC) {
    auto hChID_InBC = std::make_unique<TH1D>("hChID_InBC", "hChID_InBC", hChIDvsBC->GetYaxis()->GetNbins(), 0, hChIDvsBC->GetYaxis()->GetNbins());
    auto hChID_OutOfBC = std::make_unique<TH1D>("hChID_OutOfBC", "hChID_OutOfBC", hChIDvsBC->GetYaxis()->GetNbins(), 0, hChIDvsBC->GetYaxis()->GetNbins());
    if (mAsynchChannelLogic == std::string{ "standard" }) {
      // Standard logic, calculates outCollBC/inCollBC ratio
      calcFraction(hChIDvsBC, mHistChannelID_outOfBC);
    } else if (mAsynchChannelLogic == std::string{ "normalizedTrains" }) {
      // Train normlization, normalizes events in trains and then calculates outCollBC/inCollBC ratio
      const auto mapTrainBC = helper::getMapBCtrains(bcPattern.getBCPattern());
      for (int iChID = 0; iChID < hChIDvsBC->GetYaxis()->GetNbins(); iChID++) {
        double cntOutOfBC{ 0 };
        double cntInBC{ 0 };
        for (const auto& train : mapTrainBC) {
          double eventsTrain{ 0 };
          for (int iBC = train.first; iBC < train.first + train.second; iBC++) {
            eventsTrain += hChIDvsBC->GetBinContent(iBC + 1, iChID + 1);
          }
          if (train.second > 0) {
            cntInBC += eventsTrain / train.second;
          }
        }
        for (int iBC = 0; iBC < hChIDvsBC->GetXaxis()->GetNbins(); iBC++) {
          if (!bcPattern.testBC(iBC)) {
            cntOutOfBC += hChIDvsBC->GetBinContent(iBC + 1, iChID + 1);
          }
        }
        const auto val = cntInBC > 0 ? cntOutOfBC / cntInBC : 0;
        mHistChannelID_outOfBC->SetBinContent(iChID + 1, val);
      }
    }
  }

  // Fraction for trigger validation
  auto hTriggersSoftwareVsTCM = mPostProcHelper.template getObject<TH2F>("TriggersSoftwareVsTCM");
  if (hTriggersSoftwareVsTCM) {
    std::unique_ptr<TH1D> projOnlyHWorSW(hTriggersSoftwareVsTCM->ProjectionX("projOnlyHWorSW", 2, 3));
    std::unique_ptr<TH1D> projValidatedSWandHW(hTriggersSoftwareVsTCM->ProjectionX("projValidatedSWandHW", 4, 4));
    projOnlyHWorSW->LabelsDeflate();
    projValidatedSWandHW->LabelsDeflate();
    mHistTrgValidation->Divide(projOnlyHWorSW.get(), projValidatedSWandHW.get());
  }
  decomposeHists();
  setTimestampToMOs();
  // Needed for first-iter init
  mIsFirstIter = false;
}

void PostProcTask::decomposeHists()
{
  for (const auto& histName : mVecHistsToDecompose) {
    auto insertedMap = mMapHistsToDecompose.insert({ histName, {} });
    auto& mapHists = insertedMap.first->second;

    auto histSrcPtr = mPostProcHelper.template getObject<TH2F>(histName);
    if (histSrcPtr == nullptr) {
      continue;
    }
    const auto bins = histSrcPtr->GetYaxis()->GetNbins();
    const auto binLow = histSrcPtr->GetYaxis()->GetXmin();
    const auto binUp = histSrcPtr->GetYaxis()->GetXmax();

    for (const auto& chID : mVecChannelIDs) {
      auto insertedHistDst = mapHists.insert({ chID, nullptr });
      auto& histDstPtr = insertedHistDst.first->second;
      auto isInserted = insertedHistDst.second;
      if (isInserted == true) {
        // creation in first iter
        const std::string suffix = std::string{ Form("%03i", chID) };
        const std::string newHistName = histName + std::string{ "_" } + suffix;
        const std::string newHistTitle = histSrcPtr->GetTitle() + std::string{ " " } + suffix;
        histDstPtr = std::make_shared<HistDecomposed_t>(newHistName.c_str(), newHistTitle.c_str(), bins, binLow, binUp);
        getObjectsManager()->startPublishing(histDstPtr.get());
      }
      histDstPtr->Reset();
      // making projection
      const auto binPos = chID + 1;
      const std::unique_ptr<TH1D> proj(histSrcPtr->ProjectionY("proj", binPos, binPos));
      histDstPtr->Add(proj.get());
    }
  }
}

void PostProcTask::setTimestampToMOs()
{
  for (int iObj = 0; iObj < getObjectsManager()->getNumberPublishedObjects(); iObj++) {
    auto mo = getObjectsManager()->getMonitorObject(iObj);
    mo->addOrUpdateMetadata(mPostProcHelper.mTimestampMetaField, std::to_string(mPostProcHelper.mTimestampAnchor));
  }
}

void PostProcTask::finalize(Trigger t, framework::ServiceRegistryRef)
{
}

} // namespace o2::quality_control_modules::ft0
