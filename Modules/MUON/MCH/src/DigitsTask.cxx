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
/// \file   DigitsTask.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
/// \author Andrea Ferrero
/// \author Sebastien Perrin
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TGraph.h>
#include <TFile.h>
#include <algorithm>

#include "MCH/DigitsTask.h"
#include "MCHMappingInterface/Segmentation.h"
#include "MCHRawDecoder/DataDecoder.h"
#include "QualityControl/QcInfoLogger.h"
#include "DetectorsBase/GRPGeomHelper.h"
#include <Framework/InputRecord.h>
#include <CommonConstants/LHCConstants.h>
#include <DetectorsRaw/HBFUtils.h>
#include "MCHConstants/DetectionElements.h"
#include "MCHGlobalMapping/DsIndex.h"

using namespace std;
using namespace o2::mch;
using namespace o2::mch::raw;

static int nCycles = 0;

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

DigitsTask::DigitsTask() : TaskInterface()
{
  mIsSignalDigit = o2::mch::createDigitFilter(20, true, true);
}

DigitsTask::~DigitsTask() {}

void DigitsTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize DigitsTask" << AliceO2::InfoLogger::InfoLogger::endm;

  // flag to enable extra disagnostics plots; it also enables on-cycle plots
  mDiagnostic = false;
  if (auto param = mCustomParameters.find("Diagnostic"); param != mCustomParameters.end()) {
    if (param->second == "true" || param->second == "True" || param->second == "TRUE") {
      mDiagnostic = true;
    }
  }

  mElec2DetMapper = createElec2DetMapper<ElectronicMapperGenerated>();
  mFeeLink2SolarMapper = createFeeLink2SolarMapper<ElectronicMapperGenerated>();

  const uint32_t nElecXbins = NumberOfDualSampas;

  resetOrbits();

  // Histograms in electronics coordinates
  mHistogramOccupancyElec = std::make_unique<MergeableTH2Ratio>("Occupancy_Elec", "Occupancy", nElecXbins, 0, nElecXbins, 64, 0, 64);
  publishObject(mHistogramOccupancyElec.get(), "colz", false, false);

  mHistogramSignalOccupancyElec = std::make_unique<MergeableTH2Ratio>("OccupancySignal_Elec", "Occupancy (signal)", nElecXbins, 0, nElecXbins, 64, 0, 64);
  publishObject(mHistogramSignalOccupancyElec.get(), "colz", false, false);

  mHistogramDigitsOrbitElec = std::make_unique<TH2F>("DigitOrbit_Elec", "Digit orbits vs DS Id", nElecXbins, 0, nElecXbins, 768, -384, 384);
  publishObject(mHistogramDigitsOrbitElec.get(), "colz", true, false);

  mHistogramDigitsSignalOrbitElec = std::make_unique<TH2F>("DigitSignalOrbit_Elec", "Digit orbits vs DS Id (signal)", nElecXbins, 0, nElecXbins, 768, -384, 384);
  publishObject(mHistogramDigitsSignalOrbitElec.get(), "colz", true, false);

  mHistogramDigitsBcInOrbit = std::make_unique<TH2F>("Expert/DigitsBcInOrbit_Elec", "Digit BC vs DS Id", nElecXbins, 0, nElecXbins, 3600, 0, 3600);
  publishObject(mHistogramDigitsBcInOrbit.get(), "colz", false, true);

  mHistogramAmplitudeVsSamples = std::make_unique<TH2F>("Expert/AmplitudeVsSamples", "Digit amplitude vs nsamples", 1000, 0, 1000, 1000, 0, 10000);
  publishObject(mHistogramAmplitudeVsSamples.get(), "colz", false, true);

  // Histograms in detector coordinates
  for (auto de : o2::mch::constants::deIdsForAllMCH) {
    auto h = std::make_unique<TH1F>(TString::Format("Expert/%sADCamplitude_DE%03d", getHistoPath(de).c_str(), de),
                                    TString::Format("ADC amplitude (DE%03d)", de), 5000, 0, 5000);
    publishObject(h.get(), "hist", false, true);
    mHistogramADCamplitudeDE.emplace(de, std::move(h));
  }
}

