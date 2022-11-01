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
/// \file   DecodingErrorsTask.cxx
/// \author Sebastien Perrin
///

#include "MCH/DecodingErrorsTask.h"
#include "MCH/GlobalHistogram.h"

#include <TFile.h>

#include "DetectorsRaw/RDHUtils.h"
#include "QualityControl/QcInfoLogger.h"
#include "Framework/WorkflowSpec.h"
#include "Framework/DataRefUtils.h"

#include "MCHRawElecMap/Mapper.h"
#include "MCHMappingInterface/Segmentation.h"
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
using namespace o2::mch::mapping;
using namespace o2::mch::raw;
using RDH = o2::header::RDHAny;

DecodingErrorsTask::DecodingErrorsTask()
{
}

DecodingErrorsTask::~DecodingErrorsTask() = default;

static void setXAxisLabels(TH2F* hErrors)
{
  TAxis* ax = hErrors->GetXaxis();
  for (int i = 0; i < getErrorCodesSize(); i++) {
    ax->SetBinLabel(i + 1, errorCodeAsString(1 << i).c_str());
    ax->ChangeLabel(i + 1, 45);
  }
}

static void setYAxisLabels(TH2F* hErrors)
{
  TAxis* ay = hErrors->GetYaxis();
  for (int i = 1; i <= 10; i++) {
    auto label = fmt::format("CH{}", i);
    ay->SetBinLabel(i, label.c_str());
  }
}

//_____________________________________________________________________________

void DecodingErrorsTask::createErrorHistos()
{
  // Number of decoding errors, grouped by chamber ID and normalized to the number of processed TF
  mHistogramErrorsPerChamber = std::make_shared<MergeableTH2Ratio>("DecodingErrorsPerChamber", "Chamber Number vs. Error Type", getErrorCodesSize(), 0, getErrorCodesSize(), 10, 1, 11);
  setXAxisLabels(mHistogramErrorsPerChamber.get());
  setYAxisLabels(mHistogramErrorsPerChamber.get());
  publishObject(mHistogramErrorsPerChamber, "colz", false, false);

  mHistogramErrorsPerChamberPrevCycle = std::make_shared<MergeableTH2Ratio>("DecodingErrorsPerChamberPrevCycle", "Chamber Number vs. Error Type", getErrorCodesSize(), 0, getErrorCodesSize(), 10, 1, 11);
  mHistogramErrorsPerChamberOnCycle = std::make_shared<MergeableTH2Ratio>("DecodingErrorsPerChamberOnCycle", "Chamber Number vs. Error Type, last cycle", getErrorCodesSize(), 0, getErrorCodesSize(), 10, 1, 11);
  setXAxisLabels(mHistogramErrorsPerChamberOnCycle.get());
  setYAxisLabels(mHistogramErrorsPerChamberOnCycle.get());
  publishObject(mHistogramErrorsPerChamberOnCycle, "colz", false, false);

  // Number of decoding errors, grouped by FEE ID and normalized to the number of processed TF
  mHistogramErrorsPerFeeId = std::make_shared<MergeableTH2Ratio>("DecodingErrorsPerFeeId", "FEE ID vs. Error Type", getErrorCodesSize(), 0, getErrorCodesSize(), 64, 0, 64);
  setXAxisLabels(mHistogramErrorsPerFeeId.get());
  publishObject(mHistogramErrorsPerFeeId, "colz", false, false);

  mHistogramErrorsPerFeeIdPrevCycle = std::make_shared<MergeableTH2Ratio>("DecodingErrorsPerFeeIdPrevCycle", "FEE ID vs. Error Type", getErrorCodesSize(), 0, getErrorCodesSize(), 64, 0, 64);
  mHistogramErrorsPerFeeIdOnCycle = std::make_shared<MergeableTH2Ratio>("DecodingErrorsPerFeeIdOnCycle", "FEE ID vs. Error Type, last cycle", getErrorCodesSize(), 0, getErrorCodesSize(), 64, 0, 64);
  setXAxisLabels(mHistogramErrorsPerFeeIdOnCycle.get());
  publishObject(mHistogramErrorsPerFeeIdOnCycle, "colz", false, false);
}

//_____________________________________________________________________________

