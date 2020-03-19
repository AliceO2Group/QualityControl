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

// O2 includes
#include "DataFormatsTOF/CompressedDataFormat.h"
#include <Framework/DataRefUtils.h>

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TOF/TOFTaskCompressedCounter.h"

namespace o2::quality_control_modules::tof
{

TOFTaskCompressedCounter::TOFTaskCompressedCounter() : TaskInterface(),
                                                       mDRMCounterHisto{ nullptr },
                                                       mCounter()
{
}

TOFTaskCompressedCounter::~TOFTaskCompressedCounter()
{
  for (Int_t i = 0; i < Diagnostics::ncrates; i++) {
    mDRMCounterHisto[i].reset();
  }
}

void TOFTaskCompressedCounter::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info) << "initialize TOFTaskCompressedCounter" << ENDM;
  for (Int_t i = 0; i < Diagnostics::ncrates; i++) {
    // WARNING! Here the histograms have to be larger than the counter size, otherwise in memory they will be badly handled with undefined behaviour. I.e. put more bins than necessary, they will be trimmed out later.
    mDRMCounterHisto[i].reset(new TH1F(Form("DRMCounterCrate%i", i), ";DRM Word;Words", 32, 0, 32));
    mCounter.mDRMCounter[i].MakeHistogram(mDRMCounterHisto[i].get());
    getObjectsManager()->startPublishing(mDRMCounterHisto[i].get());
  }
}

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
  for (Int_t i = 0; i < Diagnostics::ncrates; i++) {
    mCounter.mDRMCounter[i].FillHistogram(mDRMCounterHisto[i].get());
  }
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
  for (Int_t i = 0; i < Diagnostics::ncrates; i++) {
    mDRMCounterHisto[i]->Reset();
  }
}

} // namespace o2::quality_control_modules::tof
