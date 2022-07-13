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
#include <TGraph.h>
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
#include <DetectorsRaw/HBFUtils.h>

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

PhysicsTaskDigits::GlobalHistogramRatio::GlobalHistogramRatio(std::string name, std::string title, int id)
{
  float scaleFactors[2]{ 5, 10 };
  mHistOccupancy = std::make_shared<MergeableTH2Ratio>(name.c_str(), title.c_str(), 10, 0, 10, 10, 0, 10);

  mHistNum = std::make_shared<GlobalHistogram>((name + "_num").c_str(), (title + " Num").c_str(),
                                               id, scaleFactors[id], mHistOccupancy->getNum());
  mHistNum->init();
  mHistDen = std::make_shared<GlobalHistogram>((name + "_num").c_str(), (title + " Num").c_str(),
                                               id, scaleFactors[id], mHistOccupancy->getDen());
  mHistDen->init();
}

void PhysicsTaskDigits::GlobalHistogramRatio::setNum(std::map<int, std::shared_ptr<DetectorHistogram>>& histB, std::map<int, std::shared_ptr<DetectorHistogram>>& histNB)
{
  mHistNum->set(histB, histNB);
}

void PhysicsTaskDigits::GlobalHistogramRatio::setDen(std::map<int, std::shared_ptr<DetectorHistogram>>& histB, std::map<int, std::shared_ptr<DetectorHistogram>>& histNB)
{
  mHistDen->set(histB, histNB);
}

void PhysicsTaskDigits::GlobalHistogramRatio::update()
{
  mHistOccupancy->update();
}