void DigitsTask::startOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void DigitsTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

static bool checkInput(o2::framework::ProcessingContext& ctx, std::string binding)
{
  bool result = false;
  for (auto&& input : ctx.inputs()) {
    if (input.spec->binding == binding) {
      result = true;
      break;
    }
  }
  return result;
}

void DigitsTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  bool hasOrbits = checkInput(ctx, "orbits");

  mNOrbitsPerTF = o2::base::GRPGeomHelper::instance().getNHBFPerTF();

  if (hasOrbits) {
    // if (ctx.inputs().isValid("orbits")) {
    auto orbits = ctx.inputs().get<gsl::span<uint64_t>>("orbits");
    if (orbits.empty()) {
      static AliceO2::InfoLogger::InfoLogger::AutoMuteToken msgLimit(LogWarningSupport, 1, 600); // send it once every 10 minutes
      string msg = "WARNING: empty orbits vector";
      ILOG_INST.log(msgLimit, "%s", msg.c_str());
      return;
    }

    for (auto& orb : orbits) {
      storeOrbit(orb);
    }
  } else {
    addDefaultOrbitsInTF();
  }

  auto digits = ctx.inputs().get<gsl::span<o2::mch::Digit>>("digits");
  for (auto& d : digits) {
    plotDigit(d);
  }
}

void DigitsTask::storeOrbit(const uint64_t& orb)
{
  uint32_t orbit = (orb & 0xFFFFFFFF);
  uint32_t link = (orb >> 32) & 0xFF;
  uint32_t fee = (orb >> 40) & 0xFF;
  if (link != 15) {
    if (orbit != mLastOrbitSeen[fee][link]) {
      mNOrbits[fee][link] += 1;
    }
    mLastOrbitSeen[fee][link] = orbit;
  } else if (link == 15) {
    for (int li = 0; li < FecId::sLinkNum; li++) {
      if (orbit != mLastOrbitSeen[fee][li]) {
        mNOrbits[fee][li] += 1;
      }
      mLastOrbitSeen[fee][li] = orbit;
    }
  }
}

void DigitsTask::addDefaultOrbitsInTF()
{
  for (int fee = 0; fee < FecId::sFeeNum; fee++) {
    for (int li = 0; li < FecId::sLinkNum; li++) {
      mNOrbits[fee][li] += mNOrbitsPerTF;
    }
  }
}

void DigitsTask::plotDigit(const o2::mch::Digit& digit)
{
  int ADC = digit.getADC();
  int deId = digit.getDetID();
  int padId = digit.getPadID();

  if (ADC < 0 || deId <= 0 || padId < 0) {
    return;
  }

  // Fill NHits Elec Histogram and ADC distribution
  const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(deId);

  int dsId = segment.padDualSampaId(padId);
  int channel = segment.padDualSampaChannel(padId);

  bool isSignal = mIsSignalDigit(digit);

  //--------------------------------------------------------------------------
  // Occupancy plots
  //--------------------------------------------------------------------------

  // xbin and ybin uniquely identify each physical pad
  int xbin = getDsIndex(DsDetId{ deId, dsId });
  int ybin = channel + 1;

  mHistogramOccupancyElec->getNum()->Fill(xbin - 0.5, ybin - 0.5);
  if (isSignal) {
    mHistogramSignalOccupancyElec->getNum()->Fill(xbin - 0.5, ybin - 0.5);
  }

  //--------------------------------------------------------------------------
  // ADC amplitude plots
  //--------------------------------------------------------------------------

  auto h = mHistogramADCamplitudeDE.find(deId);
  if ((h != mHistogramADCamplitudeDE.end()) && (h->second != NULL)) {
    h->second->Fill(ADC);
  }
  mHistogramAmplitudeVsSamples->Fill(digit.getNofSamples(), ADC);

  //--------------------------------------------------------------------------
  // Time plots
  //--------------------------------------------------------------------------

  auto tfTime = digit.getTime();
  int orbit = -256;
  int bc = 3559;
  if (tfTime != o2::mch::raw::DataDecoder::tfTimeInvalid) {
    orbit = digit.getTime() / static_cast<int32_t>(o2::constants::lhc::LHCMaxBunches);
    bc = digit.getTime() % static_cast<int32_t>(o2::constants::lhc::LHCMaxBunches);
  }
  mHistogramDigitsOrbitElec->Fill(xbin - 0.5, orbit);
  mHistogramDigitsBcInOrbit->Fill(xbin - 0.5, bc);
  if (isSignal) {
    mHistogramDigitsSignalOrbitElec->Fill(xbin - 0.5, orbit);
  }
}

