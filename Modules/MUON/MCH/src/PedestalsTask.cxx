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
/// \file   PedestalsTask.cxx
/// \author Andrea Ferrero
///

#include <TCanvas.h>
#include <TFile.h>

#include "Headers/RAWDataHeader.h"
#include "Framework/CallbackService.h"
#include "Framework/ControlService.h"
#include "Framework/Task.h"
#include "DPLUtils/DPLRawParser.h"
#include "QualityControl/QcInfoLogger.h"
#include "MCH/PedestalsTask.h"
#include "MCH/Helpers.h"
#include "DataFormatsMCH/DsChannelGroup.h"
#include "MCHMappingInterface/Segmentation.h"
#include "MCHMappingInterface/CathodeSegmentation.h"
#include "MCHRawElecMap/Mapper.h"
#include "MCHCalibration/PedestalChannel.h"
#ifdef MCH_HAS_MAPPING_FACTORY
#include "MCHMappingFactory/CreateSegmentation.h"
#endif
#include "MCHConstants/DetectionElements.h"

using namespace std;
using namespace o2::framework;

#define MCH_FFEID_MAX (31 * 2 + 1)

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{
PedestalsTask::PedestalsTask() : TaskInterface()
{
}

PedestalsTask::~PedestalsTask()
{
}

void PedestalsTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize PedestalsTask" << ENDM;

  mElec2DetMapper = o2::mch::raw::createElec2DetMapper<o2::mch::raw::ElectronicMapperGenerated>();
  mDet2ElecMapper = o2::mch::raw::createDet2ElecMapper<o2::mch::raw::ElectronicMapperGenerated>();
  mFeeLink2SolarMapper = o2::mch::raw::createFeeLink2SolarMapper<o2::mch::raw::ElectronicMapperGenerated>();
  mSolar2FeeLinkMapper = o2::mch::raw::createSolar2FeeLinkMapper<o2::mch::raw::ElectronicMapperGenerated>();

  const uint32_t nElecXbins = PedestalsTask::sMaxFeeId * PedestalsTask::sMaxLinkId * PedestalsTask::sMaxDsId;

  mHistogramStat = std::make_unique<TH2F>("Statistics_Elec", "Statistics", nElecXbins, 0, nElecXbins, 64, 0, 64);
  publishObject(mHistogramStat.get(), "colz", false);

  mHistogramPedestals = std::make_unique<TH2F>("Pedestals_Elec", "Pedestals", nElecXbins, 0, nElecXbins, 64, 0, 64);
  publishObject(mHistogramPedestals.get(), "colz", false);

  mHistogramNoise = std::make_unique<TH2F>("Noise_Elec", "Noise", nElecXbins, 0, nElecXbins, 64, 0, 64);
  publishObject(mHistogramNoise.get(), "colz", false);

  mHistogramBadChannels = std::make_unique<TH2F>("BadChannels_Elec", "Bad Channels", nElecXbins, 0, nElecXbins, 64, 0, 64);
  publishObject(mHistogramBadChannels.get(), "colz", false);

  // values averaged over detection elements
  mHistogramStatDE = std::make_unique<TH1F>("StatisticsPerDE", "Statistics per DE", getNumDE(), 0, getNumDE());
  publishObject(mHistogramStatDE.get(), "", false);

  mHistogramEmptyChannelsDE = std::make_unique<TH1F>("EmptyChannelsPerDE", "Empty Channels per DE", getNumDE(), 0, getNumDE());
  publishObject(mHistogramEmptyChannelsDE.get(), "", false);

  mHistogramPedestalsDE = std::make_unique<TH1F>("PedestalsPerDE", "Pedestals per DE", getNumDE(), 0, getNumDE());
  publishObject(mHistogramPedestalsDE.get(), "", false);

  mHistogramNoiseDE = std::make_unique<TH1F>("NoisePerDE", "Noise per DE", getNumDE(), 0, getNumDE());
  publishObject(mHistogramNoiseDE.get(), "", false);

  mHistogramBadChannelsDE = std::make_unique<TH1F>("BadChannelsPerDE", "Bad Channels per DE", getNumDE(), 0, getNumDE());
  publishObject(mHistogramBadChannelsDE.get(), "", false);

  std::string stname[2]{ "ST12", "ST345" };
  float scaleFactors[2]{ 5, 10 };
  for (int i = 0; i < 2; i++) {
    mHistogramStatMCH[i] = std::make_unique<GlobalHistogram>(fmt::format("Statistics_{}", stname[i]), "Statistics", i, scaleFactors[i]);
    mHistogramStatMCH[i]->init();
    publishObject(mHistogramStatMCH[i]->getHist(), "colz", false);

    mHistogramPedestalsMCH[i] = std::make_unique<GlobalHistogram>(fmt::format("Pedestals_{}", stname[i]), "Pedestals", i, scaleFactors[i]);
    mHistogramPedestalsMCH[i]->init();
    publishObject(mHistogramPedestalsMCH[i]->getHist(), "colz", false);

    mHistogramNoiseMCH[i] = std::make_unique<GlobalHistogram>(fmt::format("Noise_{}", stname[i]), "Noise", i, scaleFactors[i]);
    mHistogramNoiseMCH[i]->init();
    publishObject(mHistogramNoiseMCH[i]->getHist(), "colz", false);

    mHistogramBadChannelsMCH[i] = std::make_unique<GlobalHistogram>(fmt::format("BadChannels_{}", stname[i]), "Bad Channels", i, scaleFactors[i]);
    mHistogramBadChannelsMCH[i]->init();
    publishObject(mHistogramBadChannelsMCH[i]->getHist(), "colz", false);
  }

  for (int si = 0; si < 5; si++) {
    mHistogramNoiseDistribution[si] = std::make_unique<TH1F>(TString::Format("ST%d/Noise_Distr_ST%d", si + 1, si + 1),
                                                             TString::Format("Noise distribution (ST%d)", si + 1), 1000, 0, 10);
    publishObject(mHistogramNoiseDistribution[si].get(), "hist", false);
  }

  for (auto de : o2::mch::constants::deIdsForAllMCH) {
    for (int pi = 0; pi < 5; pi++) {
      auto hNoiseDE = std::make_shared<TH1F>(TString::Format("%sNoise_Distr_DE%03d_b_%d", getHistoPath(de).c_str(), de, pi),
                                             TString::Format("Noise distribution (DE%03d B, %d)", de, pi), 1000, 0, 10);
      mHistogramNoiseDistributionDE[pi][0].insert(make_pair(de, hNoiseDE));
      publishObject(hNoiseDE.get(), "hist", false);

      hNoiseDE = std::make_shared<TH1F>(TString::Format("%sNoise_Distr_DE%03d_nb_%d", getHistoPath(de).c_str(), de, pi),
                                        TString::Format("Noise distribution (DE%03d NB, %d)", de, pi), 1000, 0, 10);
      mHistogramNoiseDistributionDE[pi][1].insert(make_pair(de, hNoiseDE));
      publishObject(hNoiseDE.get(), "hist", false);
    }

    {
      auto hStatXY = std::make_shared<DetectorHistogram>(TString::Format("%sStatistics_XY_B_%03d", getHistoPath(de).c_str(), de),
                                                         TString::Format("Statistics (DE%03d B)", de), de, 0);
      mHistogramStatXY[0].insert(make_pair(de, hStatXY));
      publishObject(hStatXY->getHist(), "colz", false);

      auto hPedXY = std::make_shared<DetectorHistogram>(TString::Format("%sPedestals_XY_B_%03d", getHistoPath(de).c_str(), de),
                                                        TString::Format("Pedestals (DE%03d B)", de), de, 0);
      mHistogramPedestalsXY[0].insert(make_pair(de, hPedXY));
      publishObject(hPedXY->getHist(), "colz", false);

      auto hNoiseXY = std::make_shared<DetectorHistogram>(TString::Format("%sNoise_XY_B_%03d", getHistoPath(de).c_str(), de),
                                                          TString::Format("Noise (DE%03d B)", de), de, 0);
      mHistogramNoiseXY[0].insert(make_pair(de, hNoiseXY));
      publishObject(hNoiseXY->getHist(), "colz", false);

      auto hBadChannelsXY = std::make_shared<DetectorHistogram>(TString::Format("%sBadChannels_XY_B_%03d", getHistoPath(de).c_str(), de),
                                                                TString::Format("Bad Channels (DE%03d B)", de), de, 0);
      mHistogramBadChannelsXY[0].insert(make_pair(de, hBadChannelsXY));
      publishObject(hBadChannelsXY->getHist(), "colz", false);
    }
    {
      auto hStatXY = std::make_shared<DetectorHistogram>(TString::Format("%sStatistics_XY_NB_%03d", getHistoPath(de).c_str(), de),
                                                         TString::Format("Statistics (DE%03d NB)", de), de, 1);
      mHistogramStatXY[1].insert(make_pair(de, hStatXY));
      publishObject(hStatXY->getHist(), "colz", false);

      auto hPedXY = std::make_shared<DetectorHistogram>(TString::Format("%sPedestals_XY_NB_%03d", getHistoPath(de).c_str(), de),
                                                        TString::Format("Pedestals (DE%03d NB)", de), de, 1);
      mHistogramPedestalsXY[1].insert(make_pair(de, hPedXY));
      publishObject(hPedXY->getHist(), "colz", false);

      auto hNoiseXY = std::make_shared<DetectorHistogram>(TString::Format("%sNoise_XY_NB_%03d", getHistoPath(de).c_str(), de),
                                                          TString::Format("Noise (DE%03d NB)", de), de, 1);
      mHistogramNoiseXY[1].insert(make_pair(de, hNoiseXY));
      publishObject(hNoiseXY->getHist(), "colz", false);

      auto hBadChannelsXY = std::make_shared<DetectorHistogram>(TString::Format("%sBadChannels_XY_NB_%03d", getHistoPath(de).c_str(), de),
                                                                TString::Format("Bad Channels (DE%03d NB)", de), de, 1);
      mHistogramBadChannelsXY[1].insert(make_pair(de, hBadChannelsXY));
      publishObject(hBadChannelsXY->getHist(), "colz", false);
    }
  }

  mCanvasCheckerMessages = std::make_unique<TCanvas>("CheckerMessages", "Checker Messages", 800, 600);
  getObjectsManager()->startPublishing(mCanvasCheckerMessages.get());

  mPrintLevel = 0;
}

