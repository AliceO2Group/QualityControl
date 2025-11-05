// Copyright 2019-2025 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General
// Public License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   AgingLaserPostProcTask.cxx
/// \author Andreas Molander <andreas.molander@cern.ch>, Jakub Muszyński <jakub.milosz.muszynski@cern.ch>

#include "FT0/AgingLaserPostProcTask.h"

#include "Common/Utils.h"
#include "DataFormatsFIT/DeadChannelMap.h"
#include "FITCommon/HelperHist.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/QcInfoLogger.h"

#include <TH2.h>
#include <TF1.h>
#include <TMath.h>

#include <algorithm>
#include <memory>
#include <numeric>
#include <string>

using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control_modules::fit;

namespace o2::quality_control_modules::ft0
{

AgingLaserPostProcTask::~AgingLaserPostProcTask() = default;

void AgingLaserPostProcTask::configure(const boost::property_tree::ptree& cfg)
{
  mReset = o2::quality_control_modules::common::getFromConfig<bool>(mCustomParameters, "reset", false);
  ILOG(Info, Support) << "Is this a reset run: " << mReset << ENDM;

  mAgingLaserPath = o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "agingLaserTaskPath", mAgingLaserPath);
  mAgingLaserPostProcPath = o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "agingLaserPostProcPath", mAgingLaserPostProcPath);

  mDetectorChIDs.resize(208);
  std::iota(mDetectorChIDs.begin(), mDetectorChIDs.end(), 0);
  const std::string detSkip =
    o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "ignoreDetectorChannels", "");
  if (!detSkip.empty()) {
    auto toSkip = fit::helper::parseParameters<uint8_t>(detSkip, ",");
    for (auto s : toSkip) {
      mDetectorChIDs.erase(std::remove(mDetectorChIDs.begin(), mDetectorChIDs.end(), s),
                           mDetectorChIDs.end());
    }
  }
  mUseDeadChannelMap = o2::quality_control_modules::common::getFromConfig<bool>(mCustomParameters, "useDeadChannelMap", true);
  if (mUseDeadChannelMap) {
    o2::fit::DeadChannelMap* dcm = retrieveConditionAny<o2::fit::DeadChannelMap>("FT0/Calib/DeadChannelMap");
    if (!dcm) {
      ILOG(Error) << "Could not retrieve DeadChannelMap from CCDB!" << ENDM;
    } else {
      for (unsigned chId = 0; chId < dcm->map.size(); chId++) {
        if (!dcm->isChannelAlive(chId)) {
          mDetectorChIDs.erase(std::remove(mDetectorChIDs.begin(), mDetectorChIDs.end(), chId), mDetectorChIDs.end());
        }
      }
    }
  }

  // Reference channels: default 208–210, then remove skipped ones
  mReferenceChIDs.clear();
  for (uint8_t ch = 208; ch < 211; ++ch) {
    mReferenceChIDs.push_back(ch);
  }
  const std::string refSkip =
    o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "ignoreRefChannels", "");
  if (!refSkip.empty()) {
    auto toSkip = fit::helper::parseParameters<uint8_t>(refSkip, ",");
    for (auto s : toSkip) {
      mReferenceChIDs.erase(std::remove(mReferenceChIDs.begin(), mReferenceChIDs.end(), s),
                            mReferenceChIDs.end());
    }
  }

  mFracWindowA = o2::quality_control_modules::common::getFromConfig<double>(mCustomParameters, "fracWindowLow", mFracWindowA);
  mFracWindowB = o2::quality_control_modules::common::getFromConfig<double>(mCustomParameters, "fracWindowHigh", mFracWindowB);
}

