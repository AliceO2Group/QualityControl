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
/// \file    TrendingTracks.h
/// \author  Andrea Ferrero andrea.ferrero@cern.ch
/// \brief   Trending of the MCH tracking
/// \since   21/06/2022
///

#ifndef QC_MODULE_MCH_TRENDING_TRACKS_H
#define QC_MODULE_MCH_TRENDING_TRACKS_H

#include "QualityControl/PostProcessingInterface.h"
#include "QualityControl/Reductor.h"

#include "MCH/TrendingConfigMCH.h"

#include "MCHRawCommon/DataFormats.h"
#include "MCHRawElecMap/Mapper.h"

#include <memory>
#include <unordered_map>
#include <TTree.h>

class TH1F;
class TProfile;

namespace o2::quality_control::repository
{
class DatabaseInterface;
}

using namespace o2::quality_control;
using namespace o2::quality_control::postprocessing;
namespace o2::quality_control_modules::muonchambers
{

/// \brief  A post-processing task which trends MCH hits and stores them in a TTree and produces plots.
class TrendingTracks : public PostProcessingInterface
{
 public:
  TrendingTracks() = default;
  ~TrendingTracks() override = default;

  void configure(const boost::property_tree::ptree& config) override;
  void initialize(Trigger, framework::ServiceRegistryRef) override;
  void update(Trigger, framework::ServiceRegistryRef) override;
  void finalize(Trigger, framework::ServiceRegistryRef) override;

  std::string nameTracksNum = "TracksPerTF";
  std::string nameClusPerTrack = "ClustersPerTrack";
  std::string nameClusPerChamber = "ClustersPerChamber";

 private:
  struct MetaData {
    Int_t runNumber = 0;
  };

  void computeClustersPerChamber(TProfile* p);
  void trendValues(const Trigger& t, repository::DatabaseInterface&);
  void generatePlots();

  TrendingConfigMCH mConfig;
  MetaData mMetaData;
  UInt_t mTime;
  float mClusCH[10]; /// average number of clusters in each chamber

  std::unique_ptr<TTree> mTrend;
  std::map<std::string, TObject*> mPlots;
  std::unordered_map<std::string, std::unique_ptr<Reductor>> mReductors;

  TProfile* mHistClusPerChamberPrev = nullptr;

  // These are initialized from the TrendingConfigMCH.h
};

} // namespace o2::quality_control_modules::muonchambers

#endif // QC_MODULE_MCH_TRENDING_TRACKS_H
