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
#include "QualityControl/QcInfoLogger.h"
#include <Framework/InputRecord.h>

using namespace std;
using namespace o2::mch::raw;

//#define QC_MCH_SAVE_TEMP_ROOTFILE 1

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

PhysicsTaskDigits::PhysicsTaskDigits() : TaskInterface() {}

PhysicsTaskDigits::~PhysicsTaskDigits() {}

static std::string getHistoPath(int deId)
{
  return fmt::format("ST{}/DE{}/", (deId - 100) / 200 + 1, deId);
}

void PhysicsTaskDigits::initialize(o2::framework::InitContext& /*ctx*/)
{
  QcInfoLogger::GetInstance() << "initialize PhysicsTaskDigits" << AliceO2::InfoLogger::InfoLogger::endm;

  mElec2DetMapper = createElec2DetMapper<ElectronicMapperGenerated>();
  mDet2ElecMapper = createDet2ElecMapper<ElectronicMapperGenerated>();
  mFeeLink2SolarMapper = createFeeLink2SolarMapper<ElectronicMapperGenerated>();
  mSolar2FeeLinkMapper = createSolar2FeeLinkMapper<ElectronicMapperGenerated>();

  for (int fee = 0; fee < PhysicsTaskDigits::sMaxFeeId; fee++) {
    for (int link = 0; link < PhysicsTaskDigits::sMaxLinkId; link++) {
      mNOrbits[fee][link] = mLastOrbitSeen[fee][link] = 0;
    }
  }

  // Histograms in electronics coordinates
  for (int feeid = 0; feeid < PhysicsTaskDigits::sMaxFeeId; feeid++) {
    for (int linkId = 0; linkId < PhysicsTaskDigits::sMaxLinkId; linkId++) {
      int index = PhysicsTaskDigits::sMaxLinkId * feeid + linkId;
      mHistogramNHits[index] = new TH2F(TString::Format("NHits_FEE%01d_LINK%02d", feeid, linkId),
                                        TString::Format("Number of hits (FEE link %02d)", index), PhysicsTaskDigits::sMaxDsId, 0, PhysicsTaskDigits::sMaxDsId, 64, 0, 64);
    }
  }

  const uint32_t nElecXbins = PhysicsTaskDigits::sMaxFeeId * PhysicsTaskDigits::sMaxLinkId * PhysicsTaskDigits::sMaxDsId;
  mHistogramNorbitsElec = new TH2F("Norbits_Elec", "Norbits", nElecXbins, 0, nElecXbins, 64, 0, 64);
  mHistogramNorbitsElec->SetOption("colz");
  getObjectsManager()->startPublishing(mHistogramNorbitsElec);
  mHistogramNHitsElec = new TH2F("NHits_Elec", "NHits", nElecXbins, 0, nElecXbins, 64, 0, 64);
  mHistogramNHitsElec->SetOption("colz");
  getObjectsManager()->startPublishing(mHistogramNHitsElec);

  mHistogramOccupancyElec = new MergeableTH2Ratio("Occupancy_Elec", "Occupancy (KHz)",
                                                  mHistogramNHitsElec, mHistogramNorbitsElec);
  getObjectsManager()->startPublishing(mHistogramOccupancyElec);
  mHistogramOccupancyElec->SetOption("colz");

  // Histograms in detector coordinates
  for (auto de : o2::mch::raw::deIdsForAllMCH) {
    TH1F* h = new TH1F(TString::Format("%sADCamplitude_DE%03d", getHistoPath(de).c_str(), de),
                       TString::Format("ADC amplitude (DE%03d)", de), 5000, 0, 5000);
    mHistogramADCamplitudeDE.insert(make_pair(de, h));

    float Xsize = 40 * 5;
    float Xsize2 = Xsize / 2;
    float Ysize = 50;
    float Ysize2 = Ysize / 2;

    TH2F* h2n0 = new TH2F(TString::Format("%sNhits_DE%03d_B", getHistoPath(de).c_str(), de),
                          TString::Format("Number of hits (DE%03d B)", de), Xsize * 2, -Xsize2, Xsize2, Ysize * 2, -Ysize2, Ysize2);
    mHistogramNhitsDE[0].insert(make_pair(de, h2n0));

    TH2F* h2n1 = new TH2F(TString::Format("%sNhits_DE%03d_NB", getHistoPath(de).c_str(), de),
                          TString::Format("Number of hits (DE%03d NB)", de), Xsize * 2, -Xsize2, Xsize2, Ysize * 2, -Ysize2, Ysize2);
    mHistogramNhitsDE[1].insert(make_pair(de, h2n1));

    TH2F* h2d0 = new TH2F(TString::Format("%sNorbits_DE%03d_B", getHistoPath(de).c_str(), de),
                          TString::Format("Number of orbits (DE%03d B)", de), Xsize * 2, -Xsize2, Xsize2, Ysize * 2, -Ysize2, Ysize2);
    mHistogramNorbitsDE[0].insert(make_pair(de, h2d0));
    TH2F* h2d1 = new TH2F(TString::Format("%sNorbits_DE%03d_NB", getHistoPath(de).c_str(), de),
                          TString::Format("Number of orbits (DE%03d NB)", de), Xsize * 2, -Xsize2, Xsize2, Ysize * 2, -Ysize2, Ysize2);
    mHistogramNorbitsDE[1].insert(make_pair(de, h2d1));

    MergeableTH2Ratio* hm = new MergeableTH2Ratio(TString::Format("%sOccupancy_B_XY_%03d", getHistoPath(de).c_str(), de),
                                                  TString::Format("Occupancy XY (DE%03d B) (KHz)", de), h2n0, h2d0);
    mHistogramOccupancyDE[0].insert(make_pair(de, hm));
    getObjectsManager()->startPublishing(hm);

    hm = new MergeableTH2Ratio(TString::Format("%sOccupancy_NB_XY_%03d", getHistoPath(de).c_str(), de),
                               TString::Format("Occupancy XY (DE%03d NB) (KHz)", de), h2n1, h2d1);
    mHistogramOccupancyDE[1].insert(make_pair(de, hm));
    getObjectsManager()->startPublishing(hm);
  }

  mHistogramNHitsAllDE = new GlobalHistogram("NHits_AllDE", "Number of hits");
  mHistogramNHitsAllDE->init();
  mHistogramNHitsAllDE->SetOption("colz");
  getObjectsManager()->startPublishing(mHistogramNHitsAllDE);

  mHistogramOrbitsAllDE = new GlobalHistogram("Norbits_AllDE", "Number of orbits");
  mHistogramOrbitsAllDE->init();
  mHistogramOrbitsAllDE->SetOption("colz");
  getObjectsManager()->startPublishing(mHistogramOrbitsAllDE);

  mHistogramOccupancyAllDE = new MergeableTH2Ratio("Occupancy_AllDE", "Occupancy (KHz)",
                                                   mHistogramNHitsAllDE, mHistogramOrbitsAllDE);
  mHistogramOccupancyAllDE->SetOption("colz");
  getObjectsManager()->startPublishing(mHistogramOccupancyAllDE);
}

