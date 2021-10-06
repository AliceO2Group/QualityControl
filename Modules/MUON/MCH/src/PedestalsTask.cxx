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
  QcInfoLogger::GetInstance() << "initialize PedestalsTask" << AliceO2::InfoLogger::InfoLogger::endm;

  mSolar2FeeLinkMapper = o2::mch::raw::createSolar2FeeLinkMapper<o2::mch::raw::ElectronicMapperGenerated>();
  mElec2DetMapper = o2::mch::raw::createElec2DetMapper<o2::mch::raw::ElectronicMapperGenerated>();

  mHistogramPedestals = new TH2F("Pedestals", "Pedestals",
                                 (MCH_FFEID_MAX + 1) * 12 * 40, 0, (MCH_FFEID_MAX + 1) * 12 * 40, 64, 0, 64);
  getObjectsManager()->startPublishing(mHistogramPedestals);
  mHistogramPedestalsMCH = new GlobalHistogram("Pedestals_AllDE", "Pedestals");
  mHistogramPedestalsMCH->init();
  getObjectsManager()->startPublishing(mHistogramPedestalsMCH);

  mHistogramNoise = new TH2F("QcMuonChambers_Noise", "Noise",
                             (MCH_FFEID_MAX + 1) * 12 * 40, 0, (MCH_FFEID_MAX + 1) * 12 * 40, 64, 0, 64);
  getObjectsManager()->startPublishing(mHistogramNoise);
  mHistogramNoiseMCH = new GlobalHistogram("Noise_AllDE", "Noise");
  mHistogramNoiseMCH->init();
  getObjectsManager()->startPublishing(mHistogramNoiseMCH);

  for (auto de : o2::mch::raw::deIdsForAllMCH) {
    TH2F* hPedDE = new TH2F(TString::Format("Pedestals_Elec_DE%03d", de),
                            TString::Format("Pedestals (DE%03d)", de), 2000, 0, 2000, 64, 0, 64);
    mHistogramPedestalsDE.insert(make_pair(de, hPedDE));
    TH2F* hNoiseDE = new TH2F(TString::Format("Noise_Elec_DE%03d", de),
                              TString::Format("Noise (DE%03d)", de), 2000, 0, 2000, 64, 0, 64);
    mHistogramNoiseDE.insert(make_pair(de, hNoiseDE));

    for (int pi = 0; pi < 5; pi++) {
      TH1F* hNoiseDE = new TH1F(TString::Format("Noise_Distr_DE%03d_b_%d", de, pi),
                                TString::Format("Noise distribution (DE%03d B, %d)", de, pi), 1000, 0, 10);
      mHistogramNoiseDistributionDE[pi][0].insert(make_pair(de, hNoiseDE));
      hNoiseDE = new TH1F(TString::Format("Noise_Distr_DE%03d_nb_%d", de, pi),
                          TString::Format("Noise distribution (DE%03d NB, %d)", de, pi), 1000, 0, 10);
      mHistogramNoiseDistributionDE[pi][1].insert(make_pair(de, hNoiseDE));
    }

    {
      DetectorHistogram* hPedXY = new DetectorHistogram(TString::Format("%sPedestals_%03d_B", getHistoPath(de).c_str(), de),
                                                        TString::Format("Pedestals (DE%03d B)", de), de);
      mHistogramPedestalsXY[0].insert(make_pair(de, hPedXY));
      getObjectsManager()->startPublishing(hPedXY);
      DetectorHistogram* hNoiseXY = new DetectorHistogram(TString::Format("%sNoise_%03d_B", getHistoPath(de).c_str(), de),
                                                          TString::Format("Noise (DE%03d B)", de), de);
      mHistogramNoiseXY[0].insert(make_pair(de, hNoiseXY));
      getObjectsManager()->startPublishing(hNoiseXY);
    }
    {
      DetectorHistogram* hPedXY = new DetectorHistogram(TString::Format("%sPedestals_%03d_NB", getHistoPath(de).c_str(), de),
                                                        TString::Format("Pedestals (DE%03d NB)", de), de);
      mHistogramPedestalsXY[1].insert(make_pair(de, hPedXY));
      getObjectsManager()->startPublishing(hPedXY);
      DetectorHistogram* hNoiseXY = new DetectorHistogram(TString::Format("%sNoise_%03d_NB", getHistoPath(de).c_str(), de),
                                                          TString::Format("Noise (DE%03d NB)", de), de);
      mHistogramNoiseXY[1].insert(make_pair(de, hNoiseXY));
      getObjectsManager()->startPublishing(hNoiseXY);
    }
  }

  mPrintLevel = 0;
}

