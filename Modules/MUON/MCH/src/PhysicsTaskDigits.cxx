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
/// \file   PhysicsTaskDigits.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
/// \author Andrea Ferrero
/// \author Sebastien Perrin
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TFile.h>
#include <algorithm>

#include "MCH/PhysicsTaskDigits.h"
#ifdef MCH_HAS_MAPPING_FACTORY
#include "MCHMappingFactory/CreateSegmentation.h"
#endif
#include "MCHMappingInterface/Segmentation.h"
#include "MCHMappingInterface/CathodeSegmentation.h"
#include "MCHMappingSegContour/CathodeSegmentationContours.h"
#include "MCHRawDecoder/DataDecoder.h"
#include "QualityControl/QcInfoLogger.h"
#include <Framework/InputRecord.h>
#include <CommonConstants/LHCConstants.h>

using namespace std;
using namespace o2::mch::raw;

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

PhysicsTaskDigits::PhysicsTaskDigits() : TaskInterface() {}

PhysicsTaskDigits::~PhysicsTaskDigits() {}

void PhysicsTaskDigits::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize PhysicsTaskDigits" << AliceO2::InfoLogger::InfoLogger::endm;

  mDiagnostic = false;
  if (auto param = mCustomParameters.find("Diagnostic"); param != mCustomParameters.end()) {
    if (param->second == "true" || param->second == "True" || param->second == "TRUE") {
      mDiagnostic = true;
    }
  }

  mSaveToRootFile = false;
  if (auto param = mCustomParameters.find("SaveToRootFile"); param != mCustomParameters.end()) {
    if (param->second == "true" || param->second == "True" || param->second == "TRUE") {
      mSaveToRootFile = true;
    }
  }

  mElec2DetMapper = createElec2DetMapper<ElectronicMapperGenerated>();
  mDet2ElecMapper = createDet2ElecMapper<ElectronicMapperGenerated>();
  mFeeLink2SolarMapper = createFeeLink2SolarMapper<ElectronicMapperGenerated>();
  mSolar2FeeLinkMapper = createSolar2FeeLinkMapper<ElectronicMapperGenerated>();

  const uint32_t nElecXbins = PhysicsTaskDigits::sMaxFeeId * PhysicsTaskDigits::sMaxLinkId * PhysicsTaskDigits::sMaxDsId;

  for (int fee = 0; fee < PhysicsTaskDigits::sMaxFeeId; fee++) {
    for (int link = 0; link < PhysicsTaskDigits::sMaxLinkId; link++) {
      mNOrbits[fee][link] = mLastOrbitSeen[fee][link] = 0;
    }
  }

  // Histograms in electronics coordinates
  mHistogramOccupancyElec = std::make_shared<MergeableTH2Ratio>("Occupancy_Elec", "Occupancy (KHz)", nElecXbins, 0, nElecXbins, 64, 0, 64);
  mHistogramOccupancyElec->SetOption("colz");
  mAllHistograms.push_back(mHistogramOccupancyElec.get());
  if (!mSaveToRootFile) {
    getObjectsManager()->startPublishing(mHistogramOccupancyElec.get());
  }

  mHistogramNHitsElec = mHistogramOccupancyElec->getNum();
  mHistogramNorbitsElec = mHistogramOccupancyElec->getDen();
  mAllHistograms.push_back(mHistogramNHitsElec);
  mAllHistograms.push_back(mHistogramNorbitsElec);

  mMeanOccupancyPerDE = std::make_shared<MergeableTH1OccupancyPerDE>("MeanOccupancy", "Mean Occupancy of each DE (KHz)");
  mAllHistograms.push_back(mMeanOccupancyPerDE.get());
  if (!mSaveToRootFile) {
    getObjectsManager()->startPublishing(mMeanOccupancyPerDE.get());
  }

  // The code for the calculation of the on-cycle values is currently broken and therefore commented
  // mMeanOccupancyPerDECycle = std::make_shared<MergeableTH1OccupancyPerDECycle>("MeanOccupancyPerCycle", "Mean Occupancy of each DE Per Cycle (MHz)", mHistogramNHitsElec, mHistogramNorbitsElec);
  // getObjectsManager()->startPublishing(mMeanOccupancyPerDECycle.get());

  if (!mDiagnostic) {
    return;
  }

  mDigitsOrbitInTF = std::make_shared<TH2F>("Expert/DigitOrbitInTF", "Digit orbits vs DS Id", nElecXbins, 0, nElecXbins, 768, -384, 384);
  mDigitsOrbitInTF->SetOption("colz");
  mAllHistograms.push_back(mDigitsOrbitInTF.get());
  if (!mSaveToRootFile) {
    getObjectsManager()->startPublishing(mDigitsOrbitInTF.get());
  }

  mDigitsBcInOrbit = std::make_shared<TH2F>("Expert/DigitsBcInOrbit", "Digit BC vs DS Id", nElecXbins, 0, nElecXbins, 3600, 0, 3600);
  mDigitsBcInOrbit->SetOption("colz");
  mAllHistograms.push_back(mDigitsBcInOrbit.get());
  if (!mSaveToRootFile) {
    getObjectsManager()->startPublishing(mDigitsBcInOrbit.get());
  }

  mAmplitudeVsSamples = std::make_shared<TH2F>("Expert/AmplitudeVsSamples", "Digit amplitude vs nsamples", 1000, 0, 1000, 1000, 0, 10000);
  mAmplitudeVsSamples->SetOption("colz");
  mAllHistograms.push_back(mAmplitudeVsSamples.get());

  mAmplitudeElec = std::make_shared<TH2F>("Expert/AmplitudeElec", "Digit amplitude vs channel", nElecXbins, 0, nElecXbins, 1000, 0, 10000);
  mAmplitudeElec->SetOption("colz");
  mAllHistograms.push_back(mAmplitudeElec.get());

  // Histograms in detector coordinates
  for (auto de : o2::mch::raw::deIdsForAllMCH) {
    auto h = std::make_shared<TH1F>(TString::Format("Expert/%sADCamplitude_DE%03d", getHistoPath(de).c_str(), de),
                                    TString::Format("ADC amplitude (DE%03d)", de), 5000, 0, 5000);
    mHistogramADCamplitudeDE.insert(make_pair(de, h));
    mAllHistograms.push_back(h.get());
    if (!mSaveToRootFile) {
      getObjectsManager()->startPublishing(h.get());
    }

    h = std::make_shared<TH1F>(TString::Format("Expert/%sADCamplitude_DE%03d_Filtered", getHistoPath(de).c_str(), de),
                               TString::Format("ADC amplitude (DE%03d, filtered)", de), 5000, 0, 5000);
    mHistogramADCamplitudeDEFiltered.insert(make_pair(de, h));
    mAllHistograms.push_back(h.get());
    if (!mSaveToRootFile) {
      getObjectsManager()->startPublishing(h.get());
    }

    auto hm = std::make_shared<MergeableTH2Ratio>(TString::Format("Expert/%sOccupancy_B_XY_%03d", getHistoPath(de).c_str(), de),
                                                  TString::Format("Occupancy XY (DE%03d B) (KHz)", de));
    mHistogramOccupancyDE[0].insert(make_pair(de, hm));
    mAllHistograms.push_back(hm.get());
    if (!mSaveToRootFile) {
      getObjectsManager()->startPublishing(hm.get());
    }

    auto h2n0 = std::make_shared<DetectorHistogram>(TString::Format("Expert/%sNhits_DE%03d_B", getHistoPath(de).c_str(), de),
                                                    TString::Format("Number of hits (DE%03d B)", de), de, hm->getNum());
    mHistogramNhitsDE[0].insert(make_pair(de, h2n0));
    mAllHistograms.push_back(h2n0->getHist());

    auto h2d0 = std::make_shared<DetectorHistogram>(TString::Format("Expert/%sNorbits_DE%03d_B", getHistoPath(de).c_str(), de),
                                                    TString::Format("Number of orbits (DE%03d B)", de), de, hm->getDen());
    mHistogramNorbitsDE[0].insert(make_pair(de, h2d0));
    mAllHistograms.push_back(h2d0->getHist());

    hm = std::make_shared<MergeableTH2Ratio>(TString::Format("Expert/%sOccupancy_NB_XY_%03d", getHistoPath(de).c_str(), de),
                                             TString::Format("Occupancy XY (DE%03d NB) (KHz)", de));
    mHistogramOccupancyDE[1].insert(make_pair(de, hm));
    mAllHistograms.push_back(hm.get());
    if (!mSaveToRootFile) {
      getObjectsManager()->startPublishing(hm.get());
    }

    auto h2n1 = std::make_shared<DetectorHistogram>(TString::Format("Expert/%sNhits_DE%03d_NB", getHistoPath(de).c_str(), de),
                                                    TString::Format("Number of hits (DE%03d NB)", de), de, hm->getNum());
    mHistogramNhitsDE[1].insert(make_pair(de, h2n1));
    mAllHistograms.push_back(h2n1->getHist());

    auto h2d1 = std::make_shared<DetectorHistogram>(TString::Format("Expert/%sNorbits_DE%03d_NB", getHistoPath(de).c_str(), de),
                                                    TString::Format("Number of orbits (DE%03d NB)", de), de, hm->getDen());
    mHistogramNorbitsDE[1].insert(make_pair(de, h2d1));
    mAllHistograms.push_back(h2d1->getHist());
  }
}

