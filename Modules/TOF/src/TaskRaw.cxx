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
/// \file   TaskRaw.cxx
/// \author Nicolo' Jacazio and Francesca Ercolessi
/// \brief  Task To monitor data converted from TOF compressor and diagnostic words from TOF crates
/// \since  20-11-2020
///

// ROOT includes
#include <TH1.h>
#include <TH1F.h>
#include <TH2F.h>

// O2 includes
#include "DataFormatsTOF/CompressedDataFormat.h"
#include <Framework/DataRefUtils.h>
#include "Headers/RAWDataHeader.h"
#include "DetectorsRaw/HBFUtils.h"
#include <Framework/InputRecord.h>

// Fairlogger includes
#include <fairlogger/Logger.h>

using namespace o2::framework;
using namespace o2::tof;

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TOF/TaskRaw.h"

namespace o2::quality_control_modules::tof
{

void RawDataDecoder::decode()
{
  DecoderBase::run();
}

void RawDataDecoder::headerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* crateOrbit)
{
  // DRM Counter
  if (crateHeader->slotPartMask & 0 << 0) {
    mDRMCounter[crateHeader->drmID].Count(0);
  }

  // LTM Counter
  if (crateHeader->slotPartMask & 1 << 0) {
    mLTMCounter[crateHeader->drmID].Count(0);
  }

  // TRM Counter
  for (int i = 1; i < 11; i++) {
    if (crateHeader->slotPartMask & 1 << i) {
      mTRMCounter[crateHeader->drmID][i - 1].Count(0);
    }
  }

  // Paricipating slot
  for (int ibit = 0; ibit < 11; ++ibit) {
    if (crateHeader->slotPartMask & (1 << ibit)) {
      mSlotPartMask->Fill(crateHeader->drmID, ibit + 2);
    }
  }

  // Orbit ID
  mOrbitID->Fill(crateOrbit->orbitID % 1048576, crateHeader->drmID);
}

void RawDataDecoder::frameHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* /*crateOrbit*/,
                                  const FrameHeader_t* frameHeader, const PackedHit_t* packedHits)
{
  // Number of hits
  mHits->Fill(frameHeader->numberOfHits);
  for (int i = 0; i < frameHeader->numberOfHits; ++i) {
    auto packedHit = packedHits + i;
    auto indexE = packedHit->channel +
                  8 * packedHit->tdcID +
                  120 * packedHit->chain +
                  240 * (frameHeader->trmID - 3) +
                  2400 * crateHeader->drmID;
    int time = packedHit->time;
    const int timebc = time % 1024;
    time += (frameHeader->frameID << 13);

    // Equipment index
    mIndexE->Fill(indexE);
    // Raw time
    mTime->Fill(time);
    // BC time
    mTimeBC->Fill(timebc);
    // ToT
    mTOT->Fill(packedHit->tot);
  }
}

void RawDataDecoder::trailerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* /*crateOrbit*/,
                                    const CrateTrailer_t* crateTrailer, const Diagnostic_t* diagnostics,
                                    const Error_t* errors)
{
  // Diagnostic word per slot
  const int drmID = crateHeader->drmID;
  constexpr unsigned int reserved_words = 4;                   // First 4 bits are reserved
  constexpr unsigned int words_to_check = 32 - reserved_words; // Words to check
  for (int i = 0; i < crateTrailer->numberOfDiagnostics; ++i) {
    auto diagnostic = diagnostics + i;
    const int slotID = diagnostic->slotID;
    if (slotID == 1) { // Here we have a DRM
      for (unsigned int j = 0; j < words_to_check; j++) {
        if (diagnostic->faultBits & 1 << j) {
          mDRMCounter[drmID].Count(j + reserved_words);
        }
      }
    } else if (slotID == 2) { // Here we have a LTM
      for (unsigned int j = 0; j < words_to_check; j++) {
        if (diagnostic->faultBits & 1 << j) {
          mLTMCounter[drmID].Count(j + reserved_words);
        }
      }
    } else { // Here we have a TRM
      for (unsigned int j = 0; j < words_to_check; j++) {
        if (diagnostic->faultBits & 1 << j) {
          mTRMCounter[drmID][slotID - 3].Count(j + reserved_words);
        }
      }
    }
  }

  // Number of diagnostics per crate
  for (int i = 0; i < crateTrailer->numberOfDiagnostics; ++i) {
    auto diagnostic = diagnostics + i;
    mDiagnostic->Fill(crateHeader->drmID, diagnostic->slotID);
  }
  // Errors in the TDCs
  int nError = 0, nTest = 0;
  for (int i = 0; i < crateTrailer->numberOfErrors; ++i) {
    auto error = errors + i;
    if (error->undefined) {
      nTest++;
      mTest->Fill(error->slotID + 0.5 * error->chain, error->tdcID);
    } else {
      nError++;
      mError->Fill(error->slotID + 0.5 * error->chain, error->tdcID);
      for (int ibit = 0; ibit < 15; ++ibit) {
        if (error->errorFlags & (1 << ibit))
          mErrorBits->Fill(ibit);
      }
    }
  }
  mNErrors->Fill(nError);
  mNTests->Fill(nTest);
}