void AgingLaserPostProcTask::initialize(Trigger, framework::ServiceRegistryRef)
{
  ILOG(Debug, Devel) << "initialize AgingLaserPostProcTask" << ENDM;

  ILOG(Debug, Devel) << "agingTaskSourcePath : " << mAgingLaserPath << ENDM;
  ILOG(Debug, Devel) << "agingLaserPostProcPath : " << mAgingLaserPostProcPath << ENDM;
  ILOG(Debug, Devel) << "fractional window : a=" << mFracWindowA << "  b=" << mFracWindowB << ENDM;

  mAmpVsChNormWeightedMeanA = fit::helper::registerHist<TH1F>(
    getObjectsManager(),
    quality_control::core::PublicationPolicy::ThroughStop,
    "", "AmpPerChannelNormWeightedMeanA", "AmpPerChannelNormWeightedMeanA",
    96, 0, 96);

  mAmpVsChNormWeightedMeanC = fit::helper::registerHist<TH1F>(
    getObjectsManager(),
    quality_control::core::PublicationPolicy::ThroughStop,
    "", "AmpPerChannelNormWeightedMeanC", "AmpPerChannelNormWeightedMeanC",
    112, 96, 208);

  if (mReset) {
    mAmpVsChNormWeightedMeanAfterLastCorrA = fit::helper::registerHist<TH1F>(
      getObjectsManager(),
      quality_control::core::PublicationPolicy::ThroughStop,
      "", "AmpPerChannelNormWeightedMeanAfterLastCorrectionA", "AmpPerChannelNormWeightedMeanAfterLastCorrectionA",
      96, 0, 96);

    mAmpVsChNormWeightedMeanAfterLastCorrC = fit::helper::registerHist<TH1F>(
      getObjectsManager(),
      quality_control::core::PublicationPolicy::ThroughStop,
      "", "AmpPerChannelNormWeightedMeanAfterLastCorrectionC", "AmpPerChannelNormWeightedMeanAfterLastCorrectionC",
      112, 96, 208);
  }

  mAmpVsChNormWeightedMeanCorrectedA = fit::helper::registerHist<TH1F>(
    getObjectsManager(),
    quality_control::core::PublicationPolicy::ThroughStop,
    "", "AmpPerChannelNormWeightedMeanCorrectedA", "AmpPerChannelNormWeightedMeanCorrectedA",
    96, 0, 96);
  mAmpVsChNormWeightedMeanCorrectedC = fit::helper::registerHist<TH1F>(
    getObjectsManager(),
    quality_control::core::PublicationPolicy::ThroughStop,
    "", "AmpPerChannelNormWeightedMeanCorrectedC", "AmpPerChannelNormWeightedMeanCorrectedC",
    112, 96, 208);
}

