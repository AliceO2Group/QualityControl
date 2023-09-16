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
/// \file   LaserAgingFT0Task.cxx
/// \author My Name
///

#include <TCanvas.h>
#include <TH1.h>

#include "QualityControl/QcInfoLogger.h"
#include "FT0/LaserAgingFT0Task.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/DataRefUtils.h>
#include "DataFormatsFT0/LookUpTable.h"
#include "DataFormatsFT0/ChannelData.h"
#include <vector>

namespace o2::quality_control_modules::ft0
{

LaserAgingFT0Task::~LaserAgingFT0Task()
{
}

void LaserAgingFT0Task::initialize(o2::framework::InitContext& /*ctx*/)
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Debug, Devel) << "initialize LaserAgingFT0Task" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  ILOG(Debug, Support) << "Debug" << ENDM;                      // QcInfoLogger is used. FairMQ logs will go to there as well.
  ILOG(Info, Support) << "Info" << ENDM;                        // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - myOwnKey: " << param->second << ENDM;
  }

  mHistAmp2ADC0 = std::make_unique<TH2F>("AmpPerChannelADCzero", "Amplitude vs Channel;Channel;Amp", sNCHANNELS_PM, 0, sNCHANNELS_PM, 4200, -100, 4100);
  mHistAmp2ADC1 = std::make_unique<TH2F>("AmpPerChannelADCone", "Amplitude vs Channel;Channel;Amp", sNCHANNELS_PM, 0, sNCHANNELS_PM, 4200, -100, 4100);
  getObjectsManager()->startPublishing(mHistAmp2ADC0.get());
  getObjectsManager()->startPublishing(mHistAmp2ADC1.get());
  getObjectsManager()->setDefaultDrawOptions(mHistAmp2ADC0.get(), "COLZ");
  getObjectsManager()->setDefaultDrawOptions(mHistAmp2ADC1.get(), "COLZ");

  std::vector<unsigned int> vecAmpCut;
  std::vector<unsigned int> vecChannelIDs;
  std::vector<unsigned int> vecRefPMTChannelIDs;

  if (auto param = mCustomParameters.find("cutAmpl"); param != mCustomParameters.end()) {
    const auto ampCut = param->second;
    const std::string del = ",";
    vecAmpCut = parseParameters<unsigned int>(ampCut, del);
  } else {
    vecAmpCut.push_back(0);
  }
  if (auto param = mCustomParameters.find("ChannelIDs"); param != mCustomParameters.end()) {
    const auto chIDs = param->second;
    const std::string del = ",";
    vecChannelIDs = parseParameters<unsigned int>(chIDs, del);
  } else {
    for (unsigned int iCh = 0; iCh < o2::ft0::Constants::sNCHANNELS_PM; iCh++)
      vecChannelIDs.push_back(iCh);
  }
  if (auto param = mCustomParameters.find("RefPMTChannelIDs"); param != mCustomParameters.end()) {
    const auto chIDs = param->second;
    const std::string del = ",";
    vecRefPMTChannelIDs = parseParameters<unsigned int>(chIDs, del);
  }

  for (const auto& entry : vecAmpCut) {
    mSetAmpCut.insert(entry);
  }
  for (const auto& entry : vecChannelIDs) {
    mSetAllowedChIDs.insert(entry);
  }
  for (const auto& entry : vecRefPMTChannelIDs) {
    mSetRefPMTChIDs.insert(entry);
  }

  for (const auto& RefPMTChID : mSetRefPMTChIDs) {
    auto pairHistAmpVsBCADC0 = mMapHistAmpVsBCADC0.insert({ RefPMTChID, new TH2F(Form("ADCzero_Amp_vs_BC_channel%i", RefPMTChID), Form("Amplitude vs BC, channel %i;Amp;BC", RefPMTChID), 1000, 0, 1000, 1000, 0, 1000) });
    auto pairHistAmpVsBCADC1 = mMapHistAmpVsBCADC1.insert({ RefPMTChID, new TH2F(Form("ADCone_Amp_vs_BC_channel%i", RefPMTChID), Form("Amplitude vs BC, channel %i;Amp;BC", RefPMTChID), 1000, 0, 1000, 1000, 0, 1000) });
    if (pairHistAmpVsBCADC0.second) {
      getObjectsManager()->startPublishing(pairHistAmpVsBCADC0.first->second);
    }
    if (pairHistAmpVsBCADC1.second) {
      getObjectsManager()->startPublishing(pairHistAmpVsBCADC1.first->second);
    }
  }
}

void LaserAgingFT0Task::startOfActivity(const Activity& activity)
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
  mHistAmp2ADC0->Reset();
  mHistAmp2ADC1->Reset();
  for (auto& entry : mMapHistAmpVsBCADC0)
    entry.second->Reset();
  for (auto& entry : mMapHistAmpVsBCADC1)
    entry.second->Reset();
}

void LaserAgingFT0Task::startOfCycle()
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void LaserAgingFT0Task::monitorData(o2::framework::ProcessingContext& ctx)
{
  unsigned int costumAmplCut = 0;
  for (const auto& cutAmpl : mSetAmpCut) {
    costumAmplCut = cutAmpl;
  }

  auto channels = ctx.inputs().get<gsl::span<o2::ft0::ChannelData>>("channels");
  auto digits = ctx.inputs().get<gsl::span<o2::ft0::Digit>>("digits");
  for (auto& digit : digits) {
    const auto& vecChData = digit.getBunchChannelData(channels);
    for (const auto& chData : vecChData) {
      if (chData.getFlag(o2::ft0::ChannelData::kNumberADC))
        mHistAmp2ADC1->Fill(static_cast<Double_t>(chData.ChId), static_cast<Double_t>(chData.QTCAmpl));
      else
        mHistAmp2ADC0->Fill(static_cast<Double_t>(chData.ChId), static_cast<Double_t>(chData.QTCAmpl));
      if (mSetRefPMTChIDs.find(static_cast<unsigned int>(chData.ChId)) != mSetRefPMTChIDs.end()) {
        if (chData.QTCAmpl > costumAmplCut) {
          if (chData.getFlag(o2::ft0::ChannelData::kNumberADC)) {
            mMapHistAmpVsBCADC1[chData.ChId]->Fill(chData.QTCAmpl, digit.getIntRecord().bc);
          } else {
            mMapHistAmpVsBCADC0[chData.ChId]->Fill(chData.QTCAmpl, digit.getIntRecord().bc);
          }
        }
      }
    }
  }
}

void LaserAgingFT0Task::endOfCycle()
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void LaserAgingFT0Task::endOfActivity(const Activity& /*activity*/)
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void LaserAgingFT0Task::reset()
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.

  // clean all the monitor objects here

  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  mHistAmp2ADC0->Reset();
  mHistAmp2ADC1->Reset();
  for (auto& entry : mMapHistAmpVsBCADC0) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistAmpVsBCADC1) {
    entry.second->Reset();
  }
}

} // namespace o2::quality_control_modules::ft0
