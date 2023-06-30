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
/// \file   EveryObject.cxx
/// \author Piotr Konopka
///

#include <TCanvas.h>
#include <TH1.h>

#include "QualityControl/QcInfoLogger.h"
#include "Example/EveryObject.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/DataRefUtils.h>

#include <TH1F.h>
#include <TH2F.h>
#include <TH3F.h>
#include <THnSparse.h>
#include <TCanvas.h>

constexpr Double_t rangeLimiter = 16 * 64000;

namespace o2::quality_control_modules::example
{

EveryObject::~EveryObject()
{
  delete mTH1F;
  delete mTH2F;
  delete mTH3F;
  delete mTHnSparseF;
  delete mTCanvas; // TCanvas should delete the contained plots, given that we set kCanDelete
}

void EveryObject::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize EveryObject" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // todo: enable/disable with config flags (this is why objects are always accessed after checking if not null)
  // todo: ttree, tefficiency, tprofile

  mTH1F = new TH1F("th1f", "th1f", 64000, 0, rangeLimiter);
  getObjectsManager()->startPublishing(mTH1F);

  mTH2F = new TH2F("th2f", "th2f", 250, 0, rangeLimiter, 250, 0, rangeLimiter);
  getObjectsManager()->startPublishing(mTH2F);

  mTH3F = new TH3F("th3f", "th3f", 40, 0, rangeLimiter, 40, 0, rangeLimiter, 40, 0, rangeLimiter);
  getObjectsManager()->startPublishing(mTH3F);
  {
    const size_t bins = 1000;
    const size_t dim = 5;
    const Double_t min = 0.0;
    const Double_t max = rangeLimiter;

    const std::vector<Int_t> binsDims(dim, bins);
    const std::vector<Double_t> mins(dim, min);
    const std::vector<Double_t> maxs(dim, max);
    mTHnSparseF = new THnSparseF("thnsparsef", "thnsparsef", dim, binsDims.data(), mins.data(), maxs.data());
    getObjectsManager()->startPublishing(mTHnSparseF);
  }
  {
    mTCanvas = new TCanvas("tcanvas", "tcanvas", 1000, 1000);
    mTCanvas->Clear();
    mTCanvas->Divide(2, 2);
    for (size_t i = 0; i < 4; i++) {
      auto name = std::string("tcanvas_th2f_") + std::to_string(i);
      mTCanvasMembers[i] = new TH2F(name.c_str(), name.c_str(), 250, 0, rangeLimiter, 250, 0, rangeLimiter);

      mTCanvas->cd(i + 1);
      mTCanvasMembers[i]->Draw();
      mTCanvasMembers[i]->SetBit(TObject::kCanDelete);
    }
    getObjectsManager()->startPublishing(mTCanvas);
  }
}

void EveryObject::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
}

void EveryObject::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void EveryObject::monitorData(o2::framework::ProcessingContext& ctx)
{
  for (auto&& input : framework::InputRecordWalker(ctx.inputs())) {
    const auto* header = framework::DataRefUtils::getHeader<header::DataHeader*>(input);
    const auto payloadSize = framework::DataRefUtils::getPayloadSize(input);
    auto value1 = (Float_t)(payloadSize % (size_t)rangeLimiter);
    auto value2 = (Float_t)((header->tfCounter + payloadSize) % (size_t)rangeLimiter);
    auto value3 = (Float_t)((header->tfCounter * payloadSize) % (size_t)rangeLimiter);

    if (mTH1F) {
      mTH1F->Fill(value1);
    }
    if (mTH2F) {
      mTH2F->Fill(value1, value3);
    }
    if (mTH3F) {
      mTH3F->Fill(value1, value2, value3);
    }
    if (mTHnSparseF) {
      std::array<Double_t, 5> values{ value1, value2, value3, value2 / (value1 + 1), value2 / (value3 + 1) };
      mTHnSparseF->Fill(values.data());
    }
    if (mTCanvas) {
      mTCanvasMembers[0]->Fill(value1, value3);
      mTCanvasMembers[1]->Fill(value3, value1);
      mTCanvasMembers[2]->Fill(value2, value3);
      mTCanvasMembers[3]->Fill(value3, value2);
    }
  }
}

void EveryObject::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void EveryObject::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void EveryObject::reset()
{
  ILOG(Debug, Devel) << "Resetting the objects" << ENDM;
  if (mTH1F) {
    mTH1F->Reset();
  }
  if (mTH2F) {
    mTH2F->Reset();
  }
  if (mTH3F) {
    mTH3F->Reset();
  }
  if (mTHnSparseF) {
    mTHnSparseF->Reset();
  }
  if (mTCanvas) {
    for (const auto member : mTCanvasMembers) {
      if (member) {
        member->Reset();
      }
    }
  }
}

} // namespace o2::quality_control_modules::example