void DecodingErrorsTask::createHeartBeatHistos()
{
  // Heart-beat packets time distribution and synchronization errors
  mHistogramHBTimeFEC = std::make_shared<MergeableTH2Ratio>("HBTimeFEC", "HB time vs. FEC ID", 64 * 12 * 40, 0, 64 * 12 * 40, 40, mHBExpectedBc - 20, mHBExpectedBc + 20);
  publishObject(mHistogramHBTimeFEC, "colz", false, false);

  uint64_t max = ((static_cast<uint64_t>(0x100000) / 100) + 1) * 100;
  mHistogramHBCoarseTimeFEC = std::make_shared<MergeableTH2Ratio>("HBCoarseTimeFEC", "HB time vs. FEC ID (coarse)", 64 * 12 * 40, 0, 64 * 12 * 40, 100, 0, max);
  publishObject(mHistogramHBCoarseTimeFEC, "colz", false, false);

  mSyncStatusFEC = std::make_shared<TH2F>("SyncStatusFEC", "Heart-beat status vs. FEC ID", 64 * 12 * 40, 0, 64 * 12 * 40, 3, 0, 3);
  mSyncStatusFEC->GetYaxis()->SetBinLabel(1, "OK");
  mSyncStatusFEC->GetYaxis()->SetBinLabel(2, "Out-of-sync");
  mSyncStatusFEC->GetYaxis()->SetBinLabel(3, "Missing");
  publishObject(mSyncStatusFEC, "col", false, false);

  mHistogramSynchErrorsPerDE = std::make_shared<MergeableTH1Ratio>("SynchErrorsPerDE", "Out-of-sync boards fraction per DE", getDEindexMax() + 1, 0, getDEindexMax() + 1);
  publishObject(mHistogramSynchErrorsPerDE, "hist", false, false);

  mHistogramSynchErrorsPerChamber = std::make_shared<MergeableTH1Ratio>("SynchErrorsPerChamber", "Out-of-sync boards fraction per chamber", 10, 0, 10);
  publishObject(mHistogramSynchErrorsPerChamber, "hist", false, false);
}

//_____________________________________________________________________________

void DecodingErrorsTask::initialize(o2::framework::InitContext& /*ic*/)
{
  ILOG(Info, Support) << "initialize DecodingErrorsTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // expected bunch-crossing value in heart-beat packets
  if (auto param = mCustomParameters.find("HBExpectedBc"); param != mCustomParameters.end()) {
    mHBExpectedBc = std::stoi(param->second);
  }

  mElec2Det = createElec2DetMapper<ElectronicMapperGenerated>();
  mFee2Solar = createFeeLink2SolarMapper<ElectronicMapperGenerated>();
  mSolar2Fee = createSolar2FeeLinkMapper<ElectronicMapperGenerated>();

  mHistogramTimeFramesCount = std::make_shared<TH1F>("TimeFramesCount", "Number of Time Frames", 1, 0, 1);
  publishObject(mHistogramTimeFramesCount, "hist", true, false);

  createErrorHistos();
  createHeartBeatHistos();
}

//_____________________________________________________________________________

void DecodingErrorsTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity : " << activity.mId << ENDM;
}

//_____________________________________________________________________________

void DecodingErrorsTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

//_____________________________________________________________________________

void DecodingErrorsTask::decodeTF(framework::ProcessingContext& pc)
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

void DecodingErrorsTask::decodeReadout(const o2::framework::DataRef& input)
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

void DecodingErrorsTask::decodeBuffer(gsl::span<const std::byte> buf)
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

void DecodingErrorsTask::decodePage(gsl::span<const std::byte> page)
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

void DecodingErrorsTask::processErrors(framework::ProcessingContext& pc)
{
  auto rawerrors = pc.inputs().get<gsl::span<o2::mch::DecoderError>>("rawerrors");
  for (auto& error : rawerrors) {
    plotError(error.getSolarID(), error.getDsID(), error.getChip(), error.getError());
  }
}

//_____________________________________________________________________________

void DecodingErrorsTask::plotError(int solarId, int dsAddr, int chip, uint32_t error)
{
  int feeId{ -1 };
  int chamberId{ -1 };

  try {
    std::optional<FeeLinkId> feeLinkId = mSolar2Fee(solarId);
    if (feeLinkId) {
      feeId = feeLinkId->feeId();
    }

    DsElecId dsElecId{ uint16_t(solarId), uint8_t(dsAddr / 5), uint8_t(dsAddr % 5) };
    if (auto opt = mElec2Det(dsElecId); opt.has_value()) {
      DsDetId dsDetId = opt.value();
      int deId = dsDetId.deId();
      chamberId = deId / 100;
    }
  } catch (const std::exception& e) {
    ILOG(Warning, Support) << e.what() << "  SOLAR" << solarId << "  DS" << dsAddr << "  CHIP" << chip << "  ERROR " << error << ENDM;
  }

  if (feeId < 0 || chamberId < 0) {
    return;
  }

  uint32_t errMask = 1;
  for (int i = 0; i < getErrorCodesSize(); i++) {
    // Fill the error histogram if the i-th bin is set in the error word
    if ((error & errMask) != 0) {
      mHistogramErrorsPerChamber->getNum()->Fill(0.5 + i, chamberId);
      mHistogramErrorsPerFeeId->getNum()->Fill(0.5 + i, feeId);
    }
    errMask <<= 1;
  }
}