void PhysicsTaskDigits::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void PhysicsTaskDigits::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void PhysicsTaskDigits::monitorData(o2::framework::ProcessingContext& ctx)
{
  // get the input preclusters and associated digits with the orbit information
  auto digits = ctx.inputs().get<gsl::span<o2::mch::Digit>>("digits");
  auto orbits = ctx.inputs().get<gsl::span<uint64_t>>("orbits");
  if (orbits.empty()) {
    ILOG(Info, Support) << "WARNING: empty orbits vector" << AliceO2::InfoLogger::InfoLogger::endm;
    return;
  }

  for (auto& orb : orbits) {
    storeOrbit(orb);
  }

  for (auto& d : digits) {
    plotDigit(d);
  }
}

void PhysicsTaskDigits::storeOrbit(const uint64_t& orb)
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
    for (int li = 0; li < PhysicsTaskDigits::sMaxLinkId; li++) {
      if (orbit != mLastOrbitSeen[fee][li]) {
        mNOrbits[fee][li] += 1;
      }
      mLastOrbitSeen[fee][li] = orbit;
    }
  }
}

void PhysicsTaskDigits::plotDigit(const o2::mch::Digit& digit)
{
  int ADC = digit.getADC();
  int deId = digit.getDetID();
  int padId = digit.getPadID();

  if (ADC < 0 || deId <= 0 || padId < 0) {
    return;
  }

  // Fill NHits Elec Histogram and ADC distribution
  const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(deId);

  double padX = segment.padPositionX(padId);
  double padY = segment.padPositionY(padId);
  float padSizeX = segment.padSizeX(padId);
  float padSizeY = segment.padSizeY(padId);
  int cathode = segment.isBendingPad(padId) ? 0 : 1;
  int dsId = segment.padDualSampaId(padId);
  int channel = segment.padDualSampaChannel(padId);

  // Using the mapping to go from Digit info (de, pad) to Elec info (fee, link) and fill Elec Histogram,
  // where one bin is one physical pad
  // get the unique solar ID and the DS address associated to this digit
  std::optional<DsElecId> dsElecId = mDet2ElecMapper(DsDetId{ deId, dsId });
  if (!dsElecId) {
    return;
  }
  uint32_t solarId = dsElecId->solarId();
  uint32_t dsAddr = dsElecId->elinkId();

  std::optional<FeeLinkId> feeLinkId = mSolar2FeeLinkMapper(solarId);
  if (!feeLinkId) {
    return;
  }
  uint32_t feeId = feeLinkId->feeId();
  uint32_t linkId = feeLinkId->linkId();

  // xbin and ybin uniquely identify each physical pad
  int xbin = feeId * PhysicsTaskDigits::sMaxLinkId * PhysicsTaskDigits::sMaxDsId + (linkId % PhysicsTaskDigits::sMaxLinkId) * PhysicsTaskDigits::sMaxDsId + dsAddr + 1;
  int ybin = channel + 1;

  mHistogramNHitsElec->Fill(xbin - 0.5, ybin - 0.5);

  if (!mDiagnostic) {
    return;
  }

  auto h = mHistogramADCamplitudeDE.find(deId);
  if ((h != mHistogramADCamplitudeDE.end()) && (h->second != NULL)) {
    h->second->Fill(ADC);
  }

  // Fill X Y 2D hits histogram with fired pads distribution
  auto hNhits = mHistogramNhitsDE[cathode].find(deId);
  if ((hNhits != mHistogramNhitsDE[cathode].end()) && (hNhits->second != NULL)) {
    // std::cout << "DE " << deId << "  cathod " << cathode << "    filling " << hOccupancy->second->getNum() << " with " << hNhits->second << std::endl;
    hNhits->second->Fill(padX, padY, padSizeX, padSizeY);
  }

  int targetBin = -1;
  // int targetBin = 6265;
  // int targetBin = 16135;
  // int targetBin = 16143;
  // int targetBin = 17127;

  if (targetBin > 0 && xbin != targetBin)
    return;

  float ToT = digit.getNofSamples() - 11; // time-over-threshold
  float ADCmin = std::pow(ToT, 1.9);
  bool isNoise = (ADC < ADCmin);
  // if (isNoise) return;

  if (!isNoise) {
    auto h = mHistogramADCamplitudeDEFiltered.find(deId);
    if ((h != mHistogramADCamplitudeDEFiltered.end()) && (h->second != NULL)) {
      h->second->Fill(ADC);
    }
  }

  // orbit relative to start of TF (or so it is expected)
  auto tfTime = digit.getTime();
  if (tfTime == o2::mch::raw::DataDecoder::tfTimeInvalid) {
    mDigitsOrbitInTF->Fill(xbin - 0.5, -256);
    mDigitsBcInOrbit->Fill(xbin - 0.5, 3559);
  } else {
    auto orbit = digit.getTime() / o2::constants::lhc::LHCMaxBunches;
    auto bc = digit.getTime() % o2::constants::lhc::LHCMaxBunches;
    if (orbit < 0) {
      std::cout << fmt::format("Out-of-time digit: TIME {}/{}/{}",
                               digit.getTime(), orbit, bc)
                << std::endl;
    }
    mDigitsOrbitInTF->Fill(xbin - 0.5, orbit);
    mDigitsBcInOrbit->Fill(xbin - 0.5, bc);
  }

  mAmplitudeVsSamples->Fill(digit.getNofSamples(), ADC);
  mAmplitudeElec->Fill(xbin - 0.5, ADC);
}

