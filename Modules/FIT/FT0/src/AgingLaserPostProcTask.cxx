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
#include "FITCommon/HelperHist.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/QcInfoLogger.h"

#include <TH2.h>
#include <TF1.h>
#include <TMath.h>

#include <algorithm>
#include <numeric>
#include <string>

using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control_modules::fit;

namespace o2::quality_control_modules::ft0
{

AgingLaserPostProcTask::~AgingLaserPostProcTask() = default;

void AgingLaserPostProcTask::configure(const boost::property_tree::ptree& cfg)
{
  mAmpMoPath = o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "agingTaskSourcePath", mAmpMoPath);

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

  ILOG(Debug, Devel) << "agingTaskSourcePath : " << mAmpMoPath << ENDM;
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
}

void AgingLaserPostProcTask::update(Trigger t, framework::ServiceRegistryRef srv)
{
  mAmpVsChNormWeightedMeanA->Reset();
  mAmpVsChNormWeightedMeanC->Reset();

  /* ---- fetch source histogram ---- */
  auto& qcdb = srv.get<quality_control::repository::DatabaseInterface>();
  auto moIn = qcdb.retrieveMO(mAmpMoPath, "AmpPerChannel", t.timestamp, t.activity);
  auto moIn0 = qcdb.retrieveMO(mAmpMoPath, "AmpPerChannelPeak1ADC0", t.timestamp, t.activity);
  auto moIn1 = qcdb.retrieveMO(mAmpMoPath, "AmpPerChannelPeak1ADC1", t.timestamp, t.activity);
  TH2* h2amp = moIn ? dynamic_cast<TH2*>(moIn->getObject()) : nullptr;
  TH2* h2amp0 = moIn0 ? dynamic_cast<TH2*>(moIn0->getObject()) : nullptr;
  TH2* h2amp1 = moIn1 ? dynamic_cast<TH2*>(moIn1->getObject()) : nullptr;
  TH2* h2ampMerged = nullptr;
  if (h2amp0 && h2amp1) {
    h2ampMerged = new TH2F("h2ampMerged", "h2ampMerged", sNCHANNELS_PM, 0, sNCHANNELS_PM, 4200, -100, 4100);
    h2ampMerged->Add(h2amp0);
    h2ampMerged->Add(h2amp1);
  }

  if (!h2amp) {
    ILOG(Error) << "Could not retrieve " << mAmpMoPath << "/AmpPerChannel for timestamp "
                << t.timestamp << ENDM;
    return;
  }

  if (!h2amp0 || !h2amp1) {
    ILOG(Error) << "Could not retrieve " << mAmpMoPath << "/AmpPerChannelPeak1ADC0 or " << mAmpMoPath << "/AmpPerChannelPeak1ADC1 for timestamp "
                << t.timestamp << ENDM;
    return;
  }

  if (!h2ampMerged) {
    ILOG(Error) << "Could not create merged histogram from " << mAmpMoPath << "/AmpPerChannelPeak1ADC0 and " << mAmpMoPath << "/AmpPerChannelPeak1ADC1 for timestamp "
                << t.timestamp << ENDM;
    return;
  }

  /* ---- 1. Reference-channel Gaussian fits ---- */
  std::vector<double> refMus;

  for (auto chId : mReferenceChIDs) {
    auto h1 = std::unique_ptr<TH1>(h2ampMerged->ProjectionY(
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
    auto h1 = std::unique_ptr<TH1>(h2amp->ProjectionY(
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
    } else if (chId >= 96 && chId <= 208) {
      mAmpVsChNormWeightedMeanC->SetBinContent(chId - 95, val);
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