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
/// \file   PedestalCalibTask.cxx
/// \author My Name
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>

#include "FOCALCalib/PadPedestal.h"

#include "QualityControl/QcInfoLogger.h"
#include "FOCAL/PedestalCalibTask.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/DataRefUtils.h>

namespace o2::quality_control_modules::focal
{

PedestalCalibTask::PedestalCalibTask()
{
  std::fill(mPedestalChannel.begin(), mPedestalChannel.end(), nullptr);
  std::fill(mPedestalPosition.begin(), mPedestalPosition.end(), nullptr);
}

PedestalCalibTask::~PedestalCalibTask()
{
  for (auto& hist : mPedestalChannel) {
    delete hist;
  }
  for (auto& hist : mPedestalPosition) {
    delete hist;
  }
}

void PedestalCalibTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  for (auto layer = 0; layer < mPedestalChannel.size(); layer++) {
    mPedestalChannel[layer] = new TH1D(Form("mPedestalChannelLayer%d", layer), Form("Pedestals in layer %d", layer), o2::focal::constants::PADLAYER_MODULE_NCHANNELS, -0.5, o2::focal::constants::PADLAYER_MODULE_NCHANNELS - 0.5);
    mPedestalChannel[layer]->SetXTitle("Channel ID");
    mPedestalChannel[layer]->SetYTitle("Pedestal (ADC counts)");
    getObjectsManager()->startPublishing(mPedestalChannel[layer]);
    mPedestalPosition[layer] = new TH2D(Form("mPedestalPositionLayer%d", layer), Form("Pedestals in layer %d", layer), o2::focal::PadMapper::NCOLUMN, -0.5, o2::focal::PadMapper::NCOLUMN - 0.5, o2::focal::PadMapper::NROW, -0.5, o2::focal::PadMapper::NROW - 0.5);
    mPedestalPosition[layer]->SetXTitle("Column");
    mPedestalPosition[layer]->SetYTitle("Row");
    getObjectsManager()->startPublishing(mPedestalPosition[layer]);
  }
}

void PedestalCalibTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
  reset();
}

void PedestalCalibTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void PedestalCalibTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  ILOG(Debug, Support) << "Start monitoring data" << ENDM;
  bool hasPedestalsCCDB = false;
  for (auto&& input : o2::framework::InputRecordWalker(ctx.inputs())) {
    // get message header
    if (input.header != nullptr && input.payload != nullptr) {
      auto payloadSize = o2::framework::DataRefUtils::getPayloadSize(input);
      const auto* header = o2::framework::DataRefUtils::getHeader<header::DataHeader*>(input);
      if (payloadSize) {
        if ((strcmp(header->dataOrigin.str, "CLP") == 0) && (strcmp(header->dataDescription.str, "FOC_PADPEDESTALSCLP") == 0)) {
          ILOG(Info, Support) << "Pedestals for Pads found" << ENDM;
          hasPedestalsCCDB = true;
        }
      }
    }
  }

  if (hasPedestalsCCDB) {
    auto pedestals = o2::framework::DataRefUtils::as<o2::framework::CCDBSerialized<o2::focal::PadPedestal>>(ctx.inputs().get<o2::framework::DataRef>("peds"));
    if (pedestals) {
      mNumberObjectsFetched++;
      LOG(info) << "PedestalCalibTask::monitorData() : Extracted FOCAL Pad Pedestals";
      for (auto layer = 0; layer < mPedestalChannel.size(); layer++) {
        mPedestalChannel[layer]->Reset();
        mPedestalPosition[layer]->Reset();
        for (auto chan = 0; chan < o2::focal::constants::PADLAYER_MODULE_NCHANNELS; chan++) {
          try {
            auto channelPedestal = pedestals->getPedestal(layer, chan);
            mPedestalChannel[layer]->SetBinContent(chan + 1, channelPedestal);
            auto [col, row] = mPadMapper.getRowColFromChannelID(chan);
            mPedestalPosition[layer]->SetBinContent(col + 1, row + 1, channelPedestal);
          } catch (o2::focal::PadPedestal::InvalidChannelException& e) {
            ILOG(Error, Support) << "Error in pedestal access: " << e.what() << ENDM;
          }
        }
      }
    }
    ILOG(Info, Support) << "Number of CCDB fetches of pedestal objects: " << mNumberObjectsFetched << ENDM;
  }
}

void PedestalCalibTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void PedestalCalibTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void PedestalCalibTask::reset()
{
  for (auto& hist : mPedestalChannel) {
    hist->Reset();
  }
  for (auto& hist : mPedestalPosition) {
    hist->Reset();
  }
}

} // namespace o2::quality_control_modules::focal