void DigitsTask::updateOrbits()
{
  static constexpr double sOrbitLengthInNanoseconds = 3564 * 25;
  static constexpr double sOrbitLengthInMicroseconds = sOrbitLengthInNanoseconds / 1000;
  static constexpr double sOrbitLengthInMilliseconds = sOrbitLengthInMicroseconds / 1000;

  // Fill NOrbits, in Elec view, for electronics channels associated to readout pads (in order to then compute the Occupancy in Elec view, physically meaningful because in Elec view, each bin is a physical pad)
  for (uint16_t feeId = 0; feeId < FecId::sFeeNum; feeId++) {

    // loop on FEE links and check if it corresponds to an existing SOLAR board
    for (uint8_t linkId = 0; linkId < FecId::sLinkNum; linkId++) {

      if (mNOrbits[feeId][linkId] == 0) {
        continue;
      }

      std::optional<uint16_t> solarId = mFeeLink2SolarMapper(FeeLinkId{ feeId, linkId });
      if (!solarId.has_value()) {
        continue;
      }

      // loop on DS boards and check if it exists in the mapping
      for (uint8_t dsAddr = 0; dsAddr < FecId::sDsNum; dsAddr++) {
        uint8_t groupId = dsAddr / 5;
        uint8_t dsAddrInGroup = dsAddr % 5;
        std::optional<DsDetId> dsDetId = mElec2DetMapper(DsElecId{ solarId.value(), groupId, dsAddrInGroup });
        if (!dsDetId.has_value()) {
          continue;
        }
        auto deId = dsDetId->deId();
        auto dsId = dsDetId->dsId();

        int xbin = getDsIndex(DsDetId{ deId, dsId });

        // loop on DS channels and check if it is associated to a readout pad
        for (int channel = 0; channel < 64; channel++) {

          const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(deId);
          int padId = segment.findPadByFEE(dsId, channel);
          if (padId < 0) {
            continue;
          }

          int ybin = channel + 1;
          mHistogramOccupancyElec->getDen()->SetBinContent(xbin, ybin, mNOrbits[feeId][linkId] * sOrbitLengthInMilliseconds);
          mHistogramSignalOccupancyElec->getDen()->SetBinContent(xbin, ybin, mNOrbits[feeId][linkId] * sOrbitLengthInMilliseconds);
        }
      }
    }
  }
}

void DigitsTask::resetOrbits()
{
  for (int fee = 0; fee < FecId::sFeeNum; fee++) {
    for (int link = 0; link < FecId::sLinkNum; link++) {
      mNOrbits[fee][link] = mLastOrbitSeen[fee][link] = 0;
    }
  }
}

void DigitsTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;

  updateOrbits();

  // update mergeable ratios
  mHistogramOccupancyElec->update();
  mHistogramSignalOccupancyElec->update();

  nCycles += 1;
}

void DigitsTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void DigitsTask::reset()
{
  // clean all the monitor objects here
  ILOG(Debug, Devel) << "Resetting the histograms" << AliceO2::InfoLogger::InfoLogger::endm;

  resetOrbits();

  for (auto h : mAllHistograms) {
    h->Reset();
  }
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
