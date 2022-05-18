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
/// \file   PhysicsTaskRofs.cxx
/// \author Sebastien Perrin
///

#include "MCH/PhysicsTaskRofs.h"

#include <TFile.h>
#include <TMath.h>

#include "DetectorsRaw/RDHUtils.h"
#include "QualityControl/QcInfoLogger.h"
#include "Framework/WorkflowSpec.h"
#include "Framework/DataRefUtils.h"

#include "MCHRawElecMap/Mapper.h"
#include "MCHMappingInterface/Segmentation.h"
#include "MCHRawDecoder/PageDecoder.h"
#include "MCHRawDecoder/ErrorCodes.h"
#include "MCHBase/DecoderError.h"

#define publishObject(object, drawOption, statBox)        \
  {                                                       \
    object->SetOption(drawOption);                        \
    if (!statBox) {                                       \
      object->SetStats(0);                                \
    }                                                     \
    mAllHistograms.push_back(object.get());               \
    if (!mSaveToRootFile) {                               \
      getObjectsManager()->startPublishing(object.get()); \
    }                                                     \
  }

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

PhysicsTaskRofs::PhysicsTaskRofs()
{
  mIsSignalDigit = o2::mch::createDigitFilter(20, true, true);
}

PhysicsTaskRofs::~PhysicsTaskRofs() = default;

void PhysicsTaskRofs::initialize(o2::framework::InitContext& /*ic*/)
{
  ILOG(Info, Support) << "initialize PhysicsTaskRofs" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  mSaveToRootFile = false;
  if (auto param = mCustomParameters.find("SaveToRootFile"); param != mCustomParameters.end()) {
    if (param->second == "true" || param->second == "True" || param->second == "TRUE") {
      mSaveToRootFile = true;
    }
  }

  static constexpr int nLogBins = 100;
  Float_t xbins[nLogBins + 1];
  Float_t logMin = 0;
  Float_t logMax = 6;
  Float_t dLog = (logMax - logMin) / nLogBins;
  for (int b = 0; b <= nLogBins; b++) {
    xbins[b] = TMath::Power(10, dLog * b);
  }

  // ROF size distributions
  mHistRofSize = std::make_shared<TH1F>("RofSize", "ROF size", nLogBins, xbins);
  publishObject(mHistRofSize, "hist", true);

  // ROF size distributions (signal-like digits only)
  mHistRofSizeSignal = std::make_shared<TH1F>("RofSizeSignal", "ROF size (signal-like digits)", nLogBins, xbins);
  publishObject(mHistRofSizeSignal, "hist", true);

  // ROF width distributions
  mHistRofWidth = std::make_shared<TH1F>("RofWidth", "ROF width", 5000 / 25, 0, 5000 / 25);
  publishObject(mHistRofWidth, "hist", true);

  // Number of stations per ROF
  mHistRofNStations = std::make_shared<TH1F>("RofNStations", "Number of stations per ROF", 7, 0, 7);
  publishObject(mHistRofNStations, "hist", true);
}

void PhysicsTaskRofs::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity : " << activity.mId << ENDM;
}

void PhysicsTaskRofs::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void PhysicsTaskRofs::plotROF(const o2::mch::ROFRecord& rof, gsl::span<const o2::mch::Digit> digits)
{
  std::array<bool, 10> stations{ false };
  mHistRofSize->Fill(rof.getNEntries());

  auto rofDigits = digits.subspan(rof.getFirstIdx(), rof.getNEntries());
  auto start = rofDigits.front().getTime();
  auto end = rofDigits.back().getTime();
  mHistRofWidth->Fill(end - start + 1);

  int nSignal = 0;
  for (auto& digit : rofDigits) {
    int station = (digit.getDetID() - 100) / 200;
    if (station < 0 | station >= 10) {
      continue;
    }
    stations[station] = true;

    if (mIsSignalDigit(digit)) {
      nSignal += 1;
    }
  }
  mHistRofSizeSignal->Fill(nSignal);

  int nStations = 0;
  for (auto s : stations) {
    if (s) {
      nStations += 1;
    }
  }
  mHistRofNStations->Fill(nStations);
}

void PhysicsTaskRofs::monitorData(o2::framework::ProcessingContext& ctx)
{
  auto digits = ctx.inputs().get<gsl::span<o2::mch::Digit>>("digits");
  auto rofs = ctx.inputs().get<gsl::span<o2::mch::ROFRecord>>("rofs");

  for (auto& rof : rofs) {
    plotROF(rof, digits);
  }
}

void PhysicsTaskRofs::writeHistos()
{
  TFile f("mch-qc-rofs.root", "RECREATE");
  for (auto h : mAllHistograms) {
    h->Write();
  }
  f.Close();
}

void PhysicsTaskRofs::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;

  if (mSaveToRootFile) {
    writeHistos();
  }
}

void PhysicsTaskRofs::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;

  if (mSaveToRootFile) {
    writeHistos();
  }
}

void PhysicsTaskRofs::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histograms" << ENDM;

  for (auto h : mAllHistograms) {
    h->Reset();
  }
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
