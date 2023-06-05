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
/// \file   DecodingTask.cxx
/// \author Sebastien Perrin
///

#include "MCH/DecodingTask.h"
#include "DetectorsRaw/RDHUtils.h"
#include "QualityControl/QcInfoLogger.h"
#include "Framework/WorkflowSpec.h"
#include "Framework/DataRefUtils.h"

#include "MCHRawElecMap/Mapper.h"
#include "MCHRawDecoder/PageDecoder.h"
#include "MCHRawDecoder/ErrorCodes.h"
#include "MCHBase/DecoderError.h"
#include "MCHBase/HeartBeatPacket.h"

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

using namespace o2;
using namespace o2::framework;
using namespace o2::mch;
using namespace o2::mch::raw;
using RDH = o2::header::RDHAny;

//_____________________________________________________________________________

void DecodingTask::createErrorHistos()
{
  const uint32_t nElecXbins = NumberOfDualSampas;

  // Number of decoding errors, grouped by chamber ID and normalized to the number of processed TF
  mHistogramErrorsFEC = std::make_unique<MergeableTH2Ratio>("DecodingErrors_Elec", "Error Code vs. FEC ID", nElecXbins, 0, nElecXbins, getErrorCodesSize(), 0, getErrorCodesSize());
  {
    TAxis* ax = mHistogramErrorsFEC->GetYaxis();
    for (int i = 0; i < getErrorCodesSize(); i++) {
      ax->SetBinLabel(i + 1, errorCodeAsString(1 << i).c_str());
      ax->ChangeLabel(i + 1, 45);
    }
  }
  publishObject(mHistogramErrorsFEC.get(), "colz", "gridy logz", false, false);
}

//_____________________________________________________________________________

void DecodingTask::createHeartBeatHistos()
{
  const uint32_t nElecXbins = NumberOfDualSampas;

  // Heart-beat packets time distribution and synchronization errors
  mHistogramHBTimeFEC = std::make_unique<MergeableTH2Ratio>("HBTime_Elec", "HB time vs. FEC ID", nElecXbins, 0, nElecXbins, 40, mHBExpectedBc - 20, mHBExpectedBc + 20);
  publishObject(mHistogramHBTimeFEC.get(), "colz", "logz", false, false);

  uint64_t max = ((static_cast<uint64_t>(0x100000) / 100) + 1) * 100;
  mHistogramHBCoarseTimeFEC = std::make_unique<MergeableTH2Ratio>("HBCoarseTime_Elec", "HB time vs. FEC ID (coarse)", nElecXbins, 0, nElecXbins, 100, 0, max);
  publishObject(mHistogramHBCoarseTimeFEC.get(), "colz", "", false, false);

  mSyncStatusFEC = std::make_unique<MergeableTH2Ratio>("SyncStatus_Elec", "Heart-beat status vs. FEC ID", nElecXbins, 0, nElecXbins, 3, 0, 3);
  mSyncStatusFEC->GetYaxis()->SetBinLabel(1, "OK");
  mSyncStatusFEC->GetYaxis()->SetBinLabel(2, "Out-of-sync");
  mSyncStatusFEC->GetYaxis()->SetBinLabel(3, "Missing");
  publishObject(mSyncStatusFEC.get(), "colz", "gridy", false, false);
}

//_____________________________________________________________________________

void DecodingTask::initialize(o2::framework::InitContext& /*ic*/)
{
  ILOG(Debug, Devel) << "initialize DecodingErrorsTask" << ENDM;

  // expected bunch-crossing value in heart-beat packets
  if (auto param = mCustomParameters.find("HBExpectedBc"); param != mCustomParameters.end()) {
    mHBExpectedBc = std::stoi(param->second);
  }

  mElec2DetMapper = createElec2DetMapper<ElectronicMapperGenerated>();

  mHistogramTimeFramesCount = std::make_unique<TH1F>("TimeFramesCount", "Number of Time Frames", 1, 0, 1);
  publishObject(mHistogramTimeFramesCount.get(), "hist", "", true, false);

  createErrorHistos();
  createHeartBeatHistos();
}

//_____________________________________________________________________________

void DecodingTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity : " << activity.mId << ENDM;
}

//_____________________________________________________________________________

void DecodingTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

//_____________________________________________________________________________

