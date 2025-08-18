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
  const char* cfgPath = Form("qc.postprocessing.%s", getID().c_str());
  const char* cfgCustom = Form("%s.custom", cfgPath);

  mPostProcHelper.configure(cfg, cfgPath, "FT0");

  auto key = [&cfgCustom](const std::string& e) { return Form("%s.%s", cfgCustom, e.c_str()); };

  mAmpMoPath = helper::getConfigFromPropertyTree<std::string>(cfg, key("agingTaskSourcePath"), mAmpMoPath);

  mDetectorChIDs.resize(208);
  std::iota(mDetectorChIDs.begin(), mDetectorChIDs.end(), 0);
  const std::string detSkip =
    helper::getConfigFromPropertyTree<std::string>(cfg, key("ignoreDetectorChannels"), "");
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
    helper::getConfigFromPropertyTree<std::string>(cfg, key("ignoreRefChannels"), "");
  if (!refSkip.empty()) {
    auto toSkip = fit::helper::parseParameters<uint8_t>(refSkip, ",");
    for (auto s : toSkip) {
      mReferenceChIDs.erase(std::remove(mReferenceChIDs.begin(), mReferenceChIDs.end(), s),
                            mReferenceChIDs.end());
    }
  }

  mADCSearchMin = helper::getConfigFromPropertyTree<double>(cfg, key("refPeakWindowMin"), mADCSearchMin);
  mADCSearchMax = helper::getConfigFromPropertyTree<double>(cfg, key("refPeakWindowMax"), mADCSearchMax);
  mFracWindowA = helper::getConfigFromPropertyTree<double>(cfg, key("fracWindowLow"), mFracWindowA);
  mFracWindowB = helper::getConfigFromPropertyTree<double>(cfg, key("fracWindowHigh"), mFracWindowB);
}

void AgingLaserPostProcTask::initialize(Trigger, framework::ServiceRegistryRef)
{
  ILOG(Debug, Devel) << "initialize AgingLaserPostProcTask" << ENDM;

  ILOG(Debug, Devel) << "agingTaskSourcePath : " << mAmpMoPath << ENDM;
  ILOG(Debug, Devel) << "ADC search window : [" << mADCSearchMin << ", " << mADCSearchMax << "]" << ENDM;
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
  TH2* h2amp = moIn ? dynamic_cast<TH2*>(moIn->getObject()) : nullptr;

  if (!h2amp) {
    ILOG(Error) << "Could not retrieve " << mAmpMoPath << "/AmpPerChannel for timestamp "
                << t.timestamp << ENDM;
    return;
  }

  /* ---- 1. Reference-channel Gaussian fits ---- */
  std::vector<double> refMus;

  for (auto chId : mReferenceChIDs) {
    auto h1 = std::unique_ptr<TH1>(h2amp->ProjectionY(
      Form("ref_%d", chId), chId + 1, chId + 1));

    const int binLow = h1->FindBin(mADCSearchMin);
    const int binHigh = h1->FindBin(mADCSearchMax);

    int binMax = binLow;
    for (int b = binLow + 1; b <= binHigh; ++b) {
      if (h1->GetBinContent(b) > h1->GetBinContent(binMax)) {
        binMax = b;
      }
    }
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