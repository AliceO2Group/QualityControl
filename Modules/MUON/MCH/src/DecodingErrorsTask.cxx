// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
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

DecodingErrorsTask::~DecodingErrorsTask()
{
  delete mHistogramErrors;
}

static void setAxisLabels(TH2F* hErrors)
{
  TAxis* ax = hErrors->GetXaxis();
  for (int i = 1; i <= 10; i++) {
    auto label = fmt::format("CH{}", i);
    ax->SetBinLabel(i, label.c_str());
  }
  TAxis* ay = hErrors->GetYaxis();
  //a->SetBit(TAxis::kLabelsHori);
  ay->SetBinLabel(1, "Parity Error");
  ay->SetBinLabel(2, "Hamming Error (correctable)");
  ay->SetBinLabel(3, "Hamming Error (uncorrectable)");
  ay->SetBinLabel(4, "Bad Cluster Size");
  ay->SetBinLabel(5, "Bad Packet Type");
  ay->SetBinLabel(6, "Bad HB Packet");
  ay->SetBinLabel(7, "Bad Incomplete word");
  ay->SetBinLabel(8, "Truncated Data");
  ay->SetBinLabel(9, "Bad Elink ID");
  ay->SetBinLabel(10, "Bad Link ID");
  ay->SetBinLabel(11, "Unknown Link ID");
  for (int i = 1; i <= 11; i++) {
    ay->ChangeLabel(i, 45);
  }
}

void DecodingErrorsTask::initialize(o2::framework::InitContext& /*ic*/)
{
  ILOG(Info) << "initialize DecodingErrorsTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  mElec2Det = createElec2DetMapper<ElectronicMapperGenerated>();
  mFee2Solar = createFeeLink2SolarMapper<ElectronicMapperGenerated>();

  mHistogramErrors = new TH2F("QcMuonChambers_Errors", "Error codes vs. Chamber Number", 10, 1, 11, 10, 0, 10);
  setAxisLabels(mHistogramErrors);
  getObjectsManager()->startPublishing(mHistogramErrors);
}

void DecodingErrorsTask::startOfActivity(Activity& activity)
{
  ILOG(Info) << "startOfActivity : " << activity.mId << ENDM;
  mHistogramErrors->Reset();
}

void DecodingErrorsTask::startOfCycle()
{
  ILOG(Info) << "startOfCycle" << ENDM;
}

void DecodingErrorsTask::decodeTF(framework::ProcessingContext& pc)
{
  // get the input buffer
  auto& inputs = pc.inputs();
  DPLRawParser parser(inputs, o2::framework::select("TF:MCH/RAWDATA"));

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

  const auto* header = o2::header::get<header::DataHeader*>(input.header);
  if (!header) {
    return;
  }

  size_t payloadSize = header->payloadSize;
  if (payloadSize == 0) {
    return;
  }

  auto const* raw = input.payload;

  gsl::span<const std::byte> buffer(reinterpret_cast<const std::byte*>(raw), payloadSize);
  decodeBuffer(buffer);
}

void DecodingErrorsTask::decodeBuffer(gsl::span<const std::byte> buf)
{
  ILOG(Debug) << "Start of new buffer" << ENDM;

  size_t bufSize = buf.size();
  size_t pageStart = 0;
  while (bufSize > pageStart) {
    RDH* rdh = reinterpret_cast<RDH*>(const_cast<std::byte*>(&(buf[pageStart])));
    auto rdhHeaderSize = o2::raw::RDHUtils::getHeaderSize(rdh);
    if (rdhHeaderSize != 64) {
      break;
    }
    auto pageSize = o2::raw::RDHUtils::getOffsetToNext(rdh);

    gsl::span<const std::byte> page(reinterpret_cast<const std::byte*>(rdh), pageSize);
    decodePage(page);

    pageStart += pageSize;
  }
}

void DecodingErrorsTask::decodePage(gsl::span<const std::byte> page)
{
  int nErrors = 0;
  auto errorHandler = [&](DsElecId dsElecId, int8_t /*chip*/, uint32_t error) {
    nErrors += 1;
    int deId{ -1 };
    if (auto opt = mElec2Det(dsElecId); opt.has_value()) {
      DsDetId dsDetId = opt.value();
      deId = dsDetId.deId();
    }

    if (deId < 0) {
      return;
    }
    int chamber = deId / 100;

    uint32_t errMask = 1;
    for (int i = 0; i < 30; i++) {
      // Fill the error histogram if the i-th bin is set in the error word
      if ((error & errMask) != 0) {
        mHistogramErrors->Fill(chamber, 0.5 + i);
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

  auto& rdhAny = *reinterpret_cast<RDH*>(const_cast<std::byte*>(&(page[0])));
  int feeId = o2::raw::RDHUtils::getFEEID(rdhAny) & 0x7F;
  ILOG(Debug) << "Received " << nErrors << " from " << feeId << ENDM;
}

void DecodingErrorsTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  for (auto&& input : ctx.inputs()) {
    if (input.spec->binding == "readout") {
      decodeReadout(input);
    }
    if (input.spec->binding == "TF") {
      decodeTF(ctx);
    }
  }
}

void DecodingErrorsTask::saveHistograms()
{
  TFile f("/tmp/qc-errors.root", "RECREATE");

  mHistogramErrors->Write();

  f.ls();
  f.Close();
}

void DecodingErrorsTask::endOfCycle()
{
  ILOG(Info) << "endOfCycle" << ENDM;
}

void DecodingErrorsTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "endOfActivity" << ENDM;

#ifdef QC_MCH_SAVE_TEMP_ROOTFILE
  saveHistograms();
#endif
}

void DecodingErrorsTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info) << "Resetting the histogram" << ENDM;
  mHistogramErrors->Reset();
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
