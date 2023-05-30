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
/// \file   RofsTask.cxx
/// \author Sebastien Perrin
///

#include "MCH/RofsTask.h"

#include <TFile.h>
#include <TMath.h>

#include "DetectorsRaw/RDHUtils.h"
#include "QualityControl/QcInfoLogger.h"
#include "Framework/WorkflowSpec.h"
#include "Framework/DataRefUtils.h"
#include "CommonConstants/LHCConstants.h"

#include "MCHRawElecMap/Mapper.h"
#include "MCHMappingInterface/Segmentation.h"
#include "MCHRawDecoder/PageDecoder.h"
#include "MCHRawDecoder/ErrorCodes.h"
#include "MCHBase/DecoderError.h"

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

RofsTask::RofsTask()
{
  mIsSignalDigit = o2::mch::createDigitFilter(20, true, true);
}

RofsTask::~RofsTask() = default;

void RofsTask::initialize(o2::framework::InitContext& /*ic*/)
{
  ILOG(Debug, Devel) << "initialize RofsTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

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
  mHistRofSize_Signal = std::make_shared<TH1F>("RofSize_Signal", "ROF size (signal-like digits)", nLogBins, xbins);
  publishObject(mHistRofSize_Signal, "hist", true);

  // ROF width distributions
  mHistRofWidth = std::make_shared<TH1F>("RofWidth", "ROF width", 2000 / 25, 0, 2000 / 25);
  publishObject(mHistRofWidth, "hist", true);

  // Number of stations per ROF
  mHistRofNStations = std::make_shared<TH1F>("RofNStations", "Number of stations per ROF", 6, 0, 6);
  publishObject(mHistRofNStations, "hist", true);

  // Number of stations per ROF (signal-like digits)
  mHistRofNStations_Signal = std::make_shared<TH1F>("RofNStations_Signal", "Number of stations per ROF (signal-like digits)", 6, 0, 6);
  publishObject(mHistRofNStations_Signal, "hist", true);

  auto bcInOrbit = o2::constants::lhc::LHCMaxBunches;
  // ROF time distribution in orbit
  mHistRofTime = std::make_shared<TH1F>("RofTime", "ROF time in orbit", bcInOrbit, 0, bcInOrbit);
  publishObject(mHistRofTime, "hist", false);

  // ROF time distribution in orbit (signal-like digits)
  mHistRofTime_Signal = std::make_shared<TH1F>("RofTime_Signal", "ROF time in orbit (signal-like digits)", bcInOrbit, 0, bcInOrbit);
  publishObject(mHistRofTime_Signal, "hist", false);
}

void RofsTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity : " << activity.mId << ENDM;
}

void RofsTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void RofsTask::plotROF(const o2::mch::ROFRecord& rof, gsl::span<const o2::mch::Digit> digits)
{
  const auto bcInOrbit = o2::constants::lhc::LHCMaxBunches;
  auto rofDigits = digits.subspan(rof.getFirstIdx(), rof.getNEntries());
  auto start = rofDigits.front().getTime();
  auto end = rofDigits.back().getTime();
  mHistRofWidth->Fill(end - start + 1);

  // number of signal-like digits in ROF
  int nSignal = 0;

  // average ROF time
  double rofTime = 0;
  double rofTimeSignal = 0;

  // fired stations
  std::array<bool, 5> stations{ false };
  std::array<bool, 5> stationsSignal{ false };

  for (auto& digit : rofDigits) {
    int station = (digit.getDetID() - 100) / 200;
    if (station < 0 || station >= 5) {
      continue;
    }
    stations[station] = true;

    rofTime += digit.getTime() % bcInOrbit;

    if (mIsSignalDigit(digit)) {
      nSignal += 1;
      rofTimeSignal += digit.getTime() % bcInOrbit;
      stationsSignal[station] = true;
    }
  }
  mHistRofSize->Fill(rof.getNEntries());
  mHistRofSize_Signal->Fill(nSignal);

  mHistRofTime->Fill(rofTime / rofDigits.size());
  if (nSignal > 0) {
    mHistRofTime_Signal->Fill(rofTimeSignal / nSignal);
  }

  mHistRofNStations->Fill(std::count(stations.cbegin(), stations.cend(), true));
  mHistRofNStations_Signal->Fill(std::count(stationsSignal.cbegin(), stationsSignal.cend(), true));
}

void RofsTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  auto digits = ctx.inputs().get<gsl::span<o2::mch::Digit>>("digits");
  auto rofs = ctx.inputs().get<gsl::span<o2::mch::ROFRecord>>("rofs");

  for (auto& rof : rofs) {
    plotROF(rof, digits);
  }
}

void RofsTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void RofsTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void RofsTask::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Devel) << "Resetting the histogramss" << ENDM;

  for (auto h : mAllHistograms) {
    h->Reset("ICES");
  }
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
