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
/// \file   TestbeamRawTask.cxx
/// \author My Name
///

#include <algorithm>
#include <iostream>

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TProfile2D.h>

#include "QualityControl/QcInfoLogger.h"
#include "FOCAL/TestbeamRawTask.h"
#include "FOCAL/PadWord.h"
#include <CommonConstants/Triggers.h>
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/DataRefUtils.h>
#include <DPLUtils/DPLRawParser.h>
#include <DetectorsRaw/RDHUtils.h>
#include <Headers/DataHeader.h>
#include <Headers/RDHAny.h>

namespace o2::quality_control_modules::focal
{

TestbeamRawTask::~TestbeamRawTask()
{
  for (auto& hist : mPadASICChannelADC) {
    delete hist;
  }
  for (auto& hist : mPadASICChannelTOA) {
    delete hist;
  }
  for (auto& hist : mPadASICChannelTOT) {
    delete hist;
  }
}

void TestbeamRawTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  mPadDecoder.setTriggerWinDur(20); // MF @TODO make taskParameter

  /////////////////////////////////////////////////////////////////
  /// PAD histograms
  /////////////////////////////////////////////////////////////////

  constexpr int PAD_CHANNELS = 76;
  constexpr int RANGE_ADC = 1024;
  constexpr int RANGE_TOA = 1024;
  constexpr int RANGE_TOT = 4096;

  for (int iasic = 0; iasic < PadData::NASICS; iasic++) {
    mPadASICChannelADC[iasic] = new TH2D(Form("PadADC_ASIC_%d", iasic), Form("ADC vs. channel ID for ASIC %d; channel ID; ADC", iasic), PAD_CHANNELS, -0.5, PAD_CHANNELS - 0.5, RANGE_ADC, 0., RANGE_ADC);
    mPadASICChannelADC[iasic]->SetStats(false);
    mPadASICChannelTOA[iasic] = new TH2D(Form("PadTOA_ASIC_%d", iasic), Form("TOA vs. channel ID for ASIC %d; channel ID; TOA", iasic), PAD_CHANNELS, -0.5, PAD_CHANNELS - 0.5, RANGE_TOA, 0., RANGE_TOA);
    mPadASICChannelTOA[iasic]->SetStats(false);
    mPadASICChannelTOT[iasic] = new TH2D(Form("PadTOT_ASIC_%d", iasic), Form("TOT vs. channel ID for ASIC %d; channel ID; TOT", iasic), PAD_CHANNELS, -0.5, PAD_CHANNELS - 0.5, RANGE_TOT / 4, 0., RANGE_TOT);
    mPadASICChannelTOT[iasic]->SetStats(false);
    mHitMapPadASIC[iasic] = new TProfile2D(Form("HitmapPadASIC_%d", iasic), Form("Hitmap for ASIC %d; col; row", iasic), PadMapper::NCOLUMN, -0.5, PadMapper::NCOLUMN - 0.5, PadMapper::NROW, -0.5, PadMapper::NROW - 0.5);
    mHitMapPadASIC[iasic]->SetStats(false);
    getObjectsManager()->startPublishing(mPadASICChannelADC[iasic]);
    getObjectsManager()->startPublishing(mPadASICChannelTOA[iasic]);
    getObjectsManager()->startPublishing(mPadASICChannelTOT[iasic]);
    getObjectsManager()->startPublishing(mHitMapPadASIC[iasic]);
  }
  mPayloadSizePadsGBT = new TH1D("PayloadSizePadGBT", "Payload size GBT words", 10000, 0., 10000.);
  getObjectsManager()->startPublishing(mPayloadSizePadsGBT);
}

void TestbeamRawTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity " << activity.mId << ENDM;
  reset();
}

void TestbeamRawTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void TestbeamRawTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  ILOG(Info, Support) << "Called" << ENDM;
  ILOG(Info, Support) << "Received " << ctx.inputs().size() << " inputs" << ENDM;
  int inputs = 0;
  std::vector<char> rawbuffer;
  int currentendpoint = 0;
  for (const auto& rawData : framework::InputRecordWalker(ctx.inputs())) {
    if (rawData.header != nullptr && rawData.payload != nullptr) {
      const auto payloadSize = o2::framework::DataRefUtils::getPayloadSize(rawData);
      auto header = o2::framework::DataRefUtils::getHeader<o2::header::DataHeader*>(rawData);
      ILOG(Debug, Support) << "Channel " << header->dataOrigin.str << "/" << header->dataDescription.str << "/" << header->subSpecification << ENDM;

      gsl::span<const char> databuffer(rawData.payload, payloadSize);
      int currentpos = 0;
      while (currentpos < databuffer.size()) {
        auto rdh = reinterpret_cast<const o2::header::RDHAny*>(databuffer.data() + currentpos);
        o2::raw::RDHUtils::printRDH(rdh);
        if (o2::raw::RDHUtils::getMemorySize(rdh) == o2::raw::RDHUtils::getHeaderSize(rdh)) {
          auto trigger = o2::raw::RDHUtils::getTriggerType(rdh);
          if (trigger & o2::trigger::SOT || trigger & o2::trigger::HB) {
            if (o2::raw::RDHUtils::getStop(rdh)) {
              ILOG(Debug, Support) << "Stop bit received - processing payload" << ENDM;
              // Data ready
              if (currentendpoint == 1) {
                // Pad data
                ILOG(Debug, Support) << "Processing PAD data" << ENDM;
                auto payloadsizeGBT = rawbuffer.size() * sizeof(char) / sizeof(PadGBTWord);
                mPayloadSizePadsGBT->Fill(payloadsizeGBT);
                processPadPayload(gsl::span<const PadGBTWord>(reinterpret_cast<const PadGBTWord*>(rawbuffer.data()), payloadsizeGBT));
                rawbuffer.clear();
              } else {
                ILOG(Error, Support) << "Unsupported endpoint " << currentendpoint << ENDM;
              }
            } else {
              ILOG(Debug, Support) << "New HBF or Timeframe" << ENDM;
              currentendpoint = o2::raw::RDHUtils::getEndPointID(rdh);
              ILOG(Debug, Support) << "Using endpoint " << currentendpoint;
              rawbuffer.clear();
            }
          }
          currentpos += o2::raw::RDHUtils::getOffsetToNext(rdh);
          continue;
        }

        // non-0 payload size:
        auto payloadsize = o2::raw::RDHUtils::getMemorySize(rdh) - o2::raw::RDHUtils::getHeaderSize(rdh);
        int endpoint = static_cast<int>(o2::raw::RDHUtils::getEndPointID(rdh));
        ILOG(Debug, Support) << "Next RDH: " << ENDM;
        ILOG(Debug, Support) << "Found endpoint              " << endpoint << ENDM;
        ILOG(Debug, Support) << "Found trigger BC:           " << o2::raw::RDHUtils::getTriggerBC(rdh) << ENDM;
        ILOG(Debug, Support) << "Found trigger Oribt:        " << o2::raw::RDHUtils::getTriggerOrbit(rdh) << ENDM;
        ILOG(Debug, Support) << "Found payload size:         " << payloadsize << ENDM;
        ILOG(Debug, Support) << "Found offset to next:       " << o2::raw::RDHUtils::getOffsetToNext(rdh) << ENDM;
        ILOG(Debug, Support) << "Stop bit:                   " << (o2::raw::RDHUtils::getStop(rdh) ? "yes" : "no") << ENDM;
        ILOG(Debug, Support) << "Number of GBT words:        " << (payloadsize * sizeof(char) / sizeof(PadGBTWord)) << ENDM;
        auto page_payload = databuffer.subspan(currentpos + o2::raw::RDHUtils::getHeaderSize(rdh), payloadsize);
        std::copy(page_payload.begin(), page_payload.end(), std::back_inserter(rawbuffer));
        currentpos += o2::raw::RDHUtils::getOffsetToNext(rdh);
      }
    } else {
      ILOG(Error, Support) << "Input " << inputs << ": Either header or payload is nullptr" << ENDM;
    }
    inputs++;
  }
}

