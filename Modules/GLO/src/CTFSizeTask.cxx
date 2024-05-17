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
/// \file   CTFSizeTask.cxx
/// \author Ole Schmidt
///

#include <fmt/core.h>
#include <TH1F.h>
#include <TMath.h>
#include "QualityControl/QcInfoLogger.h"
#include "Common/Utils.h"
#include "GLO/CTFSizeTask.h"
#include <Framework/InputRecord.h>

using namespace o2::detectors;
using namespace o2::quality_control_modules::common;

namespace o2::quality_control_modules::glo
{

CTFSize::~CTFSize()
{
}

std::tuple<int, float, float> CTFSize::getBinningFromConfig(o2::detectors::DetID::ID iDet, const Activity& activity) const
{
  std::string cfKey = "binning";
  cfKey += DetID::getName(iDet);
  std::string binning = mCustomParameters.atOptional(cfKey, activity).value_or(mDefaultBinning[iDet]);
  auto nBins = std::stoi(binning.substr(0, binning.find(",")));
  binning.erase(0, binning.find(",") + 1);
  auto xMin = std::stof(binning.substr(0, binning.find(",")));
  binning.erase(0, binning.find(",") + 1);
  auto xMax = std::stof(binning.substr(0, binning.find(",")));
  return std::make_tuple(nBins, xMin, xMax);
}

void CTFSize::initialize(o2::framework::InitContext& /*ctx*/)
{
  // default lower limit from pp run 549884 and upper limit from PbPb run 543918, each limit with some margin
  mDefaultBinning[DetID::ITS] = "1000, 1e2, 1e5";
  mDefaultBinning[DetID::TPC] = "1000, 1e3, 1e6";
  mDefaultBinning[DetID::TRD] = "1000, 1e2, 1e5";
  mDefaultBinning[DetID::TOF] = "1000, 10, 1e4";
  mDefaultBinning[DetID::PHS] = "100, 10, 1e3";
  mDefaultBinning[DetID::CPV] = "100, 10, 3e4";
  mDefaultBinning[DetID::EMC] = "1000, 100, 5e5";
  mDefaultBinning[DetID::HMP] = "100, 1, 300";
  mDefaultBinning[DetID::MFT] = "1000, 1e2, 1e4";
  mDefaultBinning[DetID::MCH] = "100, 1e3, 5e4";
  mDefaultBinning[DetID::MID] = "100, 10, 500";
  mDefaultBinning[DetID::ZDC] = "100, 1e3, 1e4";
  mDefaultBinning[DetID::FT0] = "100, 1, 500";
  mDefaultBinning[DetID::FV0] = "100, 1, 400";
  mDefaultBinning[DetID::FDD] = "100, 1, 100";
  mDefaultBinning[DetID::CTP] = "100, 1, 100";
  mDefaultBinning[DetID::TST] = "1, 0, 1";

  std::string detList = getFromConfig<string>(mCustomParameters, "detectors", "all");
  auto detMask = DetID::getMask(detList);

  constexpr int nLogBins = 100;
  float xBins[nLogBins + 1];
  float xBinLogMin = 0.f;
  float xBinLogMax = 11.f;
  float logBinWidth = (xBinLogMax - xBinLogMin) / nLogBins;
  for (int iBin = 0; iBin <= nLogBins; ++iBin) {
    xBins[iBin] = TMath::Power(10, xBinLogMin + iBin * logBinWidth);
  }
  for (int iDet = 0; iDet < DetID::CTP + 1; ++iDet) {
    if (detMask[iDet]) {
      mIsDetEnabled[iDet] = true;
      mHistSizesLog[iDet] = new TH1F(Form("hSizeLog_%s", DetID::getName(iDet)), Form("%s CTF size per TF;Byte;counts", DetID::getName(iDet)), nLogBins, xBins);
      getObjectsManager()->startPublishing(mHistSizesLog[iDet]);
      getObjectsManager()->setDefaultDrawOptions(mHistSizesLog[iDet]->GetName(), "logx");
    }
  }
}

void CTFSize::startOfActivity(const Activity& activity)
{
  if (!mPublishingDone) {
    for (int iDet = 0; iDet < DetID::CTP + 1; ++iDet) {
      if (!mIsDetEnabled[iDet]) {
        continue;
      }
      auto binning = getBinningFromConfig(iDet, activity);
      std::string unit = (iDet == DetID::EMC || iDet == DetID::CPV) ? "B" : "kB";
      mHistSizes[iDet] = new TH1F(Form("hSize_%s", DetID::getName(iDet)), Form("%s CTF size per TF;%s;counts", DetID::getName(iDet), unit.c_str()), std::get<0>(binning), std::get<1>(binning), std::get<2>(binning));
      getObjectsManager()->startPublishing(mHistSizes[iDet]);
    }
    mPublishingDone = true;
  }
  reset();
}

void CTFSize::startOfCycle()
{
}

void CTFSize::monitorData(o2::framework::ProcessingContext& ctx)
{
  const auto sizes = ctx.inputs().get<std::array<size_t, DetID::CTP + 1>>("ctfSizes");
  for (int iDet = 0; iDet <= DetID::CTP; ++iDet) {
    ILOG(Debug, Devel) << fmt::format("Det {} : is enabled {}, data size {}", DetID::getName(iDet), mIsDetEnabled[iDet], sizes[iDet]) << ENDM;
    if (!mIsDetEnabled[iDet]) {
      continue;
    }
    float conversion = (iDet == DetID::EMC || iDet == DetID::CPV) ? 1.f : 1024.f; // EMC and CPV can sent <1kB per CTF
    mHistSizes[iDet]->Fill(sizes[iDet] / conversion);
    mHistSizesLog[iDet]->Fill(sizes[iDet]);
  }
}

void CTFSize::endOfCycle()
{
}

void CTFSize::endOfActivity(const Activity& /*activity*/)
{
}

void CTFSize::reset()
{
  for (auto h : mHistSizes) {
    if (h) {
      h->Reset();
    }
  }
  for (auto h : mHistSizesLog) {
    if (h) {
      h->Reset();
    }
  }
}

} // namespace o2::quality_control_modules::glo