void PedestalsTask::startOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;
}

void PedestalsTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

bool PedestalsTask::checkPadMapping(uint16_t feeId, uint8_t linkId, uint8_t eLinkId, o2::mch::raw::DualSampaChannelId channel, int& deId, int& padId)
{
  uint16_t solarId = -1;
  int dsIddet = -1;
  padId = -1;

  o2::mch::raw::FeeLinkId feeLinkId{ feeId, linkId };

  if (auto opt = mFeeLink2SolarMapper(feeLinkId); opt.has_value()) {
    solarId = opt.value();
  }
  if (solarId < 0 || solarId > 1023) {
    return false;
  }

  o2::mch::raw::DsElecId dsElecId{ solarId, static_cast<uint8_t>(eLinkId / 5), static_cast<uint8_t>(eLinkId % 5) };

  if (auto opt = mElec2DetMapper(dsElecId); opt.has_value()) {
    o2::mch::raw::DsDetId dsDetId = opt.value();
    dsIddet = dsDetId.dsId();
    deId = dsDetId.deId();
  }

  if (deId < 0 || dsIddet < 0) {
    return false;
  }

  const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(deId);
  padId = segment.findPadByFEE(dsIddet, int(channel));

  if (padId < 0) {
    return false;
  }
  return true;
}

