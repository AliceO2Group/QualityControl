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

void PhysicsTaskDigits::initialize(o2::framework::InitContext& /*ctx*/)
{
  QcInfoLogger::GetInstance() << "initialize PhysicsTaskDigits" << AliceO2::InfoLogger::InfoLogger::endm;

  mElec2DetMapper = createElec2DetMapper<ElectronicMapperGenerated>();
  mDet2ElecMapper = createDet2ElecMapper<ElectronicMapperGenerated>();
  mFeeLink2SolarMapper = createFeeLink2SolarMapper<ElectronicMapperGenerated>();
  mSolar2FeeLinkMapper = createSolar2FeeLinkMapper<ElectronicMapperGenerated>();

  for (int feeid = 0; feeid < MCH_FEEID_NUM; feeid++) {
    for (int linkid = 0; linkid < 12; linkid++) {
      int index = 12 * feeid + linkid;
      mHistogramNhits[index] = new TH2F(TString::Format("QcMuonChambers_NHits_FEE%01d_LINK%02d", feeid, linkid),
                                        TString::Format("QcMuonChambers - Number of hits (FEE link %02d)", index), 40, 0, 40, 64, 0, 64);
      mHistogramADCamplitude[index] = new TH1F(TString::Format("QcMuonChambers_ADC_Amplitude_FEE%01d_LINK%02d", feeid, linkid),
                                               TString::Format("QcMuonChambers - ADC amplitude (FEE link %02d)", index), 5000, 0, 5000);
    }
  }
  for (auto de : o2::mch::raw::deIdsForAllMCH) {
    TH1F* h = new TH1F(TString::Format("QcMuonChambers_ADCamplitude_DE%03d", de),
                       TString::Format("QcMuonChambers - ADC amplitude (DE%03d)", de), 5000, 0, 5000);
    mHistogramADCamplitudeDE.insert(make_pair(de, h));

    float Xsize = 40 * 5;
    float Xsize2 = Xsize / 2;
    float Ysize = 50;
    float Ysize2 = Ysize / 2;
    float scale = 0.5;

    TH2F* h2 = new TH2F(TString::Format("QcMuonChambers_Nhits_DE%03d_B", de),
                        TString::Format("QcMuonChambers - Number of hits (DE%03d B)", de), Xsize * 2, -Xsize2, Xsize2, Ysize * 2, -Ysize2, Ysize2);
    mHistogramNhitsDE[0].insert(make_pair(de, h2));
    // getObjectsManager()->startPublishing(h2);
    h2 = new TH2F(TString::Format("QcMuonChambers_Nhits_DE%03d_NB", de),
                  TString::Format("QcMuonChambers - Number of hits (DE%03d NB)", de), Xsize * 2, -Xsize2, Xsize2, Ysize * 2, -Ysize2, Ysize2);
    mHistogramNhitsDE[1].insert(make_pair(de, h2));
    // getObjectsManager()->startPublishing(h2);
    h2 = new TH2F(TString::Format("QcMuonChambers_Norbits_DE%03d_B", de),
                  TString::Format("QcMuonChambers - Number of orbits (DE%03d B)", de), Xsize * 2, -Xsize2, Xsize2, Ysize * 2, -Ysize2, Ysize2);
    mHistogramNorbitsDE[0].insert(make_pair(de, h2));
    h2 = new TH2F(TString::Format("QcMuonChambers_Norbits_DE%03d_NB", de),
                  TString::Format("QcMuonChambers - Number of orbits (DE%03d NB)", de), Xsize * 2, -Xsize2, Xsize2, Ysize * 2, -Ysize2, Ysize2);
    mHistogramNorbitsDE[1].insert(make_pair(de, h2));

    {
      TH2F* hXY = new TH2F(TString::Format("QcMuonChambers_Occupancy_B_XY_%03d", de),
                           TString::Format("QcMuonChambers - Occupancy XY (DE%03d B) (MHz)", de), Xsize / scale, -Xsize2, Xsize2, Ysize / scale, -Ysize2, Ysize2);
      mHistogramOccupancyXY[0].insert(make_pair(de, hXY));
      // getObjectsManager()->startPublishing(hXY);
      hXY = new TH2F(TString::Format("QcMuonChambers_Occupancy_NB_XY_%03d", de),
                     TString::Format("QcMuonChambers - Occupancy XY (DE%03d NB) (MHz)", de), Xsize / scale, -Xsize2, Xsize2, Ysize / scale, -Ysize2, Ysize2);
      mHistogramOccupancyXY[1].insert(make_pair(de, hXY));
      //  getObjectsManager()->startPublishing(hXY);
    }
  }

  for (int fee = 0; fee < MCH_FEEID_NUM; fee++) {
    for (int link = 0; link < 12; link++) {
      norbits[fee][link] = lastorbitseen[fee][link] = 0;
    }
  }
  for (int de = 0; de < 1100; de++) {
    MeanOccupancyDE[de] = MeanOccupancyDECycle[de] = LastMeanNhitsDE[de] = LastMeanNorbitsDE[de] = NewMeanNhitsDE[de] = NewMeanNorbitsDE[de] = NbinsDE[de] = 0;
  }

  // Histograms using the Electronic Mapping

  mHistogramNorbitsElec = new TH2F("QcMuonChambers_Norbits_Elec", "QcMuonChambers - Norbits",
                                   MCH_FEEID_NUM * 12 * 40, 0, MCH_FEEID_NUM * 12 * 40, 64, 0, 64);
  mHistogramNorbitsElec->SetOption("colz");
  getObjectsManager()->startPublishing(mHistogramNorbitsElec);
  mHistogramNHitsElec = new TH2F("QcMuonChambers_NHits_Elec", "QcMuonChambers - NHits",
                                 MCH_FEEID_NUM * 12 * 40, 0, MCH_FEEID_NUM * 12 * 40, 64, 0, 64);
  mHistogramNHitsElec->SetOption("colz");
  getObjectsManager()->startPublishing(mHistogramNHitsElec);
  mHistogramOccupancyElec = new TH2F("QcMuonChambers_Occupancy_Elec", "QcMuonChambers - Occupancy (MHz)",
                                     MCH_FEEID_NUM * 12 * 40, 0, MCH_FEEID_NUM * 12 * 40, 64, 0, 64);
  mHistogramOccupancyElec->SetOption("colz");
  getObjectsManager()->startPublishing(mHistogramOccupancyElec);

  // 1D histograms for mean occupancy per DE (integrated or per elapsed cycle) - Sent for Trending

  mMeanOccupancyPerDE = new TH1F("QcMuonChambers_MeanOccupancy", "Mean Occupancy of each DE (MHz)", 1100, -0.5, 1099.5);
  getObjectsManager()->startPublishing(mMeanOccupancyPerDE);
  mMeanOccupancyPerDECycle = new TH1F("QcMuonChambers_MeanOccupancy_OnCycle", "Mean Occupancy of each DE during the cycle (MHz)", 1100, -0.5, 1099.5);
  getObjectsManager()->startPublishing(mMeanOccupancyPerDECycle);

  mHistogramOccupancy[0] = new GlobalHistogram("QcMuonChambers_Occupancy_den", "Occupancy (MHz)");
  mHistogramOccupancy[0]->init();
  mHistogramOccupancy[0]->SetOption("colz");
  getObjectsManager()->startPublishing(mHistogramOccupancy[0]);

  mHistogramOrbits[0] = new GlobalHistogram("QcMuonChambers_Orbits_den", "Orbits");
  mHistogramOrbits[0]->init();
  mHistogramOrbits[0]->SetOption("colz");
  getObjectsManager()->startPublishing(mHistogramOrbits[0]);
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
    uint32_t orbitnumber = (orb & 0xFFFFFFFF);
    uint32_t link = (orb >> 32) & 0xFF;
    uint32_t fee = (orb >> 40) & 0xFF;
    if (link != 15) {
      if (orbitnumber != lastorbitseen[fee][link]) {
        norbits[fee][link] += 1;
      }
      lastorbitseen[fee][link] = orbitnumber;
    } else if (link == 15) {
      for (int li = 0; li < 12; li++) {
        if (orbitnumber != lastorbitseen[fee][li]) {
          norbits[fee][li] += 1;
        }
        lastorbitseen[fee][li] = orbitnumber;
      }
    }
  }

  for (auto& d : digits) {
    plotDigit(d);
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

  // Fill NHits Elec Histogram and ADC distribution
  // Using the mapping to go from Digit info (de, pad) to Elec info (fee, link) and fill Elec Histogram, where one bin is one physical pad
  const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(de);

  double padX = segment.padPositionX(padid);
  double padY = segment.padPositionY(padid);
  float padSizeX = segment.padSizeX(padid);
  float padSizeY = segment.padSizeY(padid);
  int cathode = segment.isBendingPad(padid) ? 0 : 1;
  int dsid = segment.padDualSampaId(padid);
  int chan_addr = segment.padDualSampaChannel(padid);

  uint32_t solar_id = 0;
  uint32_t ds_addr = 0;
  int32_t linkid = 0;
  int32_t fee_id = 0;

  // get the unique solar ID and the DS address associated to this digit
  std::optional<DsElecId> dsElecId = mDet2ElecMapper(DsDetId{ de, dsid });
  if (dsElecId.has_value()) {
    solar_id = dsElecId->solarId();
    ds_addr = dsElecId->elinkId();

    std::optional<FeeLinkId> feeLinkId = mSolar2FeeLinkMapper(solar_id);
    if (feeLinkId.has_value()) {
      fee_id = feeLinkId->feeId();
      linkid = feeLinkId->linkId();
    }
  }

  //xbin and ybin uniquely identify each physical pad
  int xbin = fee_id * 12 * 40 + (linkid % 12) * 40 + ds_addr + 1;
  int ybin = chan_addr + 1;

  mHistogramNHitsElec->Fill(xbin - 0.5, ybin - 0.5);

  auto h = mHistogramADCamplitudeDE.find(de);
  if ((h != mHistogramADCamplitudeDE.end()) && (h->second != NULL)) {
    h->second->Fill(ADC);
  }

  if (ADC <= 0) {
    return;
  }

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

  // Get the Elec information to fill histogram of orbits (XY histogram)
  h2 = mHistogramNorbitsDE[cathode].find(de);
  if ((h2 != mHistogramNorbitsDE[cathode].end()) && (h2->second != NULL)) {
    int NYbins = h2->second->GetYaxis()->GetNbins();
    int NXbins = h2->second->GetXaxis()->GetNbins();
    for (int by = 0; by < NYbins; by++) {
      float y = h2->second->GetYaxis()->GetBinCenter(by);
      for (int bx = 0; bx < NXbins; bx++) {
        float x = h2->second->GetXaxis()->GetBinCenter(bx);

        // get pad and DS channel mappings for this bin
        int bpad = 0;
        int nbpad = 0;
        uint32_t solar_id_boucle = 0;

        if (segment.findPadPairByPosition(x, y, bpad, nbpad)) {

          // get the unique solar ID and the DS address associated to this bin
          int padid_boucle = (cathode == 0) ? bpad : nbpad;
          int dsid_boucle = segment.padDualSampaId(padid_boucle);
          std::optional<DsElecId> dsElecId = mDet2ElecMapper(DsDetId{ de, dsid_boucle });
          if (dsElecId.has_value()) {
            solar_id_boucle = dsElecId->solarId();
          }

          if (solar_id_boucle == solar_id) {
            h2->second->SetBinContent(bx, by, norbits[fee_id][linkid]);
          }
        }
      }
    }
  }
}

