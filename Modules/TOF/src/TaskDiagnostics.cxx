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
/// \file   TOFTaskCompressedCounter.cxx
/// \author Nicolo' Jacazio
///

// ROOT includes
#include "TH1F.h"
#include "TH2F.h"

// O2 includes
#include "DataFormatsTOF/CompressedDataFormat.h"
#include <Framework/DataRefUtils.h>

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TOF/TOFTaskCompressedCounter.h"

namespace o2::quality_control_modules::tof
{

void TOFTaskCompressedCounter::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info) << "initialize TOFTaskCompressedCounter" << ENDM;
#ifdef ENABLE_2D_HISTOGRAMS
  // WARNING X axis is reserved to the counter
  // WARNING! Here the histograms have to be larger than the counter size, otherwise in memory they will be badly handled with undefined behaviour. I.e. put more bins than necessary, they will be trimmed out later.
  mDRMCounterHisto.reset(new TH2F("DRMCounter", ";DRM Word;Crate;Words", 32, 0, 32, 72, 0, 72));
  mCounter.mDRMCounter[0].MakeHistogram(mDRMCounterHisto.get());
  getObjectsManager()->startPublishing(mDRMCounterHisto.get());
  for (Int_t j = 0; j < Diagnostics::ntrms; j++) {
    mTRMCounterHisto[j].reset(new TH2F(Form("TRMCounterSlot%i", j), ";TRM Word;Crate;Words", 32, 0, 32, 72, 0, 72));
    mCounter.mTRMCounter[0][j].MakeHistogram(mTRMCounterHisto[j].get());
    getObjectsManager()->startPublishing(mTRMCounterHisto[j].get());
    for (Int_t k = 0; k < Diagnostics::ntrmschains; k++) {
      mTRMChainCounterHisto[j][k].reset(new TH2F(Form("TRMChainCounterSlot%iChain%i", j, k), ";TRMChain Word;Crate;Words", 32, 0, 32, 72, 0, 72));
      mCounter.mTRMChainCounter[0][j][k].MakeHistogram(mTRMChainCounterHisto[j][k].get());
      getObjectsManager()->startPublishing(mTRMChainCounterHisto[j][k].get());
    }
  }
#else
  for (Int_t i = 0; i < Diagnostics::ncrates; i++) {
    // WARNING! Here the histograms have to be larger than the counter size, otherwise in memory they will be badly handled with undefined behaviour. I.e. put more bins than necessary, they will be trimmed out later.
    mDRMCounterHisto[i].reset(new TH1F(Form("DRMCounterCrate%i", i), ";DRM Word;Words", 32, 0, 32));
    mCounter.mDRMCounter[i].MakeHistogram(mDRMCounterHisto[i].get());
    getObjectsManager()->startPublishing(mDRMCounterHisto[i].get());
    for (Int_t j = 0; j < Diagnostics::ntrms; j++) {
      mTRMCounterHisto[i][j].reset(new TH1F(Form("TRMCounterCrate%iSlot%i", i, j), ";TRM Word;Words", 32, 0, 32));
      mCounter.mTRMCounter[i][j].MakeHistogram(mTRMCounterHisto[i][j].get());
      getObjectsManager()->startPublishing(mTRMCounterHisto[i][j].get());
      for (Int_t k = 0; k < Diagnostics::ntrmschains; k++) {
        mTRMChainCounterHisto[i][j][k].reset(new TH1F(Form("TRMChainCounterCrate%iSlot%iChain%i", i, j, k), ";TRMChain Word;Words", 32, 0, 32));
        mCounter.mTRMChainCounter[i][j][k].MakeHistogram(mTRMChainCounterHisto[i][j][k].get());
        getObjectsManager()->startPublishing(mTRMChainCounterHisto[i][j][k].get());
      }
    }
  }
#endif
} // namespace o2::quality_control_modules::tof

void TOFTaskCompressedCounter::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "startOfActivity" << ENDM;
  reset();
}

void TOFTaskCompressedCounter::startOfCycle()
{
  ILOG(Info) << "startOfCycle" << ENDM;
}

void TOFTaskCompressedCounter::monitorData(o2::framework::ProcessingContext& ctx)
{
  LOG(INFO) << "Monitoring in the TOFTaskCompressedCounter";

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
#ifdef ENABLE_2D_HISTOGRAMS
  for (Int_t i = 0; i < Diagnostics::ncrates; i++) {
    mCounter.mDRMCounter[i].FillHistogram(mDRMCounterHisto.get(), i + 1);
    for (Int_t j = 0; j < Diagnostics::ntrms; j++) {
      mCounter.mTRMCounter[i][j].FillHistogram(mTRMCounterHisto[j].get(), i + 1);
      for (Int_t k = 0; k < Diagnostics::ntrmschains; k++) {
        mCounter.mTRMChainCounter[i][j][k].FillHistogram(mTRMChainCounterHisto[j][k].get(), i + 1);
      }
    }
  }
#else
  for (Int_t i = 0; i < Diagnostics::ncrates; i++) {
    mCounter.mDRMCounter[i].FillHistogram(mDRMCounterHisto[i].get());
    for (Int_t j = 0; j < Diagnostics::ntrms; j++) {
      mCounter.mTRMCounter[i][j].FillHistogram(mTRMCounterHisto[i][j].get());
      for (Int_t k = 0; k < Diagnostics::ntrmschains; k++) {
        mCounter.mTRMChainCounter[i][j][k].FillHistogram(mTRMChainCounterHisto[i][j][k].get());
      }
    }
  }
#endif
}

void TOFTaskCompressedCounter::endOfCycle()
{
  ILOG(Info) << "endOfCycle" << ENDM;
}

void TOFTaskCompressedCounter::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "endOfActivity" << ENDM;
}

void TOFTaskCompressedCounter::reset()
{
  // clean all the monitor objects here

  ILOG(Info) << "Resetting the histogram" << ENDM;
#ifdef ENABLE_2D_HISTOGRAMS
  mDRMCounterHisto->Reset();
  for (Int_t j = 0; j < Diagnostics::ntrms; j++) {
    mTRMCounterHisto[j]->Reset();
    for (Int_t k = 0; k < Diagnostics::ntrmschains; k++) {
      mTRMChainCounterHisto[j][k]->Reset();
    }
  }
#else
  for (Int_t i = 0; i < Diagnostics::ncrates; i++) {
    mDRMCounterHisto[i]->Reset();
    for (Int_t j = 0; j < Diagnostics::ntrms; j++) {
      mTRMCounterHisto[i][j]->Reset();
      for (Int_t k = 0; k < Diagnostics::ntrmschains; k++) {
        mTRMChainCounterHisto[i][j][k]->Reset();
      }
    }
  }
#endif
}

} // namespace o2::quality_control_modules::tof
