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
/// \file   TaskCompressedData.cxx
/// \author Nicolo' Jacazio
/// \brief  Task To monitor data converted from TOF compressor, it implements a dedicated decoder from DecoderBase
///

// ROOT includes
#include <TCanvas.h>
#include <TH1.h>
#include <TH1F.h>
#include <TH2F.h>

// O2 includes
#include "DataFormatsTOF/CompressedDataFormat.h"
#include <Framework/DataRefUtils.h>
#include "Headers/RAWDataHeader.h"
#include "DetectorsRaw/HBFUtils.h"

using namespace o2::framework;

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TOF/TaskCompressedData.h"

namespace o2::quality_control_modules::tof
{

// Implement decoder for Task

void CompressedDataDecoder::decode()
{
  DecoderBase::run();
}

void CompressedDataDecoder::headerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* /*crateOrbit*/)
{
  for (int ibit = 0; ibit < 11; ++ibit) {
    if (crateHeader->slotPartMask & (1 << ibit)) {
      mHistos.at("hSlotPartMask")->Fill(crateHeader->drmID, ibit + 2);
    }
  }
}

void CompressedDataDecoder::frameHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* crateOrbit,
                                         const FrameHeader_t* frameHeader, const PackedHit_t* packedHits)
{
  mHistos.at("hHits")->Fill(frameHeader->numberOfHits);
  for (int i = 0; i < frameHeader->numberOfHits; ++i) {
    auto packedHit = packedHits + i;
    auto indexE = packedHit->channel +
                  8 * packedHit->tdcID +
                  120 * packedHit->chain +
                  240 * (frameHeader->trmID - 3) +
                  2400 * crateHeader->drmID;
    int time = packedHit->time;
    int timebc = time % 1024;
    time += (frameHeader->frameID << 13);

    mHistos.at("hIndexE")->Fill(indexE);
    mHistos.at("hTime")->Fill(time);
    mHistos.at("hTimeBC")->Fill(timebc);
    mHistos.at("hTOT")->Fill(packedHit->tot);
  }
}

void CompressedDataDecoder::trailerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* /*crateOrbit*/,
                                           const CrateTrailer_t* crateTrailer, const Diagnostic_t* diagnostics,
                                           const Error_t* errors)
{
  for (int i = 0; i < crateTrailer->numberOfDiagnostics; ++i) {
    auto diagnostic = diagnostics + i;
    mHistos.at("hDiagnostic")->Fill(crateHeader->drmID, diagnostic->slotID);
  }
  int nError = 0, nTest = 0;
  for (int i = 0; i < crateTrailer->numberOfErrors; ++i) {
    auto error = errors + i;
    if (error->undefined) {
      nTest++;
      mHistos.at("hTest")->Fill(error->slotID + 0.5 * error->chain, error->tdcID);
    } else {
      nError++;
      mHistos.at("hError")->Fill(error->slotID + 0.5 * error->chain, error->tdcID);
      for (int ibit = 0; ibit < 15; ++ibit) {
        if (error->errorFlags & (1 << ibit))
          mHistos.at("hErrorBit")->Fill(ibit);
      }
    }
  }
  mHistos.at("hNErrors")->Fill(nError);
  mHistos.at("hNTests")->Fill(nTest);
}

#ifdef VERBOSEDECODERCOMPRESSED
void CompressedDataDecoder::rdhHandler(const o2::header::RAWDataHeader* rdh)
{
  LOG(INFO) << "Reading RDH #" << rdhread++ / 2;
  o2::raw::RDHUtils::printRDH(*rdh);
}
#else
void CompressedDataDecoder::rdhHandler(const o2::header::RAWDataHeader* /*rdh*/)
{
}
#endif