void PedestalsTask::startOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void PedestalsTask::startOfCycle()
{
  QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void PedestalsTask::fill_noise_distributions()
{
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
      }
    }
  }
}

void PedestalsTask::save_histograms()
{
  TFile f("mch-qc-pedestals.root", "RECREATE");
  fill_noise_distributions();

  mHistogramPedestalsMCH->Write();
  mHistogramNoiseMCH->Write();

  mHistogramNoise->Write();
  mHistogramPedestals->Write();

  for (int i = 0; i < 2; i++) {
    auto ih = mHistogramPedestalsXY[i].begin();
    while (ih != mHistogramPedestalsXY[i].end()) {
      ih->second->Write();
      ih++;
    }
  }
  for (int i = 0; i < 2; i++) {
    auto ih = mHistogramNoiseXY[i].begin();
    while (ih != mHistogramNoiseXY[i].end()) {
      ih->second->Write();
      ih++;
    }
  }
  {
    auto ih = mHistogramPedestalsDE.begin();
    while (ih != mHistogramPedestalsDE.end()) {
      ih->second->Write();
      ih++;
    }
  }
  {
    auto ih = mHistogramNoiseDE.begin();
    while (ih != mHistogramNoiseDE.end()) {
      ih->second->Write();
      ih++;
    }
  }
  for (int pi = 0; pi < 5; pi++) {
    for (int i = 0; i < 2; i++) {
      auto ih = mHistogramNoiseDistributionDE[pi][i].begin();
      while (ih != mHistogramNoiseDistributionDE[pi][i].end()) {
        ih->second->Write();
        ih++;
      }
    }
  }

  f.Close();
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
  QcInfoLogger::GetInstance() << "Plotting pedestals" << AliceO2::InfoLogger::InfoLogger::endm;
  using ChannelPedestal = o2::mch::calibration::MCHChannelCalibrator::ChannelPedestal;

  auto pedestals = ctx.inputs().get<gsl::span<ChannelPedestal>>("pedestals");
  for (auto& p : pedestals) {
    auto dsChID = p.mDsChId;
    double mean = p.mPedMean;
    double rms = p.mPedRms;

    auto solarID = dsChID.getSolarId();
    auto dsID = dsChID.getDsId();
    auto channel = dsChID.getChannel();

    PlotPedestal(solarID, dsID, channel, mean, rms);
  }
}

void PedestalsTask::monitorDataDigits(o2::framework::ProcessingContext& ctx)
{
  auto digits = ctx.inputs().get<gsl::span<o2::mch::calibration::PedestalDigit>>("digits");
  mPedestalProcessor.process(digits);

  auto pedestals = mPedestalProcessor.getPedestals();
  for (auto& p : pedestals) {
    auto& pMat = p.second;
    for (size_t dsId = 0; dsId < pMat.size(); dsId++) {
      auto& pRow = pMat[dsId];
      for (size_t ch = 0; ch < pRow.size(); ch++) {
        auto& pRecord = pRow[ch];

        if (pRecord.mEntries == 0) {
          continue;
        }

        PlotPedestal(p.first, dsId, ch, pRecord.mPedestal, pRecord.getRms());
      }
    }
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

void PedestalsTask::endOfCycle()
{
  QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;

  mHistogramPedestalsMCH->set(mHistogramPedestalsXY[0], mHistogramPedestalsXY[1], true);
  mHistogramNoiseMCH->set(mHistogramNoiseXY[0], mHistogramNoiseXY[1], true);
}

void PedestalsTask::endOfActivity(Activity& /*activity*/)
{
  printf("PedestalsTask::endOfActivity() called\n");
  QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;

#ifdef QC_MCH_SAVE_TEMP_ROOTFILE
  save_histograms();
#endif
}

void PedestalsTask::reset()
{
  // clean all the monitor objects here

  QcInfoLogger::GetInstance() << "Reseting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;
  mPedestalProcessor.reset();
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