void PhysicsTaskDigits::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize PhysicsTaskDigits" << AliceO2::InfoLogger::InfoLogger::endm;

  // flag to enable on-cycle plots
  mOnCycle = false;
  if (auto param = mCustomParameters.find("OnCycle"); param != mCustomParameters.end()) {
    if (param->second == "true" || param->second == "True" || param->second == "TRUE") {
      mOnCycle = true;
    }
  }

  // flag to enable extra disagnostics plots; it also enables on-cycle plots
  mDiagnostic = false;
  if (auto param = mCustomParameters.find("Diagnostic"); param != mCustomParameters.end()) {
    if (param->second == "true" || param->second == "True" || param->second == "TRUE") {
      mDiagnostic = true;
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
  mHistogramOccupancyElec = std::make_shared<MergeableTH2Ratio>("Occupancy_Elec", "Occupancy", nElecXbins, 0, nElecXbins, 64, 0, 64);
  publishObject(mHistogramOccupancyElec, "colz", false, false);

  mHistogramNHitsElec = mHistogramOccupancyElec->getNum();
  mHistogramNorbitsElec = mHistogramOccupancyElec->getDen();
  mAllHistograms.push_back(mHistogramNHitsElec);
  mAllHistograms.push_back(mHistogramNorbitsElec);

  mHistogramMeanOccupancyPerDE = std::make_shared<TH1F>("MeanOccupancy", "Mean Occupancy vs DE", getDEindexMax() + 1, 0, getDEindexMax() + 1);
  publishObject(mHistogramMeanOccupancyPerDE, "hist", false, false);

  mHistogramMeanOccupancyOnCyclePerDE = std::make_shared<TH1F>("MeanOccupancyOnCycle", "Mean Occupancy vs DE, last cycle", getDEindexMax() + 1, 0, getDEindexMax() + 1);
  publishObject(mHistogramMeanOccupancyOnCyclePerDE, "hist", false, !mOnCycle);

  // Histograms in global detector coordinates
  mHistogramOccupancyST12 = std::make_shared<GlobalHistogramRatio>("Occupancy_ST12", "ST12 Occupancy", 0);
  publishObject(mHistogramOccupancyST12->mHistOccupancy, "colz", false, false);

  mHistogramOccupancyST345 = std::make_shared<GlobalHistogramRatio>("Occupancy_ST345", "ST345 Occupancy", 1);
  publishObject(mHistogramOccupancyST345->mHistOccupancy, "colz", false, false);

  mHistogramOccupancyPrevCycleST12 = std::make_shared<GlobalHistogramRatio>("OccupancyPrevCycle_ST12", "ST12 Occupancy, last cycle", 0);
  mHistogramOccupancyOnCycleST12 = std::make_shared<GlobalHistogramRatio>("OccupancyOnCycle_ST12", "ST12 Occupancy on cycle", 0);
  publishObject(mHistogramOccupancyOnCycleST12->mHistOccupancy, "colz", false, !mOnCycle);

  mHistogramOccupancyPrevCycleST345 = std::make_shared<GlobalHistogramRatio>("OccupancyPrevCycle_ST345", "ST345 Occupancy, last cycle", 1);
  mHistogramOccupancyOnCycleST345 = std::make_shared<GlobalHistogramRatio>("OccupancyOnCycle_ST345", "ST345 Occupancy on cycle", 1);
  publishObject(mHistogramOccupancyOnCycleST345->mHistOccupancy, "colz", false, !mOnCycle);

  mHistogramDigitsOrbitInTFDE = std::make_shared<TH2F>("DigitOrbitInTFDE", "Digit orbits vs DE", getDEindexMax(), 0, getDEindexMax(), 768, -384, 384);
  publishObject(mHistogramDigitsOrbitInTFDE, "colz", false, false);

  mHistogramDigitsOrbitInTFDEPrevCycle = std::make_shared<TH2F>("DigitOrbitInTFDEPrevCycle", "Digit orbits vs DE", getDEindexMax(), 0, getDEindexMax(), 768, -384, 384);
  mHistogramDigitsOrbitInTFDEOnCycle = std::make_shared<TH2F>("DigitOrbitInTFDEOnCycle", "Digit orbits vs DE, last cycle", getDEindexMax(), 0, getDEindexMax(), 768, -384, 384);
  publishObject(mHistogramDigitsOrbitInTFDEOnCycle, "colz", false, !mOnCycle);

  mHistogramDigitsOrbitInTF = std::make_shared<TH2F>("Expert/DigitOrbitInTF", "Digit orbits vs DS Id", nElecXbins, 0, nElecXbins, 768, -384, 384);
  publishObject(mHistogramDigitsOrbitInTF, "colz", false, false);

  mHistogramDigitsBcInOrbit = std::make_shared<TH2F>("Expert/DigitsBcInOrbit", "Digit BC vs DS Id", nElecXbins, 0, nElecXbins, 3600, 0, 3600);
  publishObject(mHistogramDigitsBcInOrbit, "colz", false, true);

  mHistogramAmplitudeVsSamples = std::make_shared<TH2F>("Expert/AmplitudeVsSamples", "Digit amplitude vs nsamples", 1000, 0, 1000, 1000, 0, 10000);
  publishObject(mHistogramAmplitudeVsSamples, "colz", false, true);

  // Histograms in detector coordinates
  for (auto de : o2::mch::raw::deIdsForAllMCH) {
    auto h = std::make_shared<TH1F>(TString::Format("Expert/%sADCamplitude_DE%03d", getHistoPath(de).c_str(), de),
                                    TString::Format("ADC amplitude (DE%03d)", de), 5000, 0, 5000);
    mHistogramADCamplitudeDE.insert(make_pair(de, h));
    publishObject(h, "hist", false, true);

    auto hm = std::make_shared<MergeableTH2Ratio>(TString::Format("Expert/%sOccupancy_B_XY_%03d", getHistoPath(de).c_str(), de),
                                                  TString::Format("Occupancy XY (DE%03d B) (KHz)", de));
    mHistogramOccupancyDE[0].insert(make_pair(de, hm));
    publishObject(hm, "colz", false, true);

    auto h2n0 = std::make_shared<DetectorHistogram>(TString::Format("Expert/%sNhits_DE%03d_B", getHistoPath(de).c_str(), de),
                                                    TString::Format("Number of hits (DE%03d B)", de), de, int(0), hm->getNum());
    mHistogramNhitsDE[0].insert(make_pair(de, h2n0));
    mAllHistograms.push_back(h2n0->getHist());

    auto h2d0 = std::make_shared<DetectorHistogram>(TString::Format("Expert/%sNorbits_DE%03d_B", getHistoPath(de).c_str(), de),
                                                    TString::Format("Number of orbits (DE%03d B)", de), de, int(0), hm->getDen());
    mHistogramNorbitsDE[0].insert(make_pair(de, h2d0));
    mAllHistograms.push_back(h2d0->getHist());

    hm = std::make_shared<MergeableTH2Ratio>(TString::Format("Expert/%sOccupancy_NB_XY_%03d", getHistoPath(de).c_str(), de),
                                             TString::Format("Occupancy XY (DE%03d NB) (KHz)", de));
    mHistogramOccupancyDE[1].insert(make_pair(de, hm));
    publishObject(hm, "colz", false, true);

    auto h2n1 = std::make_shared<DetectorHistogram>(TString::Format("Expert/%sNhits_DE%03d_NB", getHistoPath(de).c_str(), de),
                                                    TString::Format("Number of hits (DE%03d NB)", de), de, int(1), hm->getNum());
    mHistogramNhitsDE[1].insert(make_pair(de, h2n1));
    mAllHistograms.push_back(h2n1->getHist());

    auto h2d1 = std::make_shared<DetectorHistogram>(TString::Format("Expert/%sNorbits_DE%03d_NB", getHistoPath(de).c_str(), de),
                                                    TString::Format("Number of orbits (DE%03d NB)", de), de, int(1), hm->getDen());
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
  bool hasOrbits = false;
  for (auto&& input : ctx.inputs()) {
    if (input.spec->binding == "orbits") {
      hasOrbits = true;
    }
  }

  if (hasOrbits) {
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

void PhysicsTaskDigits::addDefaultOrbitsInTF()
{
  for (int fee = 0; fee < PhysicsTaskDigits::sMaxFeeId; fee++) {
    for (int li = 0; li < PhysicsTaskDigits::sMaxLinkId; li++) {
      mNOrbits[fee][li] += o2::raw::HBFUtils::Instance().getNOrbitsPerTF();
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

  auto h = mHistogramADCamplitudeDE.find(deId);
  if ((h != mHistogramADCamplitudeDE.end()) && (h->second != NULL)) {
    h->second->Fill(ADC);
  }

  // Fill X Y 2D hits histogram with fired pads distribution
  auto hNhits = mHistogramNhitsDE[cathode].find(deId);
  if ((hNhits != mHistogramNhitsDE[cathode].end()) && (hNhits->second != NULL)) {
    hNhits->second->Fill(padX, padY, padSizeX, padSizeY);
  }

  // orbit relative to start of TF (or so it is expected)
  auto tfTime = digit.getTime();
  if (tfTime == o2::mch::raw::DataDecoder::tfTimeInvalid) {
    mHistogramDigitsOrbitInTF->Fill(xbin - 0.5, -256);
    mHistogramDigitsOrbitInTFDE->Fill(getDEindex(deId), -256);
    mHistogramDigitsBcInOrbit->Fill(xbin - 0.5, 3559);
  } else {
    auto orbit = digit.getTime() / o2::constants::lhc::LHCMaxBunches;
    auto bc = digit.getTime() % o2::constants::lhc::LHCMaxBunches;
    mHistogramDigitsOrbitInTF->Fill(xbin - 0.5, orbit);
    mHistogramDigitsOrbitInTFDE->Fill(getDEindex(deId), orbit);
    mHistogramDigitsBcInOrbit->Fill(xbin - 0.5, bc);
  }

  mHistogramAmplitudeVsSamples->Fill(digit.getNofSamples(), ADC);
}

void PhysicsTaskDigits::updateOrbits()
{
  static constexpr double sOrbitLengthInNanoseconds = 3564 * 25;
  static constexpr double sOrbitLengthInMicroseconds = sOrbitLengthInNanoseconds / 1000;
  static constexpr double sOrbitLengthInMilliseconds = sOrbitLengthInMicroseconds / 1000;

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
          mHistogramNorbitsElec->SetBinContent(xbin, ybin, mNOrbits[feeId][linkId] * sOrbitLengthInMilliseconds);

          double padX = segment.padPositionX(padId);
          double padY = segment.padPositionY(padId);
          float padSizeX = segment.padSizeX(padId);
          float padSizeY = segment.padSizeY(padId);
          int cathode = segment.isBendingPad(padId) ? 0 : 1;

          auto hNorbits = mHistogramNorbitsDE[cathode].find(deId);
          if ((hNorbits != mHistogramNorbitsDE[cathode].end()) && (hNorbits->second != NULL)) {
            hNorbits->second->Set(padX, padY, padSizeX, padSizeY, mNOrbits[feeId][linkId] * sOrbitLengthInMilliseconds);
          }
        }
      }
    }
  }
}

void PhysicsTaskDigits::endOfCycle()
{
  // copy bin contents from src to dst
  auto copyHist = [&](TH2F* hdst, TH2F* hsrc) {
    hdst->Reset("ICES");
    hdst->Add(hsrc);
  };

  // copy numerator and denominator from src to dst
  auto copyRatio = [&](std::shared_ptr<MergeableTH2Ratio> dst, std::shared_ptr<MergeableTH2Ratio> src) {
    copyHist(dst->getNum(), src->getNum());
    copyHist(dst->getDen(), src->getDen());
  };

  // copy numerator and denominator from src to dst
  auto copyGlobalHistRatio = [&](std::shared_ptr<GlobalHistogramRatio> dst, std::shared_ptr<GlobalHistogramRatio> src) {
    copyHist(dst->mHistOccupancy->getNum(), src->mHistOccupancy->getNum());
    copyHist(dst->mHistOccupancy->getDen(), src->mHistOccupancy->getDen());
  };

  // dst = src1 - src2
  auto subtractHist = [&](TH2F* hdst, TH2F* hsrc1, TH2F* hsrc2) {
    hdst->Reset("ICES");
    hdst->Add(hsrc1);
    hdst->Add(hsrc2, -1);
  };

  // compute (src1 - src2) difference of numerators and denominators
  auto subtractRatio = [&](std::shared_ptr<MergeableTH2Ratio> dst, std::shared_ptr<MergeableTH2Ratio> src1, std::shared_ptr<MergeableTH2Ratio> src2) {
    subtractHist(dst->getNum(), src1->getNum(), src2->getNum());
    subtractHist(dst->getDen(), src1->getDen(), src2->getDen());
    dst->update();
  };

  // compute (src1 - src2) difference of numerators and denominators
  auto subtractGlobalHistRatio = [&](std::shared_ptr<GlobalHistogramRatio> dst, std::shared_ptr<GlobalHistogramRatio> src1, std::shared_ptr<GlobalHistogramRatio> src2) {
    subtractHist(dst->mHistOccupancy->getNum(), src1->mHistOccupancy->getNum(), src2->mHistOccupancy->getNum());
    subtractHist(dst->mHistOccupancy->getDen(), src1->mHistOccupancy->getDen(), src2->mHistOccupancy->getDen());
    dst->update();
  };

  ILOG(Info, Support) << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;

  updateOrbits();

  // update mergeable ratios
  mHistogramOccupancyElec->update();

  for (auto de : o2::mch::raw::deIdsForAllMCH) {
    for (int i = 0; i < 2; i++) {
      auto h = mHistogramOccupancyDE[i].find(de);
      if ((h != mHistogramOccupancyDE[i].end()) && (h->second != NULL)) {
        h->second->update();
      }
    }
  }

  mHistogramOccupancyST12->setNum(mHistogramNhitsDE[0], mHistogramNhitsDE[1]);
  mHistogramOccupancyST12->setDen(mHistogramNorbitsDE[0], mHistogramNorbitsDE[1]);

  mHistogramOccupancyST345->setNum(mHistogramNhitsDE[0], mHistogramNhitsDE[1]);
  mHistogramOccupancyST345->setDen(mHistogramNorbitsDE[0], mHistogramNorbitsDE[1]);

  mHistogramOccupancyST12->update();
  mHistogramOccupancyST345->update();

  // fill on-cycle plots
  subtractGlobalHistRatio(mHistogramOccupancyOnCycleST12, mHistogramOccupancyST12, mHistogramOccupancyPrevCycleST12);
  subtractGlobalHistRatio(mHistogramOccupancyOnCycleST345, mHistogramOccupancyST345, mHistogramOccupancyPrevCycleST345);
  subtractHist(mHistogramDigitsOrbitInTFDEOnCycle.get(), mHistogramDigitsOrbitInTFDE.get(), mHistogramDigitsOrbitInTFDEPrevCycle.get());

  // update last cycle plots
  copyGlobalHistRatio(mHistogramOccupancyPrevCycleST12, mHistogramOccupancyST12);
  copyGlobalHistRatio(mHistogramOccupancyPrevCycleST345, mHistogramOccupancyST345);
  copyHist(mHistogramDigitsOrbitInTFDEPrevCycle.get(), mHistogramDigitsOrbitInTFDE.get());
}

void PhysicsTaskDigits::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
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
