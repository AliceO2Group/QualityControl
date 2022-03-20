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
#include <TH1.h>
#include <TH2.h>
#include <TFile.h>

#include "Headers/RAWDataHeader.h"
#include "Framework/CallbackService.h"
#include "Framework/ControlService.h"
#include "Framework/Task.h"
#include "DPLUtils/DPLRawParser.h"
#include "QualityControl/QcInfoLogger.h"
#include "MCH/PedestalsTask.h"
#include "DataFormatsMCH/DsChannelGroup.h"
#include "MCHMappingInterface/Segmentation.h"
#include "MCHMappingInterface/CathodeSegmentation.h"
#include "MCHRawElecMap/Mapper.h"
#include "MCHCalibration/PedestalChannel.h"
#ifdef MCH_HAS_MAPPING_FACTORY
#include "MCHMappingFactory/CreateSegmentation.h"
#endif

using namespace std;
using namespace o2::framework;

//#define QC_MCH_SAVE_TEMP_ROOTFILE 1

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
  ILOG(Info, Support) << "initialize PedestalsTask" << AliceO2::InfoLogger::InfoLogger::endm;

  mSaveToRootFile = false;
  if (auto param = mCustomParameters.find("SaveToRootFile"); param != mCustomParameters.end()) {
    if (param->second == "true" || param->second == "True" || param->second == "TRUE") {
      mSaveToRootFile = true;
    }
  }

  mSolar2FeeLinkMapper = o2::mch::raw::createSolar2FeeLinkMapper<o2::mch::raw::ElectronicMapperGenerated>();
  mElec2DetMapper = o2::mch::raw::createElec2DetMapper<o2::mch::raw::ElectronicMapperGenerated>();

  const uint32_t nElecXbins = PedestalsTask::sMaxFeeId * PedestalsTask::sMaxLinkId * PedestalsTask::sMaxDsId;

  mHistogramPedestals = std::make_shared<TH2F>("Pedestals_Elec", "Pedestals", nElecXbins, 0, nElecXbins, 64, 0, 64);
  mHistogramPedestals->SetOption("colz");
  mAllHistograms.push_back(mHistogramPedestals.get());
  if (!mSaveToRootFile) {
    getObjectsManager()->startPublishing(mHistogramPedestals.get());
  }

  mHistogramNoise = std::make_shared<TH2F>("Noise_Elec", "Noise", nElecXbins, 0, nElecXbins, 64, 0, 64);
  mHistogramNoise->SetOption("colz");
  mAllHistograms.push_back(mHistogramNoise.get());
  if (!mSaveToRootFile) {
    getObjectsManager()->startPublishing(mHistogramNoise.get());
  }

  std::string stname[2]{ "ST12", "ST345" };
  for (int i = 0; i < 2; i++) {
    mHistogramPedestalsMCH[i] = std::make_shared<GlobalHistogram>(fmt::format("Pedestals_{}", stname[i]), "Pedestals", i);
    mHistogramPedestalsMCH[i]->init();
    mHistogramPedestalsMCH[i]->SetOption("colz");
    mAllHistograms.push_back(mHistogramPedestalsMCH[i].get());
    if (!mSaveToRootFile) {
      getObjectsManager()->startPublishing(mHistogramPedestalsMCH[i].get());
    }

    mHistogramNoiseMCH[i] = std::make_shared<GlobalHistogram>(fmt::format("Noise_{}", stname[i]), "Noise", i);
    mHistogramNoiseMCH[i]->init();
    mHistogramNoiseMCH[i]->SetOption("colz");
    mAllHistograms.push_back(mHistogramNoiseMCH[i].get());
    if (!mSaveToRootFile) {
      getObjectsManager()->startPublishing(mHistogramNoiseMCH[i].get());
    }
  }

  for (int si = 0; si < 5; si++) {
    mHistogramNoiseDistribution[si] = std::make_shared<TH1F>(TString::Format("ST%d/Noise_Distr_ST%d", si + 1, si + 1),
                                                             TString::Format("Noise distribution (ST%d)", si + 1), 1000, 0, 10);
    mAllHistograms.push_back(mHistogramNoiseDistribution[si].get());
    if (!mSaveToRootFile) {
      getObjectsManager()->startPublishing(mHistogramNoiseDistribution[si].get());
    }
  }

  for (auto de : o2::mch::raw::deIdsForAllMCH) {
    auto hPedDE = std::make_shared<TH2F>(TString::Format("%sPedestals_Elec_DE%03d", getHistoPath(de).c_str(), de),
                                         TString::Format("Pedestals (DE%03d)", de), 2000, 0, 2000, 64, 0, 64);
    mHistogramPedestalsDE.insert(make_pair(de, hPedDE));
    hPedDE->SetOption("colz");
    mAllHistograms.push_back(hPedDE.get());

    auto hNoiseDE = std::make_shared<TH2F>(TString::Format("%sNoise_Elec_DE%03d", getHistoPath(de).c_str(), de),
                                           TString::Format("Noise (DE%03d)", de), 2000, 0, 2000, 64, 0, 64);
    mHistogramNoiseDE.insert(make_pair(de, hNoiseDE));
    hNoiseDE->SetOption("colz");
    mAllHistograms.push_back(hNoiseDE.get());

    for (int pi = 0; pi < 5; pi++) {
      auto hNoiseDE = std::make_shared<TH1F>(TString::Format("%sNoise_Distr_DE%03d_b_%d", getHistoPath(de).c_str(), de, pi),
                                             TString::Format("Noise distribution (DE%03d B, %d)", de, pi), 1000, 0, 10);
      mHistogramNoiseDistributionDE[pi][0].insert(make_pair(de, hNoiseDE));
      hNoiseDE->SetOption("hist");
      mAllHistograms.push_back(hNoiseDE.get());
      if (!mSaveToRootFile) {
        getObjectsManager()->startPublishing(hNoiseDE.get());
      }

      hNoiseDE = std::make_shared<TH1F>(TString::Format("%sNoise_Distr_DE%03d_nb_%d", getHistoPath(de).c_str(), de, pi),
                                        TString::Format("Noise distribution (DE%03d NB, %d)", de, pi), 1000, 0, 10);
      mHistogramNoiseDistributionDE[pi][1].insert(make_pair(de, hNoiseDE));
      hNoiseDE->SetOption("hist");
      mAllHistograms.push_back(hNoiseDE.get());
      if (!mSaveToRootFile) {
        getObjectsManager()->startPublishing(hNoiseDE.get());
      }
    }

    {
      auto hPedXY = std::make_shared<DetectorHistogram>(TString::Format("%sPedestals_%03d_B", getHistoPath(de).c_str(), de),
                                                        TString::Format("Pedestals (DE%03d B)", de), de);
      mHistogramPedestalsXY[0].insert(make_pair(de, hPedXY));
      mAllHistograms.push_back(hPedXY->getHist());
      if (!mSaveToRootFile) {
        getObjectsManager()->startPublishing(hPedXY->getHist());
      }

      auto hNoiseXY = std::make_shared<DetectorHistogram>(TString::Format("%sNoise_%03d_B", getHistoPath(de).c_str(), de),
                                                          TString::Format("Noise (DE%03d B)", de), de);
      mHistogramNoiseXY[0].insert(make_pair(de, hNoiseXY));
      mAllHistograms.push_back(hNoiseXY->getHist());
      if (!mSaveToRootFile) {
        getObjectsManager()->startPublishing(hNoiseXY->getHist());
      }
    }
    {
      auto hPedXY = std::make_shared<DetectorHistogram>(TString::Format("%sPedestals_%03d_NB", getHistoPath(de).c_str(), de),
                                                        TString::Format("Pedestals (DE%03d NB)", de), de);
      mHistogramPedestalsXY[1].insert(make_pair(de, hPedXY));
      mAllHistograms.push_back(hPedXY->getHist());
      if (!mSaveToRootFile) {
        getObjectsManager()->startPublishing(hPedXY->getHist());
      }

      auto hNoiseXY = std::make_shared<DetectorHistogram>(TString::Format("%sNoise_%03d_NB", getHistoPath(de).c_str(), de),
                                                          TString::Format("Noise (DE%03d NB)", de), de);
      mHistogramNoiseXY[1].insert(make_pair(de, hNoiseXY));
      mAllHistograms.push_back(hNoiseXY->getHist());
      if (!mSaveToRootFile) {
        getObjectsManager()->startPublishing(hNoiseXY->getHist());
      }
    }
  }

  mPrintLevel = 0;
}

void PedestalsTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void PedestalsTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void PedestalsTask::fill_noise_distributions()
{
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

  auto ih = mHistogramNoiseDE.begin();
  for (; ih != mHistogramNoiseDE.end(); ih++) {
    int de = ih->first;
    if (!ih->second)
      continue;
    if (ih->second->GetEntries() < 1)
      continue;

    for (int bi = 0; bi < ih->second->GetXaxis()->GetNbins(); bi++) {
      for (int ci = 0; ci < ih->second->GetYaxis()->GetNbins(); ci++) {
        float noise = ih->second->GetBinContent(bi + 1, ci + 1);
        if (noise < 0.001) {
          continue;
        }

        int dsid = bi;
        int chan_addr = ci;

        const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(de);
        int padid = segment.findPadByFEE(dsid, chan_addr);
        if (padid < 0) {
          continue;
        }

        float padSizeX = segment.padSizeX(padid);
        float padSizeY = segment.padSizeY(padid);
        int cathode = segment.isBendingPad(padid) ? 0 : 1;

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
}

void PedestalsTask::PlotPedestal(uint16_t solarID, uint8_t dsID, uint8_t channel, double mean, double rms)
{
  std::optional<o2::mch::raw::FeeLinkId> feeLinkID = mSolar2FeeLinkMapper(solarID);
  if (!feeLinkID) {
    return;
  }

  auto feeId = feeLinkID->feeId();
  auto linkId = feeLinkID->linkId();

  int xbin = feeId * 12 * 40 + (linkId % 12) * 40 + dsID + 1;
  int ybin = channel + 1;

  mHistogramPedestals->SetBinContent(xbin, ybin, mean);
  mHistogramNoise->SetBinContent(xbin, ybin, rms);

  PlotPedestalDE(solarID, dsID, channel, mean, rms);
}

void PedestalsTask::PlotPedestalDE(uint16_t solarID, uint8_t dsID, uint8_t channel, double mean, double rms)
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

  auto hPed = mHistogramPedestalsDE.find(deId);
  if ((hPed != mHistogramPedestalsDE.end()) && (hPed->second != NULL)) {
    int binx = hPed->second->GetXaxis()->FindBin(dsIddet + 0.5);
    int biny = hPed->second->GetYaxis()->FindBin(channel + 0.5);
    hPed->second->SetBinContent(binx, biny, mean);
  }

  auto hNoise = mHistogramNoiseDE.find(deId);
  if ((hNoise != mHistogramNoiseDE.end()) && (hNoise->second != NULL)) {
    int binx = hPed->second->GetXaxis()->FindBin(dsIddet + 0.5);
    int biny = hPed->second->GetYaxis()->FindBin(channel + 0.5);
    hNoise->second->SetBinContent(binx, biny, rms);
  }

  double padX = segment.padPositionX(padId);
  double padY = segment.padPositionY(padId);
  double padSizeX = segment.padSizeX(padId);
  double padSizeY = segment.padSizeY(padId);
  int cathode = segment.isBendingPad(padId) ? 0 : 1;

  // Fill the histograms for each detection element
  auto hPedXY = mHistogramPedestalsXY[cathode].find(deId);
  if ((hPedXY != mHistogramPedestalsXY[cathode].end()) && (hPedXY->second != NULL)) {
    hPedXY->second->Set(padX, padY, padSizeX, padSizeY, mean);
  }
  auto hNoiseXY = mHistogramNoiseXY[cathode].find(deId);
  if ((hNoiseXY != mHistogramNoiseXY[cathode].end()) && (hNoiseXY->second != NULL)) {
    hNoiseXY->second->Set(padX, padY, padSizeX, padSizeY, rms);
  }
}

void PedestalsTask::monitorDataPedestals(o2::framework::ProcessingContext& ctx)
{
  ILOG(Info, Support) << "Plotting pedestals" << AliceO2::InfoLogger::InfoLogger::endm;

  auto pedestals = ctx.inputs().get<gsl::span<o2::mch::calibration::PedestalChannel>>("pedestals");
  for (auto& p : pedestals) {
    auto dsChID = p.dsChannelId;
    double mean = p.mPedestal;
    double rms = p.getRms();

    auto solarID = dsChID.getSolarId();
    auto dsID = dsChID.getDsId();
    auto channel = dsChID.getChannel();

    PlotPedestal(solarID, dsID, channel, mean, rms);
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
                 p.dsChannelId.getDsId(),
                 p.dsChannelId.getChannel(),
                 p.mPedestal,
                 p.getRms());
  }
}

void PedestalsTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  for (auto&& input : ctx.inputs()) {
    if (input.spec->binding == "pedestals")
      monitorDataPedestals(ctx);
    if (input.spec->binding == "digits")
      monitorDataDigits(ctx);
  }
}

void PedestalsTask::writeHistos()
{
  if (!mSaveToRootFile) {
    return;
  }

  TFile f("mch-qc-pedestals.root", "RECREATE");
  for (auto h : mAllHistograms) {
    h->Write();
  }
  f.Close();
}

void PedestalsTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;

  fill_noise_distributions();

  mHistogramPedestalsMCH[0]->set(mHistogramPedestalsXY[0], mHistogramPedestalsXY[1], true);
  mHistogramNoiseMCH[0]->set(mHistogramNoiseXY[0], mHistogramNoiseXY[1], true);

  mHistogramPedestalsMCH[1]->set(mHistogramPedestalsXY[0], mHistogramPedestalsXY[1], true);
  mHistogramNoiseMCH[1]->set(mHistogramNoiseXY[0], mHistogramNoiseXY[1], true);

  writeHistos();
}

void PedestalsTask::endOfActivity(Activity& /*activity*/)
{
  printf("PedestalsTask::endOfActivity() called\n");
  ILOG(Info, Support) << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;

  fill_noise_distributions();

  writeHistos();
}

void PedestalsTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Reseting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;
  mPedestalData.reset();
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