void DecodingTask::decodeTF(framework::ProcessingContext& pc)
{
  // get the input buffer
  auto& inputs = pc.inputs();
  DPLRawParser parser(inputs, o2::framework::select(""));

  for (auto it = parser.begin(), end = parser.end(); it != end; ++it) {
    auto const* raw = it.raw();
    if (!raw) {
      continue;
    }
    size_t payloadSize = it.size();

    gsl::span<const std::byte> buffer(reinterpret_cast<const std::byte*>(raw), sizeof(RDH) + payloadSize);
    decodeBuffer(buffer);
  }
}

//_____________________________________________________________________________

void DecodingTask::decodeReadout(const o2::framework::DataRef& input)
{
  // get the input buffer
  if (input.spec->binding != "readout") {
    return;
  }

  const auto* header = o2::framework::DataRefUtils::getHeader<header::DataHeader*>(input);
  if (!header) {
    return;
  }

  size_t payloadSize = o2::framework::DataRefUtils::getPayloadSize(input);
  if (payloadSize == 0) {
    return;
  }

  auto const* raw = input.payload;

  gsl::span<const std::byte> buffer(reinterpret_cast<const std::byte*>(raw), payloadSize);
  decodeBuffer(buffer);
}

//_____________________________________________________________________________

void DecodingTask::decodeBuffer(gsl::span<const std::byte> buf)
{
  // RDH source ID for the MCH system
  static constexpr int sMCHSourceId = 10;

  size_t bufSize = buf.size();
  size_t pageStart = 0;
  while (bufSize > pageStart) {
    RDH* rdh = reinterpret_cast<RDH*>(const_cast<std::byte*>(&(buf[pageStart])));
    auto rdhHeaderSize = o2::raw::RDHUtils::getHeaderSize(rdh);
    if (rdhHeaderSize != 64) {
      break;
    }
    auto pageSize = o2::raw::RDHUtils::getOffsetToNext(rdh);

    // skip all buffers that do not belong to MCH
    auto sourceId = o2::raw::RDHUtils::getSourceID(rdh);
    if (sourceId != sMCHSourceId) {
      continue;
    }

    gsl::span<const std::byte> page(reinterpret_cast<const std::byte*>(rdh), pageSize);
    decodePage(page);

    pageStart += pageSize;
  }
}

//_____________________________________________________________________________

void DecodingTask::decodePage(gsl::span<const std::byte> page)
{
  auto errorHandler = [&](DsElecId dsElecId, int8_t chip, uint32_t error) {
    int feeId{ -1 };
    uint32_t solarId = dsElecId.solarId();
    uint32_t dsAddr = dsElecId.elinkId();

    plotError(solarId, dsAddr, chip, error);
  };

  if (!mDecoder) {
    o2::mch::raw::DecodedDataHandlers handlers;
    handlers.sampaErrorHandler = errorHandler;
    mDecoder = o2::mch::raw::createPageDecoder(page, handlers);
  }
  mDecoder(page);
}

//_____________________________________________________________________________

void DecodingTask::processErrors(framework::ProcessingContext& pc)
{
  auto rawerrors = pc.inputs().get<gsl::span<o2::mch::DecoderError>>("rawerrors");
  for (auto& error : rawerrors) {
    plotError(error.getSolarID(), error.getDsID(), error.getChip(), error.getError());
  }
}

//_____________________________________________________________________________

void DecodingTask::plotError(uint16_t solarId, int dsAddr, int chip, uint32_t error)
{
  int fecId = -1;
  o2::mch::raw::DsElecId dsElecId{ solarId, static_cast<uint8_t>(dsAddr / 5), static_cast<uint8_t>(dsAddr % 5) };
  if (auto opt = mElec2DetMapper(dsElecId); opt.has_value()) {
    o2::mch::raw::DsDetId dsDetId = opt.value();
    fecId = getDsIndex(dsDetId);
  }
  if (fecId < 0) {
    return;
  }

  uint32_t errMask = 1;
  for (int i = 0; i < getErrorCodesSize(); i++) {
    // Fill the error histogram if the i-th bin is set in the error word
    if ((error & errMask) != 0) {
      mHistogramErrorsFEC->getNum()->Fill(fecId, 0.5 + i);
    }
    errMask <<= 1;
  }
}

//_____________________________________________________________________________

void DecodingTask::processHBPackets(framework::ProcessingContext& pc)
{
  std::fill(mHBcount.begin(), mHBcount.end(), HBCount());
  auto hbpackets = pc.inputs().get<gsl::span<o2::mch::HeartBeatPacket>>("hbpackets");
  for (auto& hbp : hbpackets) {
    plotHBPacket(hbp.getSolarID(), hbp.getDsID(), hbp.getChip(), hbp.getBunchCrossing());
  }
  updateSyncErrors();
}

