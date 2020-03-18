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
#include <TCanvas.h>
#include <TH1.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TH1I.h>
#include <TH2I.h>

// O2 includes
#include "DataFormatsTOF/CompressedDataFormat.h"
#include <Framework/DataRefUtils.h>

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TOF/TOFTaskCompressedCounter.h"

namespace o2::quality_control_modules::tof
{

TOFTaskCompressedCounter::TOFTaskCompressedCounter() : TaskInterface(),
                                         mCounter(),
                                         mRDHCounterCrate0(nullptr)
{
}

TOFTaskCompressedCounter::~TOFTaskCompressedCounter()
{
  mRDHCounterCrate0.reset();
}


void TOFTaskCompressedCounter::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info) << "initialize TOFTaskCompressedCounter" << ENDM;

  mRDHCounterCrate0.reset(new TH1F("RDHCounterCrate0", ";RDH Word;Words", 1, 0, 1));
  mCounter.mRDHCounter[0].MakeHistogram(mRDHCounterCrate0.get());
  getObjectsManager()->startPublishing(mRDHCounterCrate0.get());
}

void TOFTaskCompressedCounter::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "startOfActivity" << ENDM;
  mRDHCounterCrate0->Reset();
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
  // mCounter.mRDHCounter[0].FillHistogram(mRDHCounterCrate0.get());

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
  mRDHCounterCrate0->Reset();
}

} // namespace o2::quality_control_modules::tof
