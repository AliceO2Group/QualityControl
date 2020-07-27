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

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TOF/TaskDiagnostics.h"

namespace o2::quality_control_modules::tof
{

void TaskDiagnostics::initialize(o2::framework::InitContext& /*ctx*/)
{
  if (auto param = mCustomParameters.find("DecoderCONET"); param != mCustomParameters.end()) {
    if (param->second == "True") {
      LOG(INFO) << "Rig for DecoderCONET";
      mCounter.setDecoderCONET(kTRUE);
    }
  }

  mDRMCounterHisto.reset(new TH2F("DRMCounter", "DRM Diagnostics;DRM Word;Crate;Words", 32, 0, 32, 72, 0, 72));
  mCounter.mDRMCounter[0].MakeHistogram(mDRMCounterHisto.get());
  getObjectsManager()->startPublishing(mDRMCounterHisto.get());
  mLTMCounterHisto.reset(new TH2F("LTMCounter", "LTM Diagnostics;LTM Word;Crate;Words", 32, 0, 32, 72, 0, 72));
  mCounter.mLTMCounter[0].MakeHistogram(mLTMCounterHisto.get());
  getObjectsManager()->startPublishing(mLTMCounterHisto.get());
  for (Int_t j = 0; j < Diagnostics::ntrms; j++) {
    mTRMCounterHisto[j].reset(new TH2F(Form("TRMCounterSlot%i", j), Form("TRM %j Diagnostics;TRM Word;Crate;Words"), 32, 0, 32, 72, 0, 72));
    mCounter.mTRMCounter[0][j].MakeHistogram(mTRMCounterHisto[j].get());
    getObjectsManager()->startPublishing(mTRMCounterHisto[j].get());
    for (Int_t k = 0; k < Diagnostics::ntrmschains; k++) {
      mTRMChainCounterHisto[j][k].reset(new TH2F(Form("TRMChainCounterSlot%iChain%i", j, k), Form("TRM %i Chain %i Diagnostics;TRMChain Word;Crate;Words", j, k), 32, 0, 32, 72, 0, 72));
      mCounter.mTRMChainCounter[0][j][k].MakeHistogram(mTRMChainCounterHisto[j][k].get());
      getObjectsManager()->startPublishing(mTRMChainCounterHisto[j][k].get());
    }
  }

} // namespace o2::quality_control_modules::tof

void TaskDiagnostics::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "startOfActivity" << ENDM;
  reset();
}

void TaskDiagnostics::startOfCycle()
{
  ILOG(Info) << "startOfCycle" << ENDM;
}

void TaskDiagnostics::monitorData(o2::framework::ProcessingContext& ctx)
{
  /** receive input **/
  for (auto& input : ctx.inputs()) {
    /** input **/
    const auto* headerIn = o2::framework::DataRefUtils::getHeader<o2::header::DataHeader*>(input);
    auto payloadIn = input.payload;
    auto payloadInSize = headerIn->payloadSize;
    mCounter.setDecoderBuffer(payloadIn);
    mCounter.setDecoderBufferSize(payloadInSize);
    mCounter.decode();
  }
}

void TaskDiagnostics::endOfCycle()
{
  ILOG(Info) << "endOfCycle" << ENDM;
  for (Int_t i = 0; i < Diagnostics::ncrates; i++) { // Filling histograms only at the end of the cycle
    mCounter.mDRMCounter[i].FillHistogram(mDRMCounterHisto.get(), i + 1);
    mCounter.mLTMCounter[i].FillHistogram(mLTMCounterHisto.get(), i + 1);
    for (Int_t j = 0; j < Diagnostics::ntrms; j++) {
      mCounter.mTRMCounter[i][j].FillHistogram(mTRMCounterHisto[j].get(), i + 1);
      for (Int_t k = 0; k < Diagnostics::ntrmschains; k++) {
        mCounter.mTRMChainCounter[i][j][k].FillHistogram(mTRMChainCounterHisto[j][k].get(), i + 1);
      }
    }
  }
}

void TaskDiagnostics::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "endOfActivity" << ENDM;
}

void TaskDiagnostics::reset()
{
  // clean all the monitor objects here

  ILOG(Info) << "Resetting the histogram" << ENDM;
  mDRMCounterHisto->Reset();
  for (Int_t j = 0; j < Diagnostics::ntrms; j++) {
    mTRMCounterHisto[j]->Reset();
    for (Int_t k = 0; k < Diagnostics::ntrmschains; k++) {
      mTRMChainCounterHisto[j][k]->Reset();
    }
  }
}

} // namespace o2::quality_control_modules::tof