// Implement Task
void TaskCompressedData::initialize(o2::framework::InitContext& /*ctx*/)
{
  LOG(INFO) << "initialize TaskCompressedData";
  if (auto param = mCustomParameters.find("DecoderCONET"); param != mCustomParameters.end()) {
    if (param->second == "True") {
      LOG(INFO) << "Rig for DecoderCONET";
      mDecoder.setDecoderCONET(kTRUE);
    }
  }

  mHits.reset(new TH1F("hHits", "Raw Hits;Hits per event", 1000, 0., 1000.));
  getObjectsManager()->startPublishing(mHits.get());
  mDecoder.mHistos[mHits->GetName()] = mHits;
  //
  mTime.reset(new TH1F("hTime", "Raw Time;Time (24.4 ps)", 2097152, 0., 2097152.));
  getObjectsManager()->startPublishing(mTime.get());
  mDecoder.mHistos[mTime->GetName()] = mTime;
  //
  mTimeBC.reset(new TH1F("hTimeBC", "Raw BC Time;BC time (24.4 ps)", 1024, 0., 1024.));
  getObjectsManager()->startPublishing(mTimeBC.get());
  mDecoder.mHistos[mTimeBC->GetName()] = mTimeBC;
  //
  mTOT.reset(new TH1F("hTOT", "Raw ToT;ToT (48.8 ps)", 2048, 0., 2048.));
  getObjectsManager()->startPublishing(mTOT.get());
  mDecoder.mHistos[mTOT->GetName()] = mTOT;
  //
  mIndexE.reset(new TH1F("hIndexE", "Equipment index;index EO", 172800, 0., 172800.));
  getObjectsManager()->startPublishing(mIndexE.get());
  mDecoder.mHistos[mIndexE->GetName()] = mIndexE;
  //
  mSlotPartMask.reset(new TH2F("hSlotPartMask", "Slot Participating;crate;slot", 72, 0., 72., 12, 1., 13.));
  getObjectsManager()->startPublishing(mSlotPartMask.get());
  mDecoder.mHistos[mSlotPartMask->GetName()] = mSlotPartMask;
  //
  mDiagnostic.reset(new TH2F("hDiagnostic", "hDiagnostic;crate;slot", 72, 0., 72., 12, 1., 13.));
  getObjectsManager()->startPublishing(mDiagnostic.get());
  mDecoder.mHistos[mDiagnostic->GetName()] = mDiagnostic;
  //
  mNErrors.reset(new TH1F("hNErrors", "Error numbers;Number of errors", 1000, 0., 1000.));
  getObjectsManager()->startPublishing(mNErrors.get());
  mDecoder.mHistos[mNErrors->GetName()] = mNErrors;
  //
  mErrorBits.reset(new TH1F("hErrorBit", "Error Bit;TDC error bit", 15, 0., 15.));
  getObjectsManager()->startPublishing(mErrorBits.get());
  mDecoder.mHistos[mErrorBits->GetName()] = mErrorBits;
  //
  mError.reset(new TH2F("hError", "Errors;slot;TDC", 24, 1., 13., 15, 0., 15.));
  getObjectsManager()->startPublishing(mError.get());
  mDecoder.mHistos[mError->GetName()] = mError;
  //
  mNTests.reset(new TH1F("hNTests", "Test numbers;Number of errors", 1000, 0., 1000.));
  getObjectsManager()->startPublishing(mNTests.get());
  mDecoder.mHistos[mNTests->GetName()] = mNTests;
  //
  mTest.reset(new TH2F("hTest", "Tests;slot;TDC", 24, 1., 13., 15, 0., 15.));
  getObjectsManager()->startPublishing(mTest.get());
  mDecoder.mHistos[mTest->GetName()] = mTest;
  //
}

void TaskCompressedData::startOfActivity(Activity& /*activity*/)
{
  LOG(INFO) << "startOfActivity";
  reset();
}

void TaskCompressedData::startOfCycle()
{
  LOG(INFO) << "startOfCycle";
}

void TaskCompressedData::monitorData(o2::framework::ProcessingContext& ctx)
{
  /** receive input **/
  for (auto& input : ctx.inputs()) {
    /** input **/
    const auto* headerIn = o2::framework::DataRefUtils::getHeader<o2::header::DataHeader*>(input);
    auto payloadIn = input.payload;
    auto payloadInSize = headerIn->payloadSize;
    mDecoder.setDecoderBuffer(payloadIn);
    mDecoder.setDecoderBufferSize(payloadInSize);
    mDecoder.decode();
  }
}

void TaskCompressedData::endOfCycle()
{
  LOG(INFO) << "endOfCycle";
}

void TaskCompressedData::endOfActivity(Activity& /*activity*/)
{
  LOG(INFO) << "endOfActivity";
}

void TaskCompressedData::reset()
{
  // clean all the monitor objects here

  LOG(INFO) << "Resetting the histograms";
  mHits->Reset();
  mTime->Reset();
  mTimeBC->Reset();
  mTOT->Reset();
  mIndexE->Reset();
  mSlotPartMask->Reset();
  mDiagnostic->Reset();
  mNErrors->Reset();
  mErrorBits->Reset();
  mError->Reset();
  mNTests->Reset();
  mTest->Reset();
}

} // namespace o2::quality_control_modules::tof