void PhysicsTaskDigits::updateOrbits()
{
  // Fill NOrbits, in Elec view, for electronics channels associated to readout pads (in order to then compute the Occupancy in Elec view, physically meaningful because in Elec view, each bin is a physical pad)
  for (uint16_t feeId = 0; feeId < sMaxFeeId; feeId++) {

    // loop on FEE links and check if it corresponds to an existing SOLAR board
    for (uint8_t linkId = 0; linkId < PhysicsTaskDigits::sMaxLinkId; linkId++) {

      if (mNOrbits[feeId][linkId] == 0) {
        continue;
      }

      std::optional<uint16_t> solarId = mFeeLink2SolarMapper(FeeLinkId{ feeId, linkId });
      if (!solarId.has_value()) {
        continue;
      }

      // loop on DS boards and check if it exists in the mapping
      for (uint8_t dsAddr = 0; dsAddr < PhysicsTaskDigits::sMaxDsId; dsAddr++) {
        uint8_t groupId = dsAddr / 5;
        uint8_t dsAddrInGroup = dsAddr % 5;
        std::optional<DsDetId> dsDetId = mElec2DetMapper(DsElecId{ solarId.value(), groupId, dsAddrInGroup });
        if (!dsDetId.has_value()) {
          continue;
        }
        auto deId = dsDetId->deId();
        auto dsid = dsDetId->dsId();

        int xbin = feeId * PhysicsTaskDigits::sMaxLinkId * PhysicsTaskDigits::sMaxDsId + (linkId % PhysicsTaskDigits::sMaxLinkId) * PhysicsTaskDigits::sMaxDsId + dsAddr + 1;

        // loop on DS channels and check if it is associated to a readout pad
        for (int channel = 0; channel < 64; channel++) {

          const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(deId);
          int padId = segment.findPadByFEE(dsid, channel);
          if (padId < 0) {
            continue;
          }

          int ybin = channel + 1;
          mHistogramNorbitsElec->SetBinContent(xbin, ybin, mNOrbits[feeId][linkId]);

          if (mDiagnostic) {
            double padX = segment.padPositionX(padId);
            double padY = segment.padPositionY(padId);
            float padSizeX = segment.padSizeX(padId);
            float padSizeY = segment.padSizeY(padId);
            int cathode = segment.isBendingPad(padId) ? 0 : 1;

            auto hNorbits = mHistogramNorbitsDE[cathode].find(deId);
            if ((hNorbits != mHistogramNorbitsDE[cathode].end()) && (hNorbits->second != NULL)) {
              hNorbits->second->Set(padX, padY, padSizeX, padSizeY, mNOrbits[feeId][linkId]);
            }
          }
        }
      }
    }
  }
}

void PhysicsTaskDigits::writeHistos()
{
  TFile f("mch-qc-digits.root", "RECREATE");
  for (auto h : mAllHistograms) {
    h->Write();
  }
  f.Close();
}

void PhysicsTaskDigits::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;

  updateOrbits();

  // update mergeable ratios
  mHistogramOccupancyElec->update();
  mMeanOccupancyPerDE->update(mHistogramOccupancyElec->getNum(), mHistogramOccupancyElec->getDen());

  if (mDiagnostic) {
    for (auto de : o2::mch::raw::deIdsForAllMCH) {
      for (int i = 0; i < 2; i++) {
        auto h = mHistogramOccupancyDE[i].find(de);
        if ((h != mHistogramOccupancyDE[i].end()) && (h->second != NULL)) {
          h->second->update();
        }
      }
    }
  }

  if (mSaveToRootFile) {
    writeHistos();
  }
}

void PhysicsTaskDigits::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;

  if (mSaveToRootFile) {
    writeHistos();
  }
}

void PhysicsTaskDigits::reset()
{
  // clean all the monitor objects here
  ILOG(Info, Support) << "Reseting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;

  for (auto h : mAllHistograms) {
    h->Reset();
  }
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