//_____________________________________________________________________________

void DecodingTask::plotHBPacket(uint16_t solarId, int dsAddr, int chip, uint32_t bc)
{
  int mBcMin{ mHBExpectedBc - 2 };
  int mBcMax{ mHBExpectedBc + 2 };

  int fecId = -1;
  o2::mch::raw::DsElecId dsElecId{ solarId, static_cast<uint8_t>(dsAddr / 5), static_cast<uint8_t>(dsAddr % 5) };
  if (auto opt = mElec2DetMapper(dsElecId); opt.has_value()) {
    o2::mch::raw::DsDetId dsDetId = opt.value();
    fecId = getDsIndex(dsDetId);
  }
  if (fecId < 0) {
    return;
  }

  mHistogramHBCoarseTimeFEC->getNum()->Fill(fecId, bc);
  if (bc < mHistogramHBTimeFEC->getNum()->GetYaxis()->GetXmin()) {
    bc = mHistogramHBTimeFEC->getNum()->GetYaxis()->GetXmin();
  }
  if (bc > mHistogramHBTimeFEC->getNum()->GetYaxis()->GetXmax()) {
    bc = mHistogramHBTimeFEC->getNum()->GetYaxis()->GetXmax();
  }
  mHistogramHBTimeFEC->getNum()->Fill(fecId, bc);

  if (bc >= mBcMin && bc <= mBcMax) {
    mHBcount[fecId].nSync += 1;
  } else {
    mHBcount[fecId].nOutOfSync += 1;
  }
}

//_____________________________________________________________________________

void DecodingTask::updateSyncErrors()
{
  for (size_t fecId = 0; fecId < mHBcount.size(); fecId++) {
    bool isOutOfSync = false;
    if (mHBcount[fecId].nOutOfSync > 0) {
      isOutOfSync = true;
    }

    bool isMissing = false;
    if (mHBcount[fecId].nSync < 1.5) {
      isMissing = true;
    }

    if (!isOutOfSync && !isMissing) {
      mSyncStatusFEC->getNum()->Fill(0.5 + fecId, 0);
    }
    if (isOutOfSync) {
      mSyncStatusFEC->getNum()->Fill(0.5 + fecId, 1);
    }
    if (isMissing) {
      mSyncStatusFEC->getNum()->Fill(0.5 + fecId, 2);
    }
  }
}

//_____________________________________________________________________________

void DecodingTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  static int nTF = 1;
  for (auto&& input : ctx.inputs()) {
    if (input.spec->binding == "readout") {
      decodeReadout(input);
    }
    if (input.spec->binding == "TF") {
      decodeTF(ctx);
    }
    if (input.spec->binding == "rawerrors") {
      processErrors(ctx);
    }
    if (input.spec->binding == "hbpackets") {
      processHBPackets(ctx);
    }
  }

  // Count the number of processed TF and set the denominators of the error histograms accordingly
  nTF += 1;

  mHistogramTimeFramesCount->Fill(0.5);

  auto updateTFcount = [](MergeableTH2Ratio* hr, int nTF) {
    auto hTF = hr->getDen();
    for (int ybin = 1; ybin <= hTF->GetYaxis()->GetNbins(); ybin++) {
      for (int xbin = 1; xbin <= hTF->GetXaxis()->GetNbins(); xbin++) {
        hTF->SetBinContent(xbin, ybin, nTF);
      }
    }
  };

  updateTFcount(mHistogramErrorsFEC.get(), nTF);
  updateTFcount(mHistogramHBTimeFEC.get(), nTF);
  updateTFcount(mHistogramHBCoarseTimeFEC.get(), nTF);
  updateTFcount(mSyncStatusFEC.get(), nTF);
}

//_____________________________________________________________________________

void DecodingTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;

  mHistogramErrorsFEC->update();
  mHistogramHBCoarseTimeFEC->update();
  mHistogramHBTimeFEC->update();
  mSyncStatusFEC->update();
}

//_____________________________________________________________________________

void DecodingTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

//_____________________________________________________________________________

void DecodingTask::reset()
{
  // clean all the monitor objects here
  ILOG(Info, Devel) << "Resetting the histograms" << ENDM;

  for (auto h : mAllHistograms) {
    h->Reset("ICES");
  }
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