void PhysicsTaskDigits::startOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void PhysicsTaskDigits::startOfCycle()
{
  QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void PhysicsTaskDigits::monitorData(o2::framework::ProcessingContext& ctx)
{
  // get the input preclusters and associated digits with the orbit information
  auto digits = ctx.inputs().get<gsl::span<o2::mch::Digit>>("digits");
  auto orbits = ctx.inputs().get<gsl::span<uint64_t>>("orbits");
  if (orbits.empty()) {
    QcInfoLogger::GetInstance() << "WARNING: empty orbits vector" << AliceO2::InfoLogger::InfoLogger::endm;
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
  int de = digit.getDetID();
  int padid = digit.getPadID();

  if (ADC < 0 || de <= 0 || padid < 0) {
    return;
  }

  auto h = mHistogramADCamplitudeDE.find(de);
  if ((h != mHistogramADCamplitudeDE.end()) && (h->second != NULL)) {
    h->second->Fill(ADC);
  }

  if (ADC < 100) return;

  // Fill NHits Elec Histogram and ADC distribution
  const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(de);

  double padX = segment.padPositionX(padid);
  double padY = segment.padPositionY(padid);
  float padSizeX = segment.padSizeX(padid);
  float padSizeY = segment.padSizeY(padid);
  int cathode = segment.isBendingPad(padid) ? 0 : 1;
  int dsId = segment.padDualSampaId(padid);
  int channel = segment.padDualSampaChannel(padid);

  // Fill X Y 2D hits histogram with fired pads distribution
  auto h2 = mHistogramNhitsDE[cathode].find(de);
  if ((h2 != mHistogramNhitsDE[cathode].end()) && (h2->second != NULL)) {
    int binx_min = h2->second->GetXaxis()->FindBin(padX - padSizeX / 2 + 0.1);
    int binx_max = h2->second->GetXaxis()->FindBin(padX + padSizeX / 2 - 0.1);
    int biny_min = h2->second->GetYaxis()->FindBin(padY - padSizeY / 2 + 0.1);
    int biny_max = h2->second->GetYaxis()->FindBin(padY + padSizeY / 2 - 0.1);
    for (int by = biny_min; by <= biny_max; by++) {
      float y = h2->second->GetYaxis()->GetBinCenter(by);
      for (int bx = binx_min; bx <= binx_max; bx++) {
        float x = h2->second->GetXaxis()->GetBinCenter(bx);
        h2->second->Fill(x, y);
      }
    }
  }

  // Using the mapping to go from Digit info (de, pad) to Elec info (fee, link) and fill Elec Histogram,
  // where one bin is one physical pad
  // get the unique solar ID and the DS address associated to this digit
  std::optional<DsElecId> dsElecId = mDet2ElecMapper(DsDetId{ de, dsId });
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
        auto de = dsDetId->deId();
        auto dsid = dsDetId->dsId();

        int xbin = feeId * PhysicsTaskDigits::sMaxLinkId * PhysicsTaskDigits::sMaxDsId + (linkId % PhysicsTaskDigits::sMaxLinkId) * PhysicsTaskDigits::sMaxDsId + dsAddr + 1;

        // loop on DS channels and check if it is associated to a readout pad
        for (int channel = 0; channel < 64; channel++) {

          const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(de);
          int padid = segment.findPadByFEE(dsid, channel);
          if (padid < 0) {
            continue;
          }

          int ybin = channel + 1;
          mHistogramNorbitsElec->SetBinContent(xbin, ybin, mNOrbits[feeId][linkId]);

          double x_pad = segment.padPositionX(padid);
          double y_pad = segment.padPositionY(padid);
          float padSizeX = segment.padSizeX(padid);
          float padSizeY = segment.padSizeY(padid);
          int cathode = segment.isBendingPad(padid) ? 0 : 1;

          auto h2 = mHistogramNorbitsDE[cathode].find(de);
          if ((h2 != mHistogramNorbitsDE[cathode].end()) && (h2->second != NULL)) {

            int binx_min = h2->second->GetXaxis()->FindBin(x_pad - padSizeX / 2 + 0.1);
            int binx_max = h2->second->GetXaxis()->FindBin(x_pad + padSizeX / 2 - 0.1);
            int biny_min = h2->second->GetYaxis()->FindBin(y_pad - padSizeY / 2 + 0.1);
            int biny_max = h2->second->GetYaxis()->FindBin(y_pad + padSizeY / 2 - 0.1);
            for (int by = biny_min; by <= biny_max; by++) {
              for (int bx = binx_min; bx <= binx_max; bx++) {
                h2->second->SetBinContent(bx, by, mNOrbits[feeId][linkId]);
              }
            }
          }
        }
      }
    }
  }
}