void PhysicsTaskDigits::endOfCycle()
{
  QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
  for (int de = 100; de <= 1030; de++) {
    for (int i = 0; i < 2; i++) {
      auto ih = mHistogramOccupancyXY[i].find(de);
      ih = mHistogramOccupancyXY[i].find(de);
      if (ih == mHistogramOccupancyXY[i].end()) {
        continue;
      }
    }
  }

  // Compute Occupancy in GlobalHistograms by dividing Hits by Orbits and scaling
  mHistogramOrbits[0]->set(mHistogramNorbitsDE[0], mHistogramNorbitsDE[1]);
  mHistogramOccupancy[0]->set(mHistogramNhitsDE[0], mHistogramNhitsDE[1]);
  mHistogramOccupancy[0]->Divide(mHistogramOrbits[0]);
  mHistogramOccupancy[0]->Scale(1 / 87.5); // Converting Occupancy from NbHits/NbOrbits to MHz

  // Fill NOrbits, in Elec view, for electronics channels associated to readout pads (in order to then compute the Occupancy in Elec view, physically meaningful because in Elec view, each bin is a physical pad)
  for (uint16_t fee_id = 0; fee_id < MCH_FEEID_NUM; fee_id++) {

    // loop on FEE links and check if it corresponds to an existing SOLAR board
    for (uint8_t linkid = 0; linkid < 12; linkid++) {

      std::optional<uint16_t> solar_id = mFeeLink2SolarMapper(FeeLinkId{ fee_id, linkid });
      if (!solar_id.has_value()) {
        continue;
      }

      // loop on DS boards and check if it exists in the mapping
      for (uint8_t ds_addr = 0; ds_addr < 40; ds_addr++) {
        uint32_t de, dsid;
        std::optional<DsDetId> dsDetId =
          mElec2DetMapper(DsElecId{ solar_id.value(), static_cast<uint8_t>(ds_addr / 5), static_cast<uint8_t>(ds_addr % 5) });
        if (!dsDetId.has_value()) {
          continue;
        }
        de = dsDetId->deId();
        dsid = dsDetId->dsId();

        int xbin = fee_id * 12 * 40 + (linkid % 12) * 40 + ds_addr + 1;

        // loop on DS channels and check if it is associated to a readout pad
        for (int chan_addr = 0; chan_addr < 64; chan_addr++) {

          const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(de);
          int padid = segment.findPadByFEE(dsid, chan_addr);
          if (padid < 0) {
            continue;
          }

          int ybin = chan_addr + 1;
          mHistogramNorbitsElec->SetBinContent(xbin, ybin, norbits[fee_id][linkid]);
        }
      }
    }
  }

  // Compute Occupancy in Elec view by dividing Hits by Orbits and scaling
  mHistogramOccupancyElec->Reset();
  mHistogramOccupancyElec->Add(mHistogramNHitsElec);
  mHistogramOccupancyElec->Divide(mHistogramNorbitsElec);
  mHistogramOccupancyElec->Scale(1 / 87.5); // Converting Occupancy from NbHits/NbOrbits to MHz

  // Compute the Occupancy for individual DE XY histograms
  for (int de = 0; de < 1100; de++) {
    for (int i = 0; i < 2; i++) {
      auto h = mHistogramOccupancyXY[i].find(de);
      if ((h != mHistogramOccupancyXY[i].end()) && (h->second != NULL)) {
        auto hhits = mHistogramNhitsDE[i].find(de);
        auto horbits = mHistogramNorbitsDE[i].find(de);
        if ((hhits != mHistogramNhitsDE[i].end()) && (horbits != mHistogramNorbitsDE[i].end()) && (hhits->second != NULL) && (horbits->second != NULL)) {
          h->second->Divide(hhits->second, horbits->second);
          h->second->Scale(1 / 87.5); // Converting Occupancy from NbHits/NbOrbits to MHz
        }
      }
    }
  }

  {
    // Using OccupancyElec to get the mean occupancy per DE
    // By looking at each bin in NHits Elec histogram, getting the Elec info (fee, link, de) for each bin, computing the number of hits seen on a given DE and dividing by the number of bins
    auto h = mHistogramOccupancyElec;
    auto horbits = mHistogramNorbitsElec;
    auto h1 = mMeanOccupancyPerDE;
    if (h && h1 && horbits) {
      for (int de = 0; de < 1100; de++) {
        MeanOccupancyDE[de] = 0;
        NbinsDE[de] = 0;
      }
      for (int binx = 1; binx < h->GetXaxis()->GetNbins() + 1; binx++) {
        for (int biny = 1; biny < h->GetYaxis()->GetNbins() + 1; biny++) {

          int norbits = horbits->GetBinContent(binx, biny);
          if (norbits <= 0) {
            // no orbits detected for this channel, skip it
            continue;
          }

          // Getting Elec information based on the definition of x and y bins in ElecHistograms
          uint32_t ds_addr = (binx - 1) % 40;
          uint32_t linkid = ((binx - 1 - ds_addr) / 40) % 12;
          uint32_t fee_id = (binx - 1 - ds_addr - 40 * linkid) / (12 * 40);
          uint32_t de;
          std::optional<uint16_t> solar_id = mFeeLink2SolarMapper(FeeLinkId{ static_cast<uint16_t>(fee_id), static_cast<uint8_t>(linkid) });
          if (!solar_id.has_value()) {
            continue;
          }
          std::optional<DsDetId> dsDetId =
            mElec2DetMapper(DsElecId{ solar_id.value(), static_cast<uint8_t>(ds_addr / 5), static_cast<uint8_t>(ds_addr % 5) });
          if (!dsDetId.has_value()) {
            continue;
          }
          de = dsDetId->deId();

          MeanOccupancyDE[de] += h->GetBinContent(binx, biny);
          NbinsDE[de] += 1;
        }
      }

      for (int i = 0; i < 1100; i++) {
        if (NbinsDE[i] > 0) {
          MeanOccupancyDE[i] /= NbinsDE[i];
        }
        h1->SetBinContent(i + 1, MeanOccupancyDE[i]);
      }
    }
  }

  {
    // Using NHitsElec and Norbits to get the mean occupancy per DE on last cycle
    // Similarly, by looking at NHits and NOrbits Elec histogram, for each bin incrementing the corresponding DE total hits and total orbits, and from the values obtained at the end of a cycle compared to the beginning of the cycle, computing the occupancy during the cycle.
    auto hhits = mHistogramNHitsElec;
    auto horbits = mHistogramNorbitsElec;
    auto h1 = mMeanOccupancyPerDECycle;
    if (hhits && horbits && h1) {
      for (int de = 0; de < 1100; de++) {
        NewMeanNhitsDE[de] = 0;
        NewMeanNorbitsDE[de] = 0;
      }
      for (int binx = 1; binx < hhits->GetXaxis()->GetNbins() + 1; binx++) {
        for (int biny = 1; biny < hhits->GetYaxis()->GetNbins() + 1; biny++) {
          // Same procedure as above in getting Elec info
          uint32_t ds_addr = (binx - 1) % 40;
          uint32_t linkid = ((binx - 1 - ds_addr) / 40) % 12;
          uint32_t fee_id = (binx - 1 - ds_addr - 40 * linkid) / (12 * 40);
          uint32_t de;
          std::optional<uint16_t> solar_id = mFeeLink2SolarMapper(FeeLinkId{ static_cast<uint16_t>(fee_id), static_cast<uint8_t>(linkid) });
          if (!solar_id.has_value()) {
            continue;
          }
          std::optional<DsDetId> dsDetId =
            mElec2DetMapper(DsElecId{ solar_id.value(), static_cast<uint8_t>(ds_addr / 5), static_cast<uint8_t>(ds_addr % 5) });
          if (!dsDetId.has_value()) {
            continue;
          }
          de = dsDetId->deId();

          NewMeanNhitsDE[de] += hhits->GetBinContent(binx, biny);
          NewMeanNorbitsDE[de] += horbits->GetBinContent(binx, biny);
          NbinsDE[de] += 1;
        }
      }
      for (int i = 0; i < 1100; i++) {
        MeanOccupancyDECycle[i] = 0;
        if (NbinsDE[i] > 0) {
          NewMeanNhitsDE[i] /= NbinsDE[i];
          NewMeanNorbitsDE[i] /= NbinsDE[i];
        }
        if ((NewMeanNorbitsDE[i] - LastMeanNorbitsDE[i]) > 0) {
          MeanOccupancyDECycle[i] = (NewMeanNhitsDE[i] - LastMeanNhitsDE[i]) / (NewMeanNorbitsDE[i] - LastMeanNorbitsDE[i]) / 87.5; //Scaling to MHz
        }
        h1->SetBinContent(i + 1, MeanOccupancyDECycle[i]);
        LastMeanNhitsDE[i] = NewMeanNhitsDE[i];
        LastMeanNorbitsDE[i] = NewMeanNorbitsDE[i];
      }
    }
  }
}

void PhysicsTaskDigits::endOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;

#ifdef QC_MCH_SAVE_TEMP_ROOTFILE
  TFile f("/tmp/qc.root", "RECREATE");

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
        auto h = mHistogramOccupancyXY[i].find(de);
        if ((h != mHistogramOccupancyXY[i].end()) && (h->second != NULL)) {
          h->second->Write();
        }
      }
    }
  }

  mMeanOccupancyPerDE->Write();
  mMeanOccupancyPerDECycle->Write();

  mHistogramOrbits[0]->Write();
  mHistogramOccupancy[0]->Write();

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
