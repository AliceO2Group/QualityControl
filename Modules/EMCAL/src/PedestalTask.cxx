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
/// \file   PedestalTask.cxx
/// \author Markus Fasel
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>

#include "QualityControl/QcInfoLogger.h"
#include "EMCAL/PedestalTask.h"
#include <EMCALBase/Geometry.h>
#include <EMCALCalib/Pedestal.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/DataRefUtils.h>

namespace o2::quality_control_modules::emcal
{

PedestalTask::~PedestalTask()
{
  delete mPedestalChannelFECHG;
  delete mPedestalChannelFECLG;
  delete mPedestalChannelLEDMONHG;
  delete mPedestalChannelLEDMONLG;

  delete mPedestalPositionFECHG;
  delete mPedestalPositionFECLG;
  delete mPedestalPositionLEDMONHG;
  delete mPedestalPositionLEDMONLG;
}

void PedestalTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  const int NFECCHANNELS = 17664,
            NLEDMONCHANNELS = 480,
            NCOLSFEC = 96,
            NROWSFEC = 208,
            NCOLSLEDMON = 48,
            NROWSLEDMON = 10;
  mPedestalChannelFECHG = new TH1D("mPedestalChannelFECHG", "Pedestals (FEC, HG); tower ID; Pedestal (ADC)", NFECCHANNELS, -0.5, NFECCHANNELS - 0.5);
  mPedestalChannelFECLG = new TH1D("mPedestalChannelFECLG", "Pedestals (FEC, LG); tower ID; Pedestal (ADC)", NFECCHANNELS, -0.5, NFECCHANNELS - 0.5);
  mPedestalChannelLEDMONHG = new TH1D("mPedestalChannelLEDMONHG", "Pedestals (LEDMON, HG); tower ID; Pedestal (ADC)", NLEDMONCHANNELS, -0.5, NLEDMONCHANNELS - 0.5);
  mPedestalChannelLEDMONLG = new TH1D("mPedestalChannelLEDMONLG", "Pedestals (LEDMON, LG); tower ID; Pedestal (ADC)", NLEDMONCHANNELS, -0.5, NLEDMONCHANNELS - 0.5);

  mPedestalPositionFECHG = new TH2D("mPedestalPositionFECHG", "Pedestals (FEC, HG); column; row", NCOLSFEC, -0.5, NCOLSFEC - 0.5, NROWSFEC, -0.5, NROWSFEC - 0.5);
  mPedestalPositionFECLG = new TH2D("mPedestalPositionFECLG", "Pedestals (FEC, LG); column; row", NCOLSFEC, -0.5, NCOLSFEC - 0.5, NROWSFEC, -0.5, NROWSFEC - 0.5);
  mPedestalPositionLEDMONHG = new TH2D("mPedestalPositionLEDMONHG", "Pedestals (LEDMON, HG); column; row", NCOLSLEDMON, -0.5, NCOLSLEDMON - 0.5, NROWSLEDMON, -0.5, NROWSLEDMON - 0.5);
  mPedestalPositionLEDMONLG = new TH2D("mPedestalPositionLEDMONLG", "Pedestals (LEDMON, LG); column; row", NCOLSLEDMON, -0.5, NCOLSLEDMON - 0.5, NROWSLEDMON, -0.5, NROWSLEDMON - 0.5);

  getObjectsManager()->startPublishing(mPedestalChannelFECHG);
  getObjectsManager()->startPublishing(mPedestalChannelFECLG);
  getObjectsManager()->startPublishing(mPedestalChannelLEDMONHG);
  getObjectsManager()->startPublishing(mPedestalChannelLEDMONLG);
  getObjectsManager()->startPublishing(mPedestalPositionFECHG);
  getObjectsManager()->startPublishing(mPedestalPositionFECLG);
  getObjectsManager()->startPublishing(mPedestalPositionLEDMONHG);
  getObjectsManager()->startPublishing(mPedestalPositionLEDMONLG);

  mGeometry = o2::emcal::Geometry::GetInstanceFromRunNumber(300000);
}

void PedestalTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
}

void PedestalTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
  reset();
}

void PedestalTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  ILOG(Debug, Support) << "Start monitoring data" << ENDM;
  bool hasPedestalsCCDB = false;
  for (auto&& input : o2::framework::InputRecordWalker(ctx.inputs())) {
    // get message header
    if (input.header != nullptr && input.payload != nullptr) {
      auto payloadSize = o2::framework::DataRefUtils::getPayloadSize(input);
      const auto* header = o2::framework::DataRefUtils::getHeader<header::DataHeader*>(input);
      if (payloadSize) {
        if ((strcmp(header->dataOrigin.str, "CLP") == 0) && (strcmp(header->dataDescription.str, "EMC_PEDCALIB") == 0)) {
          ILOG(Info, Support) << "Pedestals for Pads found" << ENDM;
          hasPedestalsCCDB = true;
        }
      }
    }
  }

  if (hasPedestalsCCDB) {
    auto pedestals = o2::framework::DataRefUtils::as<o2::framework::CCDBSerialized<o2::emcal::Pedestal>>(ctx.inputs().get<o2::framework::DataRef>("peds"));
    if (pedestals) {
      mNumberObjectsFetched++;
      for (int ichan = 0; ichan < mPedestalChannelFECHG->GetXaxis()->GetNbins(); ichan++) {
        auto pedestalLow = pedestals->getPedestalValue(ichan, true, false),
             pedestalHigh = pedestals->getPedestalValue(ichan, false, false);
        mPedestalChannelFECHG->SetBinContent(ichan + 1, pedestalHigh);
        mPedestalChannelFECLG->SetBinContent(ichan + 1, pedestalLow);
        auto [row, col] = mGeometry->GlobalRowColFromIndex(ichan);
        mPedestalPositionFECHG->SetBinContent(col + 1, row + 1, pedestalHigh);
        mPedestalPositionFECLG->SetBinContent(col + 1, row + 1, pedestalLow);
      }
      for (auto ichan = 0; ichan < mPedestalChannelLEDMONHG->GetXaxis()->GetNbins(); ichan++) {
        auto pedestalLow = pedestals->getPedestalValue(ichan, true, true),
             pedestalHigh = pedestals->getPedestalValue(ichan, false, true);
        mPedestalChannelLEDMONHG->SetBinContent(ichan + 1, pedestalHigh);
        mPedestalChannelLEDMONLG->SetBinContent(ichan + 1, pedestalLow);
        int col = ichan % 48,
            row = ichan / 48;
        mPedestalPositionLEDMONHG->SetBinContent(col + 1, row + 1, pedestalHigh);
        mPedestalPositionLEDMONLG->SetBinContent(col + 1, row + 1, pedestalLow);
      }
    }
  }
  ILOG(Info, Support) << "Number of CCDB fetches of pedestal objects: " << mNumberObjectsFetched << ENDM;
}

void PedestalTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void PedestalTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void PedestalTask::reset()
{
  mPedestalChannelFECHG->Reset();
  mPedestalChannelFECLG->Reset();
  mPedestalChannelLEDMONHG->Reset();
  mPedestalChannelLEDMONLG->Reset();

  mPedestalPositionFECHG->Reset();
  mPedestalPositionFECLG->Reset();
  mPedestalPositionLEDMONHG->Reset();
  mPedestalPositionLEDMONLG->Reset();

  mNumberObjectsFetched = 0;
}

} // namespace o2::quality_control_modules::emcal