void PhysicsTaskDigits::endOfCycle()
{
  QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;

  updateOrbits();

  mHistogramOrbitsAllDE->set(mHistogramNorbitsDE[0], mHistogramNorbitsDE[1]);
  mHistogramNHitsAllDE->set(mHistogramNhitsDE[0], mHistogramNhitsDE[1]);

  // update mergeable ratios
  mHistogramOccupancyAllDE->update();
  mHistogramOccupancyElec->update();
  for (auto de : o2::mch::raw::deIdsForAllMCH) {
    for (int i = 0; i < 2; i++) {
      auto h = mHistogramOccupancyDE[i].find(de);
      if ((h != mHistogramOccupancyDE[i].end()) && (h->second != NULL)) {
        h->second->update();
      }
    }
  }
}

void PhysicsTaskDigits::endOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;

#ifdef QC_MCH_SAVE_TEMP_ROOTFILE
  TFile f("qc-digits.root", "RECREATE");

  mHistogramNorbitsElec->Write();
  mHistogramNHitsElec->Write();
  mHistogramOccupancyElec->Write();

  for (int de = 0; de < 1100; de++) {
    {
      auto h = mHistogramADCamplitudeDE.find(de);
      if ((h != mHistogramADCamplitudeDE.end()) && (h->second != NULL)) {
        h->second->Write();
      }
    }
    for (int i = 0; i < 2; i++) {
      {
        auto h = mHistogramNhitsDE[i].find(de);
        if ((h != mHistogramNhitsDE[i].end()) && (h->second != NULL)) {
          h->second->Write();
        }
      }
      {
        auto h = mHistogramNorbitsDE[i].find(de);
        if ((h != mHistogramNorbitsDE[i].end()) && (h->second != NULL)) {
          h->second->Write();
        }
      }
      {
        auto h = mHistogramOccupancyDE[i].find(de);
        if ((h != mHistogramOccupancyDE[i].end()) && (h->second != NULL)) {
          h->second->Write();
        }
      }
    }
  }

  f.Close();
#endif
}

void PhysicsTaskDigits::reset()
{
  // clean all the monitor objects here

  QcInfoLogger::GetInstance() << "Reseting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
