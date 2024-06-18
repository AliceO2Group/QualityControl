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
/// \author Andrea Ferrero
/// \author Sebastien Perrin
///

#include "MCH/DigitsTask.h"
#include "MCH/Helpers.h"
#include "MUONCommon/Helpers.h"
#include "MCHMappingInterface/Segmentation.h"
#include "MCHRawDecoder/DataDecoder.h"
#include "QualityControl/QcInfoLogger.h"
#include "DetectorsBase/GRPGeomHelper.h"
#include <Framework/InputRecord.h>
#include <CommonConstants/LHCConstants.h>
#include <DetectorsRaw/HBFUtils.h>
#include "MCHGlobalMapping/DsIndex.h"

#include <TH1.h>
#include <TH2.h>

using namespace std;
using namespace o2::mch;
using namespace o2::mch::raw;
using namespace o2::quality_control_modules::muon;

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

template <typename T>
void DigitsTask::publishObject(T* histo, std::string drawOption, bool statBox, bool isExpert)
{
  histo->SetOption(drawOption.c_str());
  if (!statBox) {
    histo->SetStats(0);
  }
  mAllHistograms.push_back(histo);
  if (mFullHistos || (isExpert == false)) {
    getObjectsManager()->startPublishing(histo);
    getObjectsManager()->setDefaultDrawOptions(histo, drawOption);
  }
}

void DigitsTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize DigitsTask" << AliceO2::InfoLogger::InfoLogger::endm;

  mIsSignalDigit = o2::mch::createDigitFilter(20, true, true);

  // flag to enable extra disagnostics plots; it also enables on-cycle plots
  mFullHistos = getConfigurationParameter<bool>(mCustomParameters, "FullHistos", mFullHistos);

  resetOrbits();

  const uint32_t nElecXbins = NumberOfDualSampas;

  // Histograms in electronics coordinates
  mHistogramOccupancyElec = std::make_unique<TH2FRatio>("Occupancy_Elec", "Occupancy", nElecXbins, 0, nElecXbins, 64, 0, 64, true);
  mHistogramOccupancyElec->Sumw2(kFALSE);
  publishObject(mHistogramOccupancyElec.get(), "colz", false, false);

  mHistogramSignalOccupancyElec = std::make_unique<TH2FRatio>("OccupancySignal_Elec", "Occupancy (signal)", nElecXbins, 0, nElecXbins, 64, 0, 64, true);
  mHistogramSignalOccupancyElec->Sumw2(kFALSE);
  publishObject(mHistogramSignalOccupancyElec.get(), "colz", false, false);

  mHistogramDigitsOrbitElec = std::make_unique<TH2F>("DigitOrbit_Elec", "Digit orbits vs DS Id", nElecXbins, 0, nElecXbins, 130, -1, 129);
  publishObject(mHistogramDigitsOrbitElec.get(), "colz", true, false);

  mHistogramDigitsSignalOrbitElec = std::make_unique<TH2F>("DigitSignalOrbit_Elec", "Digit orbits vs DS Id (signal)", nElecXbins, 0, nElecXbins, 130, -1, 129);
  publishObject(mHistogramDigitsSignalOrbitElec.get(), "colz", true, false);

  if (mFullHistos) {
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
  static auto nOrbitsPerTF = o2::base::GRPGeomHelper::instance().getNHBFPerTF();
  mNOrbits += nOrbitsPerTF;

  auto digits = ctx.inputs().get<gsl::span<o2::mch::Digit>>("digits");
  for (auto& d : digits) {
    plotDigit(d);
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

  // fecId and channel uniquely identify each physical pad
  int fecId = getDsIndex(DsDetId{ deId, dsId });

  mHistogramOccupancyElec->getNum()->Fill(fecId, channel);
  if (isSignal) {
    mHistogramSignalOccupancyElec->getNum()->Fill(fecId, channel);
  }

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
  mHistogramDigitsOrbitElec->Fill(fecId, orbit);
  if (isSignal) {
    mHistogramDigitsSignalOrbitElec->Fill(fecId, orbit);
  }

  if (mFullHistos) {
    mHistogramDigitsBcInOrbit->Fill(fecId, bc);

    //--------------------------------------------------------------------------
    // ADC amplitude plots
    //--------------------------------------------------------------------------

    auto h = mHistogramADCamplitudeDE.find(deId);
    if ((h != mHistogramADCamplitudeDE.end()) && (h->second != NULL)) {
      h->second->Fill(ADC);
    }
    mHistogramAmplitudeVsSamples->Fill(digit.getNofSamples(), ADC);
  }
}

void DigitsTask::updateOrbits()
{
  static constexpr double sOrbitLengthInNanoseconds = 3564 * 25;
  static constexpr double sOrbitLengthInMicroseconds = sOrbitLengthInNanoseconds / 1000;
  static constexpr double sOrbitLengthInMilliseconds = sOrbitLengthInMicroseconds / 1000;

  mHistogramOccupancyElec->getDen()->SetBinContent(1, 1, mNOrbits * sOrbitLengthInMilliseconds);
  mHistogramSignalOccupancyElec->getDen()->SetBinContent(1, 1, mNOrbits * sOrbitLengthInMilliseconds);
}

void DigitsTask::resetOrbits()
{
  mNOrbits = 0;
}

void DigitsTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;

  updateOrbits();

  // update mergeable ratios
  mHistogramOccupancyElec->update();
  mHistogramSignalOccupancyElec->update();
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