void RawDataDecoder::initHistograms() // Initialization of histograms in Decoder
{
  mHits.reset(new TH1F("hHits", "Raw Hits;Hits per event", 1000, 0., 1000.));
  //
  mTime.reset(new TH1F("hTime", "Raw Time;Time (24.4 ps)", 2097152, 0., 2097152.));
  //
  mTimeBC.reset(new TH1F("hTimeBC", "Raw BC Time;BC time (24.4 ps)", 1024, 0., 1024.));
  //
  mTOT.reset(new TH1F("hTOT", "Raw ToT;ToT (48.8 ps)", 2048, 0., 2048.));
  //
  mIndexE.reset(new TH1F("hIndexE", "Equipment index;index EO", 172800, 0., 172800.));
  //
  mSlotPartMask.reset(new TH2F("hSlotPartMask", "Slot Participating;crate;slot", 72, 0., 72., 12, 1., 13.));
  //
  mDiagnostic.reset(new TH2F("hDiagnostic", "hDiagnostic;crate;slot", 72, 0., 72., 12, 1., 13.));
  //
  mNErrors.reset(new TH1F("hNErrors", "Error numbers;Number of errors", 1000, 0., 1000.));
  //
  mErrorBits.reset(new TH1F("hErrorBit", "Error Bit;TDC error bit", 15, 0., 15.));
  //
  mError.reset(new TH2F("hError", "Errors;slot;TDC", 24, 1., 13., 15, 0., 15.));
  //
  mNTests.reset(new TH1F("hNTests", "Test numbers;Number of errors", 1000, 0., 1000.));
  //
  mTest.reset(new TH2F("hTest", "Tests;slot;TDC", 24, 1., 13., 15, 0., 15.));
  //
  mOrbitID.reset(new TH2F("hOrbitID", "OrbitID;OrbitID % 1048576;Crate", 1024, 0, 1048576, 72, 0, 72));
  //
}

void RawDataDecoder::resetHistograms() // Reset of histograms in Decoder
{
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
  mOrbitID->Reset();
}

// Implement the Task
void TaskRaw::initialize(o2::framework::InitContext& /*ctx*/)
{
  if (auto param = mCustomParameters.find("DecoderCONET"); param != mCustomParameters.end()) {
    if (param->second == "True") {
      LOG(INFO) << "Rig for DecoderCONET";
      mDecoderRaw.setDecoderCONET(kTRUE);
    }
  }

  mDRMHisto.reset(new TH2F("DRMCounter", "DRM Diagnostics;DRM Word;Crate;Words", 32, 0, 32, 72, 0, 72));
  mDecoderRaw.mDRMCounter[0].MakeHistogram(mDRMHisto.get());
  getObjectsManager()->startPublishing(mDRMHisto.get());
  mLTMHisto.reset(new TH2F("LTMCounter", "LTM Diagnostics;LTM Word;Crate;Words", 32, 0, 32, 72, 0, 72));
  mDecoderRaw.mLTMCounter[0].MakeHistogram(mLTMHisto.get());
  getObjectsManager()->startPublishing(mLTMHisto.get());
  for (int j = 0; j < RawDataDecoder::ntrms; j++) {
    mTRMHisto[j].reset(new TH2F(Form("TRMCounterSlot%i", j), Form("TRM %i Diagnostics;TRM Word;Crate;Words", j), 32, 0, 32, 72, 0, 72));
    mDecoderRaw.mTRMCounter[0][j].MakeHistogram(mTRMHisto[j].get());
    getObjectsManager()->startPublishing(mTRMHisto[j].get());
  }

  mDecoderRaw.initHistograms();
  getObjectsManager()->startPublishing(mDecoderRaw.mHits.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mTime.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mTimeBC.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mTOT.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mIndexE.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mSlotPartMask.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mDiagnostic.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mNErrors.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mErrorBits.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mError.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mNTests.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mTest.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mOrbitID.get());
}

void TaskRaw::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;
  reset();
  mDecoderRaw.resetHistograms();
}

void TaskRaw::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void TaskRaw::monitorData(o2::framework::ProcessingContext& ctx)
{

  for (auto iit = ctx.inputs().begin(), iend = ctx.inputs().end(); iit != iend; ++iit) {
    if (!iit.isValid()) {
      continue;
    }
    /** loop over input parts **/
    for (auto const& input : iit) {
      /** input **/
      const auto* headerIn = o2::framework::DataRefUtils::getHeader<o2::header::DataHeader*>(input);
      auto payloadIn = input.payload;
      auto payloadInSize = headerIn->payloadSize;
      mDecoderRaw.setDecoderBuffer(payloadIn);
      mDecoderRaw.setDecoderBufferSize(payloadInSize);
      mDecoderRaw.decode();
    }
  }
}

void TaskRaw::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
  for (int i = 0; i < RawDataDecoder::ncrates; i++) { // Filling histograms only at the end of the cycle
    mDecoderRaw.mDRMCounter[i].FillHistogram(mDRMHisto.get(), i + 1);
    mDecoderRaw.mLTMCounter[i].FillHistogram(mLTMHisto.get(), i + 1);
    for (int j = 0; j < RawDataDecoder::ntrms; j++) {
      mDecoderRaw.mTRMCounter[i][j].FillHistogram(mTRMHisto[j].get(), i + 1);
    }
  }
}

void TaskRaw::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void TaskRaw::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  mDRMHisto->Reset();
  for (int j = 0; j < RawDataDecoder::ntrms; j++) {
    mTRMHisto[j]->Reset();
  }
  mDecoderRaw.resetHistograms();
}

} // namespace o2::quality_control_modules::tof