void PedestalsTask::PlotPedestal(uint16_t solarID, uint8_t dsID, uint8_t channel, double stat, double mean, double rms)
{
  std::optional<o2::mch::raw::FeeLinkId> feeLinkID = mSolar2FeeLinkMapper(solarID);
  if (!feeLinkID) {
    return;
  }

  auto feeId = feeLinkID->feeId();
  auto linkId = feeLinkID->linkId();

  int xbin = feeId * 12 * 40 + (linkId % 12) * 40 + dsID + 1;
  int ybin = channel + 1;

  mHistogramStat->SetBinContent(xbin, ybin, stat);
  mHistogramPedestals->SetBinContent(xbin, ybin, mean);
  mHistogramNoise->SetBinContent(xbin, ybin, rms);

  // PlotPedestalDE(solarID, dsID, channel, stat, mean, rms);
}

void PedestalsTask::PlotPedestalDE(uint16_t solarID, uint8_t dsID, uint8_t channel, double stat, double mean, double rms)
{
  o2::mch::raw::DsElecId dsElecId(solarID, dsID / 5, dsID % 5);
  std::optional<o2::mch::raw::DsDetId> dsDetId = mElec2DetMapper(dsElecId);
  if (!dsDetId) {
    return;
  }

  auto deId = dsDetId->deId();
  auto dsIddet = dsDetId->dsId();

  int padId = -1;
  const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(deId);
  padId = segment.findPadByFEE(dsIddet, int(channel));
  if (padId < 0) {
    return;
  }

  double padX = segment.padPositionX(padId);
  double padY = segment.padPositionY(padId);
  double padSizeX = segment.padSizeX(padId);
  double padSizeY = segment.padSizeY(padId);
  int cathode = segment.isBendingPad(padId) ? 0 : 1;

  // Fill the histograms for each detection element
  auto hStatXY = mHistogramStatXY[cathode].find(deId);
  if ((hStatXY != mHistogramStatXY[cathode].end()) && (hStatXY->second != NULL)) {
    hStatXY->second->Set(padX, padY, padSizeX, padSizeY, stat);
  }
  auto hPedXY = mHistogramPedestalsXY[cathode].find(deId);
  if ((hPedXY != mHistogramPedestalsXY[cathode].end()) && (hPedXY->second != NULL)) {
    hPedXY->second->Set(padX, padY, padSizeX, padSizeY, mean);
  }
  auto hNoiseXY = mHistogramNoiseXY[cathode].find(deId);
  if ((hNoiseXY != mHistogramNoiseXY[cathode].end()) && (hNoiseXY->second != NULL)) {
    hNoiseXY->second->Set(padX, padY, padSizeX, padSizeY, rms);
  }
}

