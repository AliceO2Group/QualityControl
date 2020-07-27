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
/// \file   TOFTaskCompressed.cxx
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
#include "TOF/TOFTaskCompressed.h"

namespace o2::quality_control_modules::tof
{

TOFTaskCompressed::TOFTaskCompressed() : TaskInterface(),
                                         mDecoder(),
                                         mHits(nullptr),
                                         mTime(nullptr),
                                         mTimeBC(nullptr),
                                         mTOT(nullptr),
                                         mIndexE(nullptr),
                                         mSlotPartMask(nullptr),
                                         mDiagnostic(nullptr)
{
}

TOFTaskCompressed::~TOFTaskCompressed()
{
  mHits.reset();
  mTime.reset();
  mTimeBC.reset();
  mTOT.reset();
  mIndexE.reset();
  mSlotPartMask.reset();
  mDiagnostic.reset();
}

void TOFTaskCompressed::initialize(o2::framework::InitContext& /*ctx*/)
{
  LOG(INFO) << "initialize TOFTaskCompressed";
  if (auto param = mCustomParameters.find("DecoderCONET"); param != mCustomParameters.end()) {
    if (param->second == "True") {
      LOG(INFO) << "Rig for DecoderCONET";
      mDecoder.setDecoderCONET(kTRUE);
    }
  }

  mHits.reset(new TH1F("hHits", "hHits;Number of hits", 1000, 0., 1000.));
  getObjectsManager()->startPublishing(mHits.get());
  mDecoder.mHistos[mHits->GetName()] = mHits;
  //
  mTime.reset(new TH1F("hTime", "hTime;time (24.4 ps)", 2097152, 0., 2097152.));
  getObjectsManager()->startPublishing(mTime.get());
  mDecoder.mHistos[mTime->GetName()] = mTime;
  //
  mTimeBC.reset(new TH1F("hTimeBC", "hTimeBC;time (24.4 ps)", 1024, 0., 1024.));
  getObjectsManager()->startPublishing(mTimeBC.get());
  mDecoder.mHistos[mTimeBC->GetName()] = mTimeBC;
  //
  mTOT.reset(new TH1F("hTOT", "hTOT;ToT (48.8 ps)", 2048, 0., 2048.));
  getObjectsManager()->startPublishing(mTOT.get());
  mDecoder.mHistos[mTOT->GetName()] = mTOT;
  //
  mIndexE.reset(new TH1F("hIndexE", "hIndexE;index EO", 172800, 0., 172800.));
  getObjectsManager()->startPublishing(mIndexE.get());
  mDecoder.mHistos[mIndexE->GetName()] = mIndexE;
  //
  mSlotPartMask.reset(new TH2F("hSlotPartMask", "hSlotPartMask;crate;slot", 72, 0., 72., 12, 1., 13.));
  getObjectsManager()->startPublishing(mSlotPartMask.get());
  mDecoder.mHistos[mSlotPartMask->GetName()] = mSlotPartMask;
  //
  mDiagnostic.reset(new TH2F("hDiagnostic", "hDiagnostic;crate;slot", 72, 0., 72., 12, 1., 13.));
  getObjectsManager()->startPublishing(mDiagnostic.get());
  mDecoder.mHistos[mDiagnostic->GetName()] = mDiagnostic;
  //
}

void TOFTaskCompressed::startOfActivity(Activity& /*activity*/)
{
  LOG(INFO) << "startOfActivity";
  mHits->Reset();
  mTime->Reset();
  mTimeBC->Reset();
  mTOT->Reset();
  mIndexE->Reset();
  mSlotPartMask->Reset();
  mDiagnostic->Reset();
}

void TOFTaskCompressed::startOfCycle()
{
  LOG(INFO) << "startOfCycle";
}

void TOFTaskCompressed::monitorData(o2::framework::ProcessingContext& ctx)
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

void TOFTaskCompressed::endOfCycle()
{
  LOG(INFO) << "endOfCycle";
}

void TOFTaskCompressed::endOfActivity(Activity& /*activity*/)
{
  LOG(INFO) << "endOfActivity";
}

void TOFTaskCompressed::reset()
{
  // clean all the monitor objects here

  LOG(INFO) << "Resetting the histogram";
  mHits->Reset();
  mTime->Reset();
  mTOT->Reset();
  mIndexE->Reset();
  mSlotPartMask->Reset();
  mDiagnostic->Reset();
}

} // namespace o2::quality_control_modules::tof
