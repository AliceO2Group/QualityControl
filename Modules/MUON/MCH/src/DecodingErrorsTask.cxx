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

void DecodingErrorsTask::initialize(o2::framework::InitContext& /*ic*/)
{
  ILOG(Info, Support) << "initialize DecodingErrorsTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  mElec2Det = createElec2DetMapper<ElectronicMapperGenerated>();
  mFee2Solar = createFeeLink2SolarMapper<ElectronicMapperGenerated>();
  mSolar2Fee = createSolar2FeeLinkMapper<ElectronicMapperGenerated>();

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

void DecodingErrorsTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity : " << activity.mId << ENDM;
}

void DecodingErrorsTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

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

void DecodingErrorsTask::processErrors(framework::ProcessingContext& pc)
{
  auto rawerrors = pc.inputs().get<gsl::span<o2::mch::DecoderError>>("rawerrors");
  for (auto& error : rawerrors) {
    plotError(error.getSolarID(), error.getDsID(), error.getChip(), error.getError());
  }
}

void DecodingErrorsTask::plotError(int solarId, int dsAddr, int chip, uint32_t error)
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
  }

  // Count the number of processed TF and set the denominators of the error histograms accordingly
  nTF += 1;

  auto hTF = mHistogramErrorsPerChamber->getDen();
  for (int ybin = 0; ybin <= hTF->GetYaxis()->GetNbins(); ybin++) {
    for (int xbin = 0; xbin <= hTF->GetXaxis()->GetNbins(); xbin++) {
      hTF->SetBinContent(xbin, ybin, nTF);
    }
  }

  hTF = mHistogramErrorsPerFeeId->getDen();
  for (int ybin = 0; ybin <= hTF->GetYaxis()->GetNbins(); ybin++) {
    for (int xbin = 0; xbin <= hTF->GetXaxis()->GetNbins(); xbin++) {
      hTF->SetBinContent(xbin, ybin, nTF);
    }
  }
}

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

  // fill on-cycle plots
  subtractRatio(mHistogramErrorsPerChamberOnCycle, mHistogramErrorsPerChamber, mHistogramErrorsPerChamberPrevCycle);
  subtractRatio(mHistogramErrorsPerFeeIdOnCycle, mHistogramErrorsPerFeeId, mHistogramErrorsPerFeeIdPrevCycle);

  // update last cycle plots
  copyRatio(mHistogramErrorsPerChamberPrevCycle, mHistogramErrorsPerChamber);
  copyRatio(mHistogramErrorsPerFeeIdPrevCycle, mHistogramErrorsPerFeeId);
}

void DecodingErrorsTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;

  mHistogramErrorsPerChamber->update();
  mHistogramErrorsPerFeeId->update();
}

void DecodingErrorsTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histograms" << ENDM;

  for (auto h : mAllHistograms) {
    h->Reset();
  }
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
