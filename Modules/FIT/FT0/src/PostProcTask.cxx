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
  delete mAmpl;
  delete mTime;
}

void PostProcTask::configure(const boost::property_tree::ptree& config)
{
  const char* configPath = Form("qc.postprocessing.%s", getID().c_str());
  mPostProcHelper.configure(config, configPath);
}

void PostProcTask::initialize(Trigger trg, framework::ServiceRegistryRef services)
{
  mPostProcHelper.initialize(trg, services);

  mHistChDataNegBits = helper::registerHist<TH2F>(getObjectsManager(), "COLZ", "ChannelDataNegBits", "ChannelData negative bits per ChannelID;Channel;Negative bit", sNCHANNELS_PM, 0, sNCHANNELS_PM, mMapPMbits);
  mHistTriggers = helper::registerHist<TH1F>(getObjectsManager(), "", "Triggers", "FT0 Triggers from TCM", mMapTechTrgBits);
  mHistBcTrgOutOfBunchColl = helper::registerHist<TH2F>(getObjectsManager(), "COLZ", "OutOfBunchColl_BCvsTrg", "FT0 BC vs Triggers for out-of-bunch collisions;BC;Triggers", sBCperOrbit, 0, sBCperOrbit, mMapTechTrgBits);
  mHistBcPattern = helper::registerHist<TH2F>(getObjectsManager(), "COLZ", "bcPattern", "BC pattern", sBCperOrbit, 0, sBCperOrbit, mMapTechTrgBits);
  mHistTimeInWindow = helper::registerHist<TH1F>(getObjectsManager(), "", "TimeInWindowFraction", Form("Fraction of events with CFD in time gate(%i,%i) vs ChannelID;ChannelID;Event fraction with CFD in time gate", mLowTimeThreshold, mUpTimeThreshold), sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mHistCFDEff = helper::registerHist<TH1F>(getObjectsManager(), "", "CFD_efficiency", "FT0 Fraction of events with CFD in ADC gate vs ChannelID;ChannelID;Event fraction with CFD in ADC gate;", sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mHistChannelID_outOfBC = helper::registerHist<TH1F>(getObjectsManager(), "", "ChannelID_outOfBC", "FT0 ChannelID, out of bunch", sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mHistTrgValidation = helper::registerHist<TH1F>(getObjectsManager(), "", "TrgValidation", "FT0 SW + HW only to validated triggers fraction", mMapTrgBits);

  mAmpl = new TProfile("MeanAmplPerChannel", "mean ampl per channel;Channel;Ampl #mu #pm #sigma", o2::ft0::Constants::sNCHANNELS_PM, 0, o2::ft0::Constants::sNCHANNELS_PM);
  mTime = new TProfile("MeanTimePerChannel", "mean time per channel;Channel;Time #mu #pm #sigma", o2::ft0::Constants::sNCHANNELS_PM, 0, o2::ft0::Constants::sNCHANNELS_PM);

  getObjectsManager()->startPublishing(mAmpl);
  getObjectsManager()->startPublishing(mTime);
}

void PostProcTask::update(Trigger t, framework::ServiceRegistryRef)
{
  mPostProcHelper.setTrigger(t);
  mPostProcHelper.getMetadata();
  auto getBinContent2Ddiag = [](TH2* hist, const std::string& binName) {return hist->GetBinContent(hist->GetXaxis()->FindBin(binName.c_str()), hist->GetYaxis()->FindBin(binName.c_str()));};
  // Trigger correlation
  auto hTrgCorr = mPostProcHelper.template getObject<TH2F>("TriggersCorrelation");

  mHistTriggers->Reset();
  if (hTrgCorr) {
    double totalStat{ 0 };
    for (int iBin = 1; iBin < mHistTriggers->GetXaxis()->GetNbins() + 1; iBin++) {
      std::string binName{ mHistTriggers->GetXaxis()->GetBinLabel(iBin) };
      const auto binContent = getBinContent2Ddiag(hTrgCorr, binName);
      mHistTriggers->SetBinContent(iBin, getBinContent2Ddiag(hTrgCorr, binName));
      totalStat += binContent;
    }
    mHistTriggers->SetEntries(totalStat);
  }
  // PM bits
  auto hChDataBits = mPostProcHelper.template getObject<TH2F>("ChannelDataBits");
  auto hStatChannelID = mPostProcHelper.template getObject<TH1F>("StatChannelID");
  mHistChDataNegBits->Reset();
  if (hChDataBits && hStatChannelID) {
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

  // Amplitudes
  auto hAmpPerChannel = mPostProcHelper.template getObject<TH2F>("AmpPerChannel");
  if(hAmpPerChannel) {
    std::unique_ptr<TH1D> projNom(hAmpPerChannel->ProjectionX("projNom", hAmpPerChannel->GetYaxis()->FindBin(1.0), -1));
    std::unique_ptr<TH1D> projDen(hAmpPerChannel->ProjectionX("projDen"));
    mHistCFDEff->Divide(projNom.get(), projDen.get());
  }
  // Times
  auto hTimePerChannel = mPostProcHelper.template getObject<TH2F>("TimePerChannel");
  if(hTimePerChannel) {
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

  auto hBcVsTrg = mPostProcHelper.template getObject<TH2F>("BCvsTriggers");

  for (const auto& entry : mMapTrgHistBC) {
    hBcVsTrg->ProjectionX(entry.second->GetName(), entry.first + 1, entry.first + 1);
  }

  const auto &bcPattern = mPostProcHelper.getGRPLHCIFData().getBunchFilling();

  mHistBcPattern->Reset();
  for (int i = 0; i < sBCperOrbit + 1; i++) {
    for (int j = 0; j < mMapTechTrgBits.size() + 1; j++) {
      mHistBcPattern->SetBinContent(i + 1, j + 1, bcPattern.testBC(i));
    }
  }

  mHistBcTrgOutOfBunchColl->Reset();
  float vmax = hBcVsTrg->GetBinContent(hBcVsTrg->GetMaximumBin());
  mHistBcTrgOutOfBunchColl->Add(hBcVsTrg, mHistBcPattern.get(), 1, -1 * vmax);
  for (int i = 0; i < sBCperOrbit + 1; i++) {
    for (int j = 0; j < mMapTechTrgBits.size() + 1; j++) {
      if (mHistBcTrgOutOfBunchColl->GetBinContent(i + 1, j + 1) < 0) {
        mHistBcTrgOutOfBunchColl->SetBinContent(i + 1, j + 1, 0); // is it too slow?
      }
    }
  }
  mHistBcTrgOutOfBunchColl->SetEntries(mHistBcTrgOutOfBunchColl->Integral(1, sBCperOrbit, 1, mMapTechTrgBits.size()));
  for (int iBin = 1; iBin < mMapTechTrgBits.size() + 1; iBin++) {
    const std::string metadataKey = "BcVsTrgIntegralBin" + std::to_string(iBin);
    const std::string metadataValue = std::to_string(hBcVsTrg->Integral(1, sBCperOrbit, iBin, iBin));
    getObjectsManager()->getMonitorObject(mHistBcTrgOutOfBunchColl->GetName())->addOrUpdateMetadata(metadataKey, metadataValue);
    ILOG(Info, Support) << metadataKey << ":" << metadataValue << ENDM;
  }
  auto hChIDvsBC = mPostProcHelper.template getObject<TH2F>("ChannelIDperBC");
  if (hChIDvsBC) {
    auto hChID_InBC = std::make_unique<TH1D>("hChID_InBC", "hChID_InBC", hChIDvsBC->GetYaxis()->GetNbins(), 0, hChIDvsBC->GetYaxis()->GetNbins());
    auto hChID_OutOfBC = std::make_unique<TH1D>("hChID_OutOfBC", "hChID_OutOfBC", hChIDvsBC->GetYaxis()->GetNbins(), 0, hChIDvsBC->GetYaxis()->GetNbins());
    if (mAsynchChannelLogic == std::string{ "standard" }) {
    // Standard logic, calculates outCollBC/inCollBC ratio
      for (int iChID = 0; iChID < hChIDvsBC->GetYaxis()->GetNbins(); iChID++) {
        double cntInBC{ 0 };
        double cntOutOfBC{ 0 };
        for (int iBC = 0; iBC < hChIDvsBC->GetXaxis()->GetNbins(); iBC++) {
          if (bcPattern.testBC(iBC)) {
            cntInBC += hChIDvsBC->GetBinContent(iBC + 1, iChID + 1);
          } else {
            cntOutOfBC += hChIDvsBC->GetBinContent(iBC + 1, iChID + 1);
          }
        }
        hChID_InBC->Fill(iChID, cntInBC);
        hChID_OutOfBC->Fill(iChID, cntOutOfBC);
      }
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
        hChID_InBC->Fill(iChID, cntInBC);
        hChID_OutOfBC->Fill(iChID, cntOutOfBC);
      }
    }
    mHistChannelID_outOfBC->Divide(hChID_OutOfBC.get(), hChID_InBC.get());
  }

  auto hTriggersSoftwareVsTCM = mPostProcHelper.template getObject<TH2F>("TriggersSoftwareVsTCM");
  if (hTriggersSoftwareVsTCM) {
    std::unique_ptr<TH1D> projOnlyHWorSW(hTriggersSoftwareVsTCM->ProjectionX("projOnlyHWorSW", 1, 2));
    std::unique_ptr<TH1D> projValidatedSWandHW(hTriggersSoftwareVsTCM->ProjectionX("projValidatedSWandHW", 4, 4));
    projOnlyHWorSW->LabelsDeflate();
    projValidatedSWandHW->LabelsDeflate();
    mHistTrgValidation->Divide(projOnlyHWorSW.get(), projValidatedSWandHW.get());
  }
}

void PostProcTask::finalize(Trigger t, framework::ServiceRegistryRef)
{
}

} // namespace o2::quality_control_modules::ft0