void PedestalsTask::PlotBadChannel(uint16_t solarID, uint8_t dsID, uint8_t channel)
{
  static constexpr double sBinValMin = 0.000001;
  std::optional<o2::mch::raw::FeeLinkId> feeLinkID = mSolar2FeeLinkMapper(solarID);
  if (!feeLinkID) {
    return;
  }

  auto feeId = feeLinkID->feeId();
  auto linkId = feeLinkID->linkId();

  int xbin = feeId * 12 * 40 + (linkId % 12) * 40 + dsID + 1;
  int ybin = channel + 1;

  mHistogramBadChannels->SetBinContent(xbin, ybin, sBinValMin);

  // PlotBadChannelDE(solarID, dsID, channel);
}

void PedestalsTask::PlotBadChannelDE(uint16_t solarID, uint8_t dsID, uint8_t channel)
{
  o2::mch::raw::DsElecId dsElecId(solarID, dsID / 5, dsID % 5);
  std::optional<o2::mch::raw::DsDetId> dsDetId = mElec2DetMapper(dsElecId);
  if (!dsDetId) {
    return;
  }

  auto deId = dsDetId->deId();
  auto dsIddet = dsDetId->dsId();

  int padId = -1;
  const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(deId);
  padId = segment.findPadByFEE(dsIddet, int(channel));
  if (padId < 0) {
    return;
  }

  double padX = segment.padPositionX(padId);
  double padY = segment.padPositionY(padId);
  double padSizeX = segment.padSizeX(padId);
  double padSizeY = segment.padSizeY(padId);
  int cathode = segment.isBendingPad(padId) ? 0 : 1;

  // Fill the histograms for each detection element
  auto hXY = mHistogramBadChannelsXY[cathode].find(deId);
  if ((hXY != mHistogramBadChannelsXY[cathode].end()) && (hXY->second != NULL)) {
    hXY->second->Set(padX, padY, padSizeX, padSizeY, 0.000001);
  }
}