//_____________________________________________________________________________

void DecodingErrorsTask::processHBPackets(framework::ProcessingContext& pc)
{
  auto hbpackets = pc.inputs().get<gsl::span<o2::mch::HeartBeatPacket>>("hbpackets");
  for (auto& hbp : hbpackets) {
    plotHBPacket(hbp.getSolarID(), hbp.getDsID(), hbp.getChip(), hbp.getBunchCrossing());
  }
}

//_____________________________________________________________________________

void DecodingErrorsTask::plotHBPacket(int solarId, int dsAddr, int chip, uint32_t bc)
{
  int feeId{ -1 };
  std::optional<FeeLinkId> feeLinkId = mSolar2Fee(solarId);
  if (feeLinkId) {
    feeId = feeLinkId->feeId();
  }

  DsElecId dsElecId{ uint16_t(solarId), uint8_t(dsAddr / 5), uint8_t(dsAddr % 5) };
  int chamberId{ -1 };
  if (auto opt = mElec2Det(dsElecId); opt.has_value()) {
    DsDetId dsDetId = opt.value();
    int deId = dsDetId.deId();
    chamberId = deId / 100;
  }

  int fecId = feeId * 12 * 40 + feeLinkId->linkId() * 40 + dsAddr;
  mHistogramHBTimeFEC->getNum()->Fill(fecId, bc);
  mHistogramHBCoarseTimeFEC->getNum()->Fill(fecId, bc);
}

//_____________________________________________________________________________

void DecodingErrorsTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  static int nTF = 0;
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

  updateTFcount(mHistogramErrorsPerChamber.get(), nTF);
  updateTFcount(mHistogramErrorsPerFeeId.get(), nTF);
  updateTFcount(mHistogramHBTimeFEC.get(), nTF);
  updateTFcount(mHistogramHBCoarseTimeFEC.get(), nTF);
}

//_____________________________________________________________________________

int DecodingErrorsTask::getDeId(uint16_t feeId, uint8_t linkId, uint8_t eLinkId)
{
  uint16_t solarId = -1;
  int deId = -1;
  int dsIddet = -1;
  int padId = -1;

  o2::mch::raw::FeeLinkId feeLinkId{ feeId, linkId };

  if (auto opt = mFee2Solar(feeLinkId); opt.has_value()) {
    solarId = opt.value();
  }
  if (solarId < 0 || solarId > 1023) {
    return -1;
  }

  o2::mch::raw::DsElecId dsElecId{ solarId, static_cast<uint8_t>(eLinkId / 5), static_cast<uint8_t>(eLinkId % 5) };

  if (auto opt = mElec2Det(dsElecId); opt.has_value()) {
    o2::mch::raw::DsDetId dsDetId = opt.value();
    dsIddet = dsDetId.dsId();
    deId = dsDetId.deId();
  }

  if (deId < 0 || dsIddet < 0) {
    return -1;
  }

  return deId;
}

//_____________________________________________________________________________

