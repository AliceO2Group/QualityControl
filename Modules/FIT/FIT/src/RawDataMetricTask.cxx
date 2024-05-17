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
/// \file   RawDataMetricTask.cxx
/// \author Artur Furs afurs@cern.ch
/// \brief Task for checking FIT RawDataMetric

#include <vector>
#include <string>

#include "QualityControl/QcInfoLogger.h"
#include "Common/Utils.h"
#include <FITCommon/HelperHist.h>
#include <DataFormatsFIT/RawDataMetric.h>

#include <FIT/RawDataMetricTask.h>

#include <Framework/InputRecord.h>
#include "QualityControl/QcInfoLogger.h"
#include "Framework/ProcessingContext.h"
#include "Framework/InputRecord.h"
#include "Framework/DeviceSpec.h"
#include "Framework/DataSpecUtils.h"

namespace o2::quality_control_modules::fit
{
const std::set<o2::header::DataOrigin> RawDataMetricTask::sSetOfAllowedDets = { o2::header::gDataOriginFDD, o2::header::gDataOriginFT0, o2::header::gDataOriginFV0 };
RawDataMetricTask::~RawDataMetricTask()
{
}

void RawDataMetricTask::initialize(o2::framework::InitContext& ctx)
{
  // Obtaining data origin, i.e. detector ID
  const auto& inputRouts = ctx.services().get<const o2::framework::DeviceSpec>().inputs;
  std::set<header::DataOrigin> setOfOrigins{}; // should be only one
  header::DataOrigin det;
  for (const auto& route : inputRouts) {
    for (const auto& allowedDet : sSetOfAllowedDets) {
      if (o2::framework::DataSpecUtils::partialMatch(route.matcher, allowedDet)) {
        setOfOrigins.insert(allowedDet);
      }
    }
  }

  if (setOfOrigins.size() == 1) {
    det = *setOfOrigins.begin();
    mDetName = std::string{ det.str };
  } else {
    ILOG(Error, Devel) << "Cannot recognize detector!" << ENDM;

    return;
    // assertion
  }
  {
    const auto useModuleNamesForBins = o2::quality_control_modules::common::getFromConfig<bool>(mCustomParameters, "useModuleNamesForBins", false);
    std::map<unsigned int, std::string> mapBinPos2FeeLabel{};
    helperLUT::Fee2ModuleName_t mapFEE2ModuleName{};
    helperLUT::obtainFromLUT(det, mapFEE2ModuleName);
    mMapFEE2binPos.clear();
    unsigned int binPos = 0;
    for (const auto& entry : mapFEE2ModuleName) {
      const auto& entryFEE = entry.first; // (epID,linkID)
      const auto& epID = entryFEE.first;
      const auto& linkID = entryFEE.second;
      const auto& moduleName = entry.second;
      std::string binLabel = useModuleNamesForBins ? moduleName : Form("%i/%i", epID, linkID);
      mapBinPos2FeeLabel.insert({ binPos, binLabel });
      mMapFEE2binPos.insert({ entryFEE, binPos });
      binPos++;
    }
    mBinPosUnknown = mMapFEE2binPos.size();
    mapBinPos2FeeLabel.insert({ mBinPosUnknown, "Unknown" });
    const std::string titleAxisX = useModuleNamesForBins ? "FEE module" : "EPID/LinkID";
    mHistRawDataMetrics = helper::registerHist<TH2F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "RawDataMetric", Form("%s raw data metric statistics;%s;Raw data metric bits", mDetName.c_str(), titleAxisX.c_str()), mapBinPos2FeeLabel, o2::fit::RawDataMetric::sMapBitsToNames);
  }
  //
}

void RawDataMetricTask::startOfActivity(const Activity& activity)
{
  mHistRawDataMetrics->Reset();
}

void RawDataMetricTask::startOfCycle()
{
}

void RawDataMetricTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  const auto& vecRawDataMetric = ctx.inputs().get<gsl::span<o2::fit::RawDataMetric>>("rawDataMetric");
  for (const auto& rawDataMetric : vecRawDataMetric) {
    const auto& itBinPos = mMapFEE2binPos.find(helperLUT::getFEEID(rawDataMetric.mEPID, rawDataMetric.mLinkID));
    if (itBinPos != mMapFEE2binPos.end()) { // registered FEE
      const auto& binPos = itBinPos->second;
      for (int iPosY = 0; iPosY < rawDataMetric.mBitStats.size(); iPosY++) {
        mHistRawDataMetrics->Fill(binPos, iPosY, rawDataMetric.mBitStats[iPosY]);
      }
    } else { // non-registered FEE => unknown, info will be in IL
      for (int iPosY = 0; iPosY < rawDataMetric.mBitStats.size(); iPosY++) {
        mHistRawDataMetrics->Fill(mBinPosUnknown, iPosY, rawDataMetric.mBitStats[iPosY]);
      }
    }
  }
}

void RawDataMetricTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
  // one has to set num. of entries manually because
  // default TH1Reductor gets only mean,stddev and entries (no integral)
}

void RawDataMetricTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void RawDataMetricTask::reset()
{
  // clean all the monitor objects here
  mHistRawDataMetrics->Reset();
}

} // namespace o2::quality_control_modules::fit