void PedestalsTask::processElecMaps()
{
  auto updateMean = [](double val, double mean, int N) { return (mean + (val - mean) / N); };

  for (int si = 0; si < 5; si++) {
    mHistogramNoiseDistribution[si]->Reset();
  }

  for (int pi = 0; pi < 5; pi++) {
    for (int i = 0; i < 2; i++) {
      auto ih = mHistogramNoiseDistributionDE[pi][i].begin();
      while (ih != mHistogramNoiseDistributionDE[pi][i].end()) {
        ih->second->Reset();
        ih++;
      }
    }
  }

  std::vector<double> nPadsPerDE(getNumDE());
  std::fill(nPadsPerDE.begin(), nPadsPerDE.end(), 0);

  std::vector<double> nBadPerDE(getNumDE());
  std::fill(nBadPerDE.begin(), nBadPerDE.end(), 0);

  std::vector<double> nEmptyPerDE(getNumDE());
  std::fill(nEmptyPerDE.begin(), nEmptyPerDE.end(), 0);

  std::vector<double> nEntriesPerDE(getNumDE());
  std::fill(nEntriesPerDE.begin(), nEntriesPerDE.end(), 0);

  std::vector<double> meanPedPerDE(getNumDE());
  std::fill(meanPedPerDE.begin(), meanPedPerDE.end(), 0);

  std::vector<double> meanNoisePerDE(getNumDE());
  std::fill(meanNoisePerDE.begin(), meanNoisePerDE.end(), 0);

  std::vector<double> meanStatPerDE(getNumDE());
  std::fill(meanStatPerDE.begin(), meanStatPerDE.end(), 0);

  //..............
  int nbinsx = mHistogramStat->GetXaxis()->GetNbins();
  int nbinsy = mHistogramStat->GetYaxis()->GetNbins();
  for (int i = 1; i <= nbinsx; i++) {
    int index = i - 1;
    int ds_addr = (index % 40);
    int link_id = (index / 40) % 12;
    int fee_id = index / (12 * 40);

    for (int j = 1; j <= nbinsy; j++) {
      int chan_addr = j - 1;
      int de = -1;
      int padId = -1;

      if (!checkPadMapping(fee_id, link_id, ds_addr, chan_addr, de, padId)) {
        continue;
      }
      if (de < 0) {
        continue;
      }

      int deId = getDEindex(de);
      if (deId < 0) {
        continue;
      }

      nPadsPerDE[deId] += 1;

      double stat = mHistogramStat->GetBinContent(i, j);
      float ped = mHistogramPedestals->GetBinContent(i, j);
      float noise = mHistogramNoise->GetBinContent(i, j);
      auto bad = mHistogramBadChannels->GetBinContent(i, j);

      // update mean values per detection element
      if (stat > 0) {
        if (bad > 0 && bad < 1) {
          nBadPerDE[deId] += 1;
        }

        nEntriesPerDE[deId] += 1;
        int N = nEntriesPerDE[deId];

        double ped = mHistogramPedestals->GetBinContent(i, j);
        double noise = mHistogramNoise->GetBinContent(i, j);

        meanStatPerDE[deId] = updateMean(stat, meanStatPerDE[deId], N);
        meanPedPerDE[deId] = updateMean(ped, meanPedPerDE[deId], N);
        meanNoisePerDE[deId] = updateMean(noise, meanNoisePerDE[deId], N);
      } else {
        nEmptyPerDE[deId] += 1;
      }

      // update 2D maps in detector coordinates
      const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(de);
      float padX = segment.padPositionX(padId);
      float padY = segment.padPositionY(padId);
      float padSizeX = segment.padSizeX(padId);
      float padSizeY = segment.padSizeY(padId);
      int cathode = segment.isBendingPad(padId) ? 0 : 1;

      // Fill the histograms for each detection element
      auto hXY = mHistogramBadChannelsXY[cathode].find(de);
      if ((hXY != mHistogramBadChannelsXY[cathode].end()) && (hXY->second != NULL)) {
        hXY->second->Set(padX, padY, padSizeX, padSizeY, bad);
      }

      hXY = mHistogramStatXY[cathode].find(de);
      if ((hXY != mHistogramStatXY[cathode].end()) && (hXY->second != NULL)) {
        hXY->second->Set(padX, padY, padSizeX, padSizeY, stat);
      }

      hXY = mHistogramPedestalsXY[cathode].find(de);
      if ((hXY != mHistogramPedestalsXY[cathode].end()) && (hXY->second != NULL)) {
        hXY->second->Set(padX, padY, padSizeX, padSizeY, ped);
      }

      hXY = mHistogramNoiseXY[cathode].find(de);
      if ((hXY != mHistogramNoiseXY[cathode].end()) && (hXY->second != NULL)) {
        hXY->second->Set(padX, padY, padSizeX, padSizeY, noise);
      }

      // fill 1D noise distributions
      if (noise >= 0.001) {
        float szmax = padSizeX;
        if (szmax < padSizeY) {
          szmax = padSizeY;
        }

        int szid = 0;
        if (fabs(szmax - 2.5) < 0.001) {
          szid = 1;
        } else if (fabs(szmax - 5.0) < 0.001) {
          szid = 2;
        } else if (fabs(szmax - 10.0) < 0.001) {
          szid = 3;
        }

        auto hNoiseDE = mHistogramNoiseDistributionDE[szid][cathode].find(de);
        if ((hNoiseDE != mHistogramNoiseDistributionDE[szid][cathode].end()) && (hNoiseDE->second != NULL)) {
          hNoiseDE->second->Fill(noise);
        }

        int si = (de - 100) / 200;
        if (si >= 0 && si < 5) {
          mHistogramNoiseDistribution[si]->Fill(noise);
        }
      }
    }
  }

  // update global detector plots
  for (int i = 0; i < 2; i++) {
    mHistogramBadChannelsMCH[i]->set(mHistogramBadChannelsXY[0], mHistogramBadChannelsXY[1], true);
    mHistogramStatMCH[i]->set(mHistogramStatXY[0], mHistogramStatXY[1], true);
    mHistogramPedestalsMCH[i]->set(mHistogramPedestalsXY[0], mHistogramPedestalsXY[1], true);
    mHistogramNoiseMCH[i]->set(mHistogramNoiseXY[0], mHistogramNoiseXY[1], true);
  }

  // update 1-D plots with quantities averaged over detection elements
  for (int i = 0; i < getNumDE(); i++) {
    mHistogramStatDE->SetBinContent(i + 1, meanStatPerDE[i]);
    mHistogramPedestalsDE->SetBinContent(i + 1, meanPedPerDE[i]);
    mHistogramNoiseDE->SetBinContent(i + 1, meanNoisePerDE[i]);

    if (nPadsPerDE[i] > 0) {
      mHistogramEmptyChannelsDE->SetBinContent(i + 1, nEmptyPerDE[i] / nPadsPerDE[i]);
      mHistogramBadChannelsDE->SetBinContent(i + 1, nBadPerDE[i] / nPadsPerDE[i]);
    }
  }
}

