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

//#define QC_MCH_SAVE_TEMP_ROOTFILE 1

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

static int sErrorMax = 11;

static void setXAxisLabels(TH2F* hErrors)
{
  TAxis* ax = hErrors->GetXaxis();
  ax->SetBinLabel(1, "Parity Error");
  ax->SetBinLabel(2, "Hamming Error (correctable)");
  ax->SetBinLabel(3, "Hamming Error (uncorrectable)");
  ax->SetBinLabel(4, "Bad Cluster Size");
  ax->SetBinLabel(5, "Bad Packet Type");
  ax->SetBinLabel(6, "Bad HB Packet");
  ax->SetBinLabel(7, "Bad Incomplete word");
  ax->SetBinLabel(8, "Truncated Data");
  ax->SetBinLabel(9, "Bad Elink ID");
  ax->SetBinLabel(10, "Bad Link ID");
  ax->SetBinLabel(11, "Unknown Link ID");
  for (int i = 1; i <= sErrorMax; i++) {
    ax->ChangeLabel(i, 45);
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

  mSaveToRootFile = false;
  if (auto param = mCustomParameters.find("SaveToRootFile"); param != mCustomParameters.end()) {
    if (param->second == "true" || param->second == "True" || param->second == "TRUE") {
      mSaveToRootFile = true;
    }
  }

  mElec2Det = createElec2DetMapper<ElectronicMapperGenerated>();
  mFee2Solar = createFeeLink2SolarMapper<ElectronicMapperGenerated>();
  mSolar2Fee = createSolar2FeeLinkMapper<ElectronicMapperGenerated>();

  // Number of decoding errors, grouped by chamber ID and normalized to the number of processed TF
  mHistogramErrorsPerChamber = std::make_shared<MergeableTH2Ratio>("DecodingErrorsPerChamber", "Chamber Number vs. Error Type", sErrorMax, 1, sErrorMax + 1, 10, 1, 11);
  setXAxisLabels(mHistogramErrorsPerChamber.get());
  setYAxisLabels(mHistogramErrorsPerChamber.get());
  mHistogramErrorsPerChamber->SetOption("colz");
  mAllHistograms.push_back(mHistogramErrorsPerChamber.get());
  if (!mSaveToRootFile) {
    getObjectsManager()->startPublishing(mHistogramErrorsPerChamber.get());
  }

  // Number of decoding errors, grouped by FEE ID and normalized to the number of processed TF
  mHistogramErrorsPerFeeId = std::make_shared<MergeableTH2Ratio>("DecodingErrorsPerFeeId", "FEE ID vs. Error Type", sErrorMax, 1, sErrorMax + 1, 64, 0, 64);
  setXAxisLabels(mHistogramErrorsPerFeeId.get());
  mHistogramErrorsPerFeeId->SetOption("colz");
  mAllHistograms.push_back(mHistogramErrorsPerFeeId.get());
  if (!mSaveToRootFile) {
    getObjectsManager()->startPublishing(mHistogramErrorsPerFeeId.get());
  }
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
  auto errorHandler = [&](DsElecId dsElecId, int8_t /*chip*/, uint32_t error) {
    int feeId{ -1 };
    uint32_t solarId = dsElecId.solarId();
    uint32_t dsAddr = dsElecId.elinkId();

    std::optional<FeeLinkId> feeLinkId = mSolar2Fee(solarId);
    if (feeLinkId) {
      feeId = feeLinkId->feeId();
    }

    int chamberId{ -1 };
    if (auto opt = mElec2Det(dsElecId); opt.has_value()) {
      DsDetId dsDetId = opt.value();
      int deId = dsDetId.deId();
      chamberId = deId / 100;
    }

    uint32_t errMask = 1;
    for (int i = 0; i < 30; i++) {
      // Fill the error histogram if the i-th bin is set in the error word
      if ((error & errMask) != 0) {
        mHistogramErrorsPerChamber->getNum()->Fill(0.5 + i, chamberId);
        mHistogramErrorsPerFeeId->getNum()->Fill(0.5 + i, feeId);
      }
      errMask <<= 1;
    }
  };

  if (!mDecoder) {
    o2::mch::raw::DecodedDataHandlers handlers;
    handlers.sampaErrorHandler = errorHandler;
    mDecoder = o2::mch::raw::createPageDecoder(page, handlers);
  }
  mDecoder(page);
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

void DecodingErrorsTask::writeHistos()
{
  TFile f("mch-qc-errors.root", "RECREATE");
  for (auto h : mAllHistograms) {
    h->Write();
  }
  f.Close();
}

void DecodingErrorsTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;

  mHistogramErrorsPerChamber->update();
  mHistogramErrorsPerFeeId->update();

  if (mSaveToRootFile) {
    writeHistos();
  }
}

void DecodingErrorsTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;

  mHistogramErrorsPerChamber->update();
  mHistogramErrorsPerFeeId->update();

  if (mSaveToRootFile) {
    writeHistos();
  }
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