void DecodingErrorsTask::checkSyncErrors()
{
  int mBcMin{ mHBExpectedBc - 2 };
  int mBcMax{ mHBExpectedBc + 2 };

  std::vector<double> deNum(static_cast<size_t>(getDEindexMax() + 1));
  std::vector<double> deDen(static_cast<size_t>(getDEindexMax() + 1));
  std::array<double, 10> chNum;
  std::array<double, 10> chDen;

  std::fill(deNum.begin(), deNum.end(), 0);
  std::fill(deDen.begin(), deDen.end(), 0);
  std::fill(chNum.begin(), chNum.end(), 0);
  std::fill(chDen.begin(), chDen.end(), 0);

  int nbinsx = mHistogramHBTimeFEC->GetXaxis()->GetNbins();
  int nbinsy = mHistogramHBTimeFEC->GetYaxis()->GetNbins();
  for (int i = 1; i <= nbinsx; i++) {
    int index = i - 1;
    int ds_addr = (index % 40);
    int link_id = (index / 40) % 12;
    int fee_id = index / (12 * 40);

    int de = getDeId(fee_id, link_id, ds_addr);
    if (de < 0) {
      continue;
    }

    int deIndex = getDEindex(de);
    if (deIndex < 0) {
      continue;
    }

    int chamber = de / 100 - 1;
    if (chamber < 0 || chamber >= 10) {
      continue;
    }

    deDen[deIndex] += 1;
    chDen[chamber] += 1;

    int ybinmin = mHistogramHBTimeFEC->GetYaxis()->FindBin(mBcMin);
    int ybinmax = mHistogramHBTimeFEC->GetYaxis()->FindBin(mBcMax);
    // number of HB packets in the good bc range
    auto ngood = mHistogramHBTimeFEC->getNum()->Integral(i, i, ybinmin, ybinmax);
    // number of HB packets in the good bc range, normalized to the number of processed time-frames
    // we expect 2 HB packets per TF and per DS (one per SAMPA chip)
    auto ngoodNorm = mHistogramHBTimeFEC->Integral(i, i, ybinmin, ybinmax);

    // total number of HB packets received, including underflow/overflow
    auto total = mHistogramHBTimeFEC->getNum()->Integral(i, i, 1, nbinsy); // integral over all the bc values
    total += mHistogramHBTimeFEC->getNum()->GetBinContent(i, 0);           // add underflow
    total += mHistogramHBTimeFEC->getNum()->GetBinContent(i, nbinsy + 1);  // add overflow

    bool isOutOfSync = false;
    auto nbad = total - ngood;
    if (nbad > 0) {
      isOutOfSync = true;
    }

    bool isMissing = false;
    if (ngoodNorm < 1.5) {
      isMissing = true;
    }

    if (isOutOfSync || isMissing) {
      deNum[deIndex] += 1;
      chNum[chamber] += 1;
    }

    if (!isOutOfSync && !isMissing) {
      mSyncStatusFEC->Fill(i - 1, 0);
    }
    if (isOutOfSync) {
      mSyncStatusFEC->Fill(i - 1, 1);
    }
    if (isMissing) {
      mSyncStatusFEC->Fill(i - 1, 2);
    }
  }

  // update the average number of out-of-sync boards
  for (size_t de = 0; de < deDen.size(); de++) {
    mHistogramSynchErrorsPerDE->getNum()->SetBinContent(de + 1, deNum[de]);
    mHistogramSynchErrorsPerDE->getDen()->SetBinContent(de + 1, deDen[de]);
  }
  for (size_t ch = 0; ch < chDen.size(); ch++) {
    mHistogramSynchErrorsPerChamber->getNum()->SetBinContent(ch + 1, chNum[ch]);
    mHistogramSynchErrorsPerChamber->getDen()->SetBinContent(ch + 1, chDen[ch]);
  }
  mHistogramSynchErrorsPerDE->update();
  mHistogramSynchErrorsPerChamber->update();
}

//_____________________________________________________________________________

void DecodingErrorsTask::endOfCycle()
{
  // copy bin contents from src to dst
  auto copyHist = [&](TH2F* hdst, TH2F* hsrc) {
    hdst->Reset("ICES");
    hdst->Add(hsrc);
  };

  // copy numerator and denominator from src to dst
  auto copyRatio = [&](std::shared_ptr<MergeableTH2Ratio> dst, std::shared_ptr<MergeableTH2Ratio> src) {
    copyHist(dst->getNum(), src->getNum());
    copyHist(dst->getDen(), src->getDen());
  };

  // dst = src1 - src2
  auto subtractHist = [&](TH2F* hdst, TH2F* hsrc1, TH2F* hsrc2) {
    hdst->Reset("ICES");
    hdst->Add(hsrc1);
    hdst->Add(hsrc2, -1);
  };

  // compute (src1 - src2) difference of numerators and denominators
  auto subtractRatio = [&](std::shared_ptr<MergeableTH2Ratio> dst, std::shared_ptr<MergeableTH2Ratio> src1, std::shared_ptr<MergeableTH2Ratio> src2) {
    subtractHist(dst->getNum(), src1->getNum(), src2->getNum());
    subtractHist(dst->getDen(), src1->getDen(), src2->getDen());
    dst->update();
  };

  ILOG(Info, Support) << "endOfCycle" << ENDM;

  mHistogramErrorsPerChamber->update();
  mHistogramErrorsPerFeeId->update();

  mHistogramHBCoarseTimeFEC->update();
  mHistogramHBTimeFEC->update();
  checkSyncErrors();

  // fill on-cycle plots
  subtractRatio(mHistogramErrorsPerChamberOnCycle, mHistogramErrorsPerChamber, mHistogramErrorsPerChamberPrevCycle);
  subtractRatio(mHistogramErrorsPerFeeIdOnCycle, mHistogramErrorsPerFeeId, mHistogramErrorsPerFeeIdPrevCycle);

  // update last cycle plots
  copyRatio(mHistogramErrorsPerChamberPrevCycle, mHistogramErrorsPerChamber);
  copyRatio(mHistogramErrorsPerFeeIdPrevCycle, mHistogramErrorsPerFeeId);
}

//_____________________________________________________________________________

void DecodingErrorsTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

//_____________________________________________________________________________

void DecodingErrorsTask::reset()
{
  // clean all the monitor objects here
  ILOG(Info, Support) << "Resetting the histograms" << ENDM;

  for (auto h : mAllHistograms) {
    h->Reset("ICES");
  }
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