void PedestalsTask::monitorDataPedestals(o2::framework::ProcessingContext& ctx)
{
  auto pedestals = ctx.inputs().get<gsl::span<o2::mch::calibration::PedestalChannel>>("pedestals");
  for (auto& p : pedestals) {
    if (p.mEntries == 0) {
      continue;
    }

    PlotPedestal(p.dsChannelId.getSolarId(),
                 p.dsChannelId.getElinkId(),
                 p.dsChannelId.getChannel(),
                 p.mEntries,
                 p.mPedestal,
                 p.getRms());
  }
}

void PedestalsTask::monitorDataDigits(o2::framework::ProcessingContext& ctx)
{
  auto digits = ctx.inputs().get<gsl::span<o2::mch::calibration::PedestalDigit>>("digits");
  mPedestalData.fill(digits);

  for (auto& p : mPedestalData) {
    if (p.mEntries == 0) {
      continue;
    }

    PlotPedestal(p.dsChannelId.getSolarId(),
                 p.dsChannelId.getElinkId(),
                 p.dsChannelId.getChannel(),
                 p.mEntries,
                 p.mPedestal,
                 p.getRms());
  }
}

void PedestalsTask::monitorDataBadChannels(o2::framework::ProcessingContext& ctx)
{
  static constexpr double sBinValMin = 0.000001;
  auto badChannels = ctx.inputs().get<gsl::span<o2::mch::DsChannelId>>("badchan");

  // Initialize the bad channels map by setting to 1 all the bins that have non-zero statistics
  int nbinsx = mHistogramStat->GetXaxis()->GetNbins();
  int nbinsy = mHistogramStat->GetYaxis()->GetNbins();
  for (int i = 1; i <= nbinsx; i++) {
    for (int j = 1; j <= nbinsy; j++) {
      auto stat = mHistogramStat->GetBinContent(i, j);
      if (stat == 0) {
        continue;
      }
      mHistogramBadChannels->SetBinContent(i, j, 1);
    }
  }

  // update the bad channels maps with the table received from the calibrator
  for (auto& bc : badChannels) {
    PlotBadChannel(bc.getSolarId(), bc.getElinkId(), bc.getChannel());
  }
}

void PedestalsTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  if (ctx.inputs().isValid("pedestals")) {
    monitorDataPedestals(ctx);
  } else if (ctx.inputs().isValid("digits")) {
    monitorDataDigits(ctx);
  }

  if (ctx.inputs().isValid("badchan")) {
    auto input = ctx.inputs().getDataRefByString("badchan");
    auto subspecification = framework::DataRefUtils::getHeader<header::DataHeader*>(input)->subSpecification;
    if (subspecification != 0xDEADBEEF) {
      monitorDataBadChannels(ctx);
    }
  }
}

static void fillGlobalHistograms(std::array<std::map<int, std::shared_ptr<DetectorHistogram>>, 2>& hDE,
                                 std::array<std::unique_ptr<GlobalHistogram>, 2>& hGlobal, bool doAverage)
{
  hGlobal[0]->set(hDE[0], hDE[1], doAverage);
  hGlobal[1]->set(hDE[0], hDE[1], doAverage);
}

void PedestalsTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;

  processElecMaps();

  fillGlobalHistograms(mHistogramBadChannelsXY, mHistogramBadChannelsMCH, true);
  fillGlobalHistograms(mHistogramStatXY, mHistogramStatMCH, true);
  fillGlobalHistograms(mHistogramPedestalsXY, mHistogramPedestalsMCH, true);
  fillGlobalHistograms(mHistogramNoiseXY, mHistogramNoiseMCH, true);
}

void PedestalsTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void PedestalsTask::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Devel) << "Reseting the histogram" << ENDM;
  mPedestalData.reset();

  for (auto h : mAllHistograms) {
    h->Reset("ICES");
  }
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
