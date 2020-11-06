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
/// \file   TaskDiagnostics.cxx
/// \author Nicolo' Jacazio
/// \brief Task to check the diagnostic words of TOF crates
///

// ROOT includes
#include "TH1F.h"
#include "TH2F.h"

// O2 includes
#include "DataFormatsTOF/CompressedDataFormat.h"
#include <Framework/DataRefUtils.h>
#include "Headers/RAWDataHeader.h"
#include "DetectorsRaw/HBFUtils.h"
#include "Framework/InputRecord.h"

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TOF/TaskDiagnostics.h"

using namespace o2::tof;

namespace o2::quality_control_modules::tof
{

void DiagnosticsCounter::decode()
{
  DecoderBase::run();
}

void DiagnosticsCounter::headerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* /*crateOrbit*/)
{
  // DRM
  if (crateHeader->slotPartMask & 0 << 0) {
    mDRMCounter[crateHeader->drmID].Count(0);
  }
  // LTM
  if (crateHeader->slotPartMask & 1 << 0) {
    mLTMCounter[crateHeader->drmID].Count(0);
  }
  // TRM
  for (int i = 1; i < 11; i++) {
    if (crateHeader->slotPartMask & 1 << i) {
      mTRMCounter[crateHeader->drmID][i - 1].Count(0);
    }
  }
}

void DiagnosticsCounter::trailerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* /*crateOrbit*/,
                                        const CrateTrailer_t* crateTrailer, const Diagnostic_t* diagnostics,
                                        const Error_t* /*errors*/)
{
  const int drmID = crateHeader->drmID;
  const unsigned int words_to_check = 32 - 4;
  for (int i = 0; i < crateTrailer->numberOfDiagnostics; ++i) {
    auto diagnostic = diagnostics + i;
    const int slotID = diagnostic->slotID;
    if (slotID == 1) { // Here we have a DRM
      for (unsigned int j = 0; j < words_to_check; j++) {
        if (diagnostic->faultBits & 1 << j) {
          mDRMCounter[drmID].Count(j + 4);
        }
      }
    } else if (slotID == 2) { // Here we have a LTM
      for (unsigned int j = 0; j < words_to_check; j++) {
        if (diagnostic->faultBits & 1 << j) {
          mLTMCounter[drmID].Count(j + 4);
        }
      }
    } else { // Here we have a TRM
      const int trmID = slotID - 3;
      for (unsigned int j = 0; j < words_to_check; j++) {
        if (diagnostic->faultBits & 1 << j) {
          mTRMCounter[drmID][trmID].Count(j + 4);
        }
      }
    }
  }
}

// Implement the Task
void TaskDiagnostics::initialize(o2::framework::InitContext& /*ctx*/)
{
  if (auto param = mCustomParameters.find("DecoderCONET"); param != mCustomParameters.end()) {
    if (param->second == "True") {
      LOG(INFO) << "Rig for DecoderCONET";
      mDecoderCounter.setDecoderCONET(kTRUE);
    }
  }

  mDRMHisto.reset(new TH2F("DRMCounter", "DRM Diagnostics;DRM Word;Crate;Words", 32, 0, 32, 72, 0, 72));
  mDecoderCounter.mDRMCounter[0].MakeHistogram(mDRMHisto.get());
  getObjectsManager()->startPublishing(mDRMHisto.get());
  mLTMHisto.reset(new TH2F("LTMCounter", "LTM Diagnostics;LTM Word;Crate;Words", 32, 0, 32, 72, 0, 72));
  mDecoderCounter.mLTMCounter[0].MakeHistogram(mLTMHisto.get());
  getObjectsManager()->startPublishing(mLTMHisto.get());
  for (int j = 0; j < DiagnosticsCounter::ntrms; j++) {
    mTRMHisto[j].reset(new TH2F(Form("TRMCounterSlot%i", j), Form("TRM %i Diagnostics;TRM Word;Crate;Words", j), 32, 0, 32, 72, 0, 72));
    mDecoderCounter.mTRMCounter[0][j].MakeHistogram(mTRMHisto[j].get());
    getObjectsManager()->startPublishing(mTRMHisto[j].get());
  }

} // namespace o2::quality_control_modules::tof

void TaskDiagnostics::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;
  reset();
}

void TaskDiagnostics::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void TaskDiagnostics::monitorData(o2::framework::ProcessingContext& ctx)
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
      mDecoderCounter.setDecoderBuffer(payloadIn);
      mDecoderCounter.setDecoderBufferSize(payloadInSize);
      mDecoderCounter.decode();
    }
  }
}

void TaskDiagnostics::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
  for (int i = 0; i < DiagnosticsCounter::ncrates; i++) { // Filling histograms only at the end of the cycle
    mDecoderCounter.mDRMCounter[i].FillHistogram(mDRMHisto.get(), i + 1);
    mDecoderCounter.mLTMCounter[i].FillHistogram(mLTMHisto.get(), i + 1);
    for (int j = 0; j < DiagnosticsCounter::ntrms; j++) {
      mDecoderCounter.mTRMCounter[i][j].FillHistogram(mTRMHisto[j].get(), i + 1);
    }
  }
}

void TaskDiagnostics::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void TaskDiagnostics::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  mDRMHisto->Reset();
  for (int j = 0; j < DiagnosticsCounter::ntrms; j++) {
    mTRMHisto[j]->Reset();
  }
}

} // namespace o2::quality_control_modules::tof