void TestbeamRawTask::processPadPayload(gsl::span<const PadGBTWord> padpayload)
{
  processPadEvent(padpayload);

  constexpr std::size_t EVENTSIZEPADGBT = 1180;
  int nevents = padpayload.size() / EVENTSIZEPADGBT;
  for (int iev = 0; iev < nevents; iev++) {
    processPadEvent(padpayload.subspan(iev * EVENTSIZEPADGBT, EVENTSIZEPADGBT));
  }
}

void TestbeamRawTask::processPadEvent(gsl::span<const PadGBTWord> padpayload)
{
  mPadDecoder.reset();
  mPadDecoder.decodeEvent(padpayload);
  const auto& eventdata = mPadDecoder.getData();
  for (int iasic = 0; iasic < PadData::NASICS; iasic++) {
    const auto& asic = eventdata[iasic].getASIC();
    ILOG(Debug, Support) << "ASIC " << iasic << ", Header 0: " << asic.getFirstHeader() << ENDM;
    ILOG(Debug, Support) << "ASIC " << iasic << ", Header 1: " << asic.getSecondHeader() << ENDM;
    int currentchannel = 0;
    for (const auto& chan : asic.getChannels()) {
      mPadASICChannelADC[iasic]->Fill(currentchannel, chan.getADC());
      mPadASICChannelTOA[iasic]->Fill(currentchannel, chan.getTOA());
      mPadASICChannelTOT[iasic]->Fill(currentchannel, chan.getTOT());
      auto [column, row] = mPadMapper.getRowColFromChannelID(currentchannel);
      mHitMapPadASIC[iasic]->Fill(column, row, chan.getADC());
      currentchannel++;
    }
    // Fill CMN channels
    mPadASICChannelADC[iasic]->Fill(currentchannel, asic.getFirstCMN().getADC());
    mPadASICChannelTOA[iasic]->Fill(currentchannel, asic.getFirstCMN().getTOA());
    mPadASICChannelTOT[iasic]->Fill(currentchannel, asic.getFirstCMN().getTOT());
    currentchannel++;
    mPadASICChannelADC[iasic]->Fill(currentchannel, asic.getSecondCMN().getADC());
    mPadASICChannelTOA[iasic]->Fill(currentchannel, asic.getSecondCMN().getTOA());
    mPadASICChannelTOT[iasic]->Fill(currentchannel, asic.getSecondCMN().getTOT());
    currentchannel++;
    // Fill Calib channels
    mPadASICChannelADC[iasic]->Fill(currentchannel, asic.getFirstCalib().getADC());
    mPadASICChannelTOA[iasic]->Fill(currentchannel, asic.getFirstCalib().getTOA());
    mPadASICChannelTOT[iasic]->Fill(currentchannel, asic.getFirstCalib().getTOT());
    currentchannel++;
    mPadASICChannelADC[iasic]->Fill(currentchannel, asic.getSecondCalib().getADC());
    mPadASICChannelTOA[iasic]->Fill(currentchannel, asic.getSecondCalib().getTOA());
    mPadASICChannelTOT[iasic]->Fill(currentchannel, asic.getSecondCalib().getTOT());
    currentchannel++;
  }
}

void TestbeamRawTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void TestbeamRawTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void TestbeamRawTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  mPayloadSizePadsGBT->Reset();
  for (auto padadc : mPadASICChannelADC) {
    padadc->Reset();
  }
  for (auto padtoa : mPadASICChannelTOA) {
    padtoa->Reset();
  }
  for (auto padtot : mPadASICChannelTOT) {
    padtot->Reset();
  }
  for (auto hitmap : mHitMapPadASIC) {
    hitmap->Reset();
  }
}

} // namespace o2::quality_control_modules::focal