void AgingLaserPostProcTask::update(Trigger t, framework::ServiceRegistryRef srv)
{
  mAmpVsChNormWeightedMeanA->Reset();
  mAmpVsChNormWeightedMeanC->Reset();
  if (mReset) {
    mAmpVsChNormWeightedMeanAfterLastCorrA->Reset();
    mAmpVsChNormWeightedMeanAfterLastCorrC->Reset();
  }
  mAmpVsChNormWeightedMeanCorrectedA->Reset();
  mAmpVsChNormWeightedMeanCorrectedC->Reset();

  /* ---- fetch source histogram ---- */
  auto& qcdb = srv.get<quality_control::repository::DatabaseInterface>();

  auto moAmpPerChannel = qcdb.retrieveMO(mAgingLaserPath, "AmpPerChannel", t.timestamp, t.activity);
  auto moAmpPerChannelPeak1ADC0 = qcdb.retrieveMO(mAgingLaserPath, "AmpPerChannelPeak1ADC0", t.timestamp, t.activity);
  auto moAmpPerChannelPeak1ADC1 = qcdb.retrieveMO(mAgingLaserPath, "AmpPerChannelPeak1ADC1", t.timestamp, t.activity);

  TH2* h2AmpPerChannel = moAmpPerChannel ? dynamic_cast<TH2*>(moAmpPerChannel->getObject()) : nullptr;
  TH2* h2AmpPerChannelPeak1ADC0 = moAmpPerChannelPeak1ADC0 ? dynamic_cast<TH2*>(moAmpPerChannelPeak1ADC0->getObject()) : nullptr;
  TH2* h2AmpPerChannelPeak1ADC1 = moAmpPerChannelPeak1ADC1 ? dynamic_cast<TH2*>(moAmpPerChannelPeak1ADC1->getObject()) : nullptr;
  TH2* hAmpPerChannelPeak1 = nullptr;

  if (h2AmpPerChannelPeak1ADC0 && h2AmpPerChannelPeak1ADC1) {
    hAmpPerChannelPeak1 = new TH2F("hAmpPerChannelPeak1", "hAmpPerChannelPeak1", sNCHANNELS_PM, 0, sNCHANNELS_PM, 4200, -100, 4100);
    hAmpPerChannelPeak1->Add(h2AmpPerChannelPeak1ADC0);
    hAmpPerChannelPeak1->Add(h2AmpPerChannelPeak1ADC1);
  } else {
    ILOG(Fatal) << "Could not retrieve " << mAgingLaserPath << "/AmpPerChannelPeak1ADC0 or " << mAgingLaserPath << "/AmpPerChannelPeak1ADC1 for timestamp "
                << t.timestamp << ENDM;
  }

  if (!h2AmpPerChannel) {
    ILOG(Fatal) << "Could not retrieve " << mAgingLaserPath << "/AmpPerChannel for timestamp "
                << t.timestamp << ENDM;
  }

  if (!hAmpPerChannelPeak1) {
    ILOG(Fatal) << "Could not create merged histogram from " << mAgingLaserPath << "/AmpPerChannelPeak1ADC0 and " << mAgingLaserPath << "/AmpPerChannelPeak1ADC1 for timestamp "
                << t.timestamp << ENDM;
  }

  // Weighted mean of amplitude per detector channel, normalized by the average of the reference channel amplitudes, as stored after performing an aging correction.
  // This is used as a normalization factor to get the relative aging in "non-reset" runs since the time of the last aging correction.
  std::unique_ptr<TH1> hAmpVsChWeightedMeanAfterLastCorrA;
  std::unique_ptr<TH1> hAmpVsChWeightedMeanAfterLastCorrC;

  if (!mReset) {
    auto validity = qcdb.getLatestObjectValidity("qc/" + mAgingLaserPostProcPath + "/AmpPerChannelNormWeightedMeanAfterLastCorrectionA");
    long timestamp = validity.getMin();

    ILOG(Info, Support) << "Retrieving normalization histograms from timestamp " << timestamp << ENDM;

    auto moAmpPerChannelNormWeightedMeanAfterLastCorrA = qcdb.retrieveMO(
      mAgingLaserPostProcPath, "AmpPerChannelNormWeightedMeanAfterLastCorrectionA", timestamp);
    if (!moAmpPerChannelNormWeightedMeanAfterLastCorrA) {
      ILOG(Fatal) << "Failed to retrieve histogram " << mAgingLaserPostProcPath + "/AmpPerChannelNormWeightedMeanAfterLastCorrectionA"
                  << " for timestamp " << timestamp << "! This is not a 'resetting' run and this histogram is therefore needed." << ENDM;
    } else {
      hAmpVsChWeightedMeanAfterLastCorrA = std::unique_ptr<TH1>(
        dynamic_cast<TH1*>(moAmpPerChannelNormWeightedMeanAfterLastCorrA->getObject()->Clone()));
    }

    auto moAmpPerChannelNormWeightedMeanAfterLastCorrC = qcdb.retrieveMO(
      mAgingLaserPostProcPath, "AmpPerChannelNormWeightedMeanAfterLastCorrectionC", timestamp);
    if (!moAmpPerChannelNormWeightedMeanAfterLastCorrC) {
      ILOG(Fatal) << "Failed to retrieve histogram " << mAgingLaserPostProcPath + "/AmpPerChannelNormWeightedMeanAfterLastCorrectionC"
                  << " for timestamp " << timestamp << "! This is not a 'resetting' run and this histogram is therefore needed."
                  << " Please contact the FIT expert." << ENDM;
    } else {
      hAmpVsChWeightedMeanAfterLastCorrC = std::unique_ptr<TH1>(
        dynamic_cast<TH1*>(moAmpPerChannelNormWeightedMeanAfterLastCorrC->getObject()->Clone()));
    }

    if (!hAmpVsChWeightedMeanAfterLastCorrA || !hAmpVsChWeightedMeanAfterLastCorrC) {
      ILOG(Fatal) << "Failed to clone normalization histograms from "
                  << mAgingLaserPostProcPath + "/AmpPerChannelNormWeightedMeanAfterLastCorrectionA or "
                  << mAgingLaserPostProcPath + "/AmpPerChannelNormWeightedMeanAfterLastCorrectionC"
                  << "! This is not a 'resetting' run and these histograms are therefore needed." << ENDM;
    }
  }

  /* ---- 1. Reference-channel Gaussian fits ---- */
  std::vector<double> refMus;

  for (auto chId : mReferenceChIDs) {
    auto h1 = std::unique_ptr<TH1>(hAmpPerChannelPeak1->ProjectionY(
      Form("ref_%d", chId), chId + 1, chId + 1));

    int binMax = h1->GetMaximumBin();
    const double xMax = h1->GetBinCenter(binMax);
    const double winLo = TMath::Max(0., (1. - mFracWindowA) * xMax);
    const double winHi = (1. + mFracWindowB) * xMax;

    TF1 g("g", "gaus", winLo, winHi);
    if (h1->Fit(&g, "QNRS") == 0) { // 0 = fit OK
      refMus.push_back(g.GetParameter(1));
    } else {
      ILOG(Warning) << "Gaussian fit failed for reference channel " << int(chId) << ENDM;
    }
  }

  const unsigned nRef = refMus.size();
  if (nRef == 0) {
    ILOG(Error) << "No successful reference fits – cannot normalise." << ENDM;
    return;
  }
  const double norm = std::accumulate(refMus.begin(), refMus.end(), 0.) / nRef;

  /* ---- 2. Loop over ALL channels ---- */
  auto processChannel = [&](uint8_t chId) {
    auto h1 = std::unique_ptr<TH1>(h2AmpPerChannel->ProjectionY(
      Form("proj_%d", chId), chId + 1, chId + 1));

    // global maximum
    const int binMax = h1->GetMaximumBin();
    const double xMax = h1->GetBinCenter(binMax);
    const double winLo = TMath::Max(0., (1. - mFracWindowA) * xMax);
    const double winHi = (1. + mFracWindowB) * xMax;

    double num = 0., den = 0.;
    const int binLo = h1->FindBin(winLo);
    const int binHi = h1->FindBin(winHi);
    for (int b = binLo; b <= binHi; ++b) {
      const double w = h1->GetBinContent(b);
      const double x = h1->GetBinCenter(b);
      num += w * x;
      den += w;
    }

    const double wMean = (den > 0.) ? num / den : 0.;
    const double val = (norm > 0.) ? wMean / norm : 0.;
    if (chId < 96) {
      mAmpVsChNormWeightedMeanA->SetBinContent(chId + 1, val);
      if (mReset) {
        mAmpVsChNormWeightedMeanAfterLastCorrA->SetBinContent(chId + 1, mAmpVsChNormWeightedMeanA->GetBinContent(chId + 1));
      }
      Float_t valCorr = (!mReset ? hAmpVsChWeightedMeanAfterLastCorrA->GetBinContent(chId + 1) : mAmpVsChNormWeightedMeanAfterLastCorrA->GetBinContent(chId + 1));
      if (valCorr == 0) {
        ILOG(Error, Support) << "Normalization factor = 0 for channel " << chId + 1 << ". Skipping." << ENDM;
        return;
      }
      mAmpVsChNormWeightedMeanCorrectedA->SetBinContent(chId + 1, val / valCorr);
    } else if (chId >= 96 && chId <= 208) {
      mAmpVsChNormWeightedMeanC->SetBinContent(chId - 95, val);
      if (mReset) {
        mAmpVsChNormWeightedMeanAfterLastCorrC->SetBinContent(chId - 95, mAmpVsChNormWeightedMeanC->GetBinContent(chId - 95));
      }
      Float_t valCorr = (!mReset ? hAmpVsChWeightedMeanAfterLastCorrC->GetBinContent(chId - 95) : mAmpVsChNormWeightedMeanAfterLastCorrC->GetBinContent(chId - 95));
      if (valCorr == 0) {
        ILOG(Error, Support) << "Normalization factor = 0 for channel " << chId + 1 << ". Skipping." << ENDM;
        return;
      }
      mAmpVsChNormWeightedMeanCorrectedC->SetBinContent(chId - 95, val / valCorr);
    }
  };

  for (auto ch : mDetectorChIDs) {
    processChannel(ch);
  }

  ILOG(Debug, Devel) << "update done – " << nRef << " reference fits, norm=" << norm << ENDM;
}

void AgingLaserPostProcTask::finalize(Trigger, framework::ServiceRegistryRef)
{
  ILOG(Debug, Devel) << "finalize AgingLaserPostProcTask" << ENDM;
}

} // namespace o2::quality_control_modules::ft0