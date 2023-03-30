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
/// \file    TrendingRate.h
/// \author  Nicolò Jacazio nicolo.jacazio@cern.ch
/// \author  Francesco Noferini francesco.noferini@cern.ch
/// \author  Francesca Ercolessi francesca.ercolessi@cern.ch
/// \brief   Trending of the TOF interaction rate
/// \since   06/06/2022
///

#ifndef QC_MODULE_TOF_TRENDING_RATE_H
#define QC_MODULE_TOF_TRENDING_RATE_H

#include "QualityControl/PostProcessingInterface.h"
#include "QualityControl/Reductor.h"

#include "TOF/TrendingConfigTOF.h"

#include <memory>
#include <unordered_map>
#include <TTree.h>

class TH2F;
class TProfile;

namespace o2::quality_control::repository
{
class DatabaseInterface;
}

using namespace o2::quality_control;
using namespace o2::quality_control::postprocessing;
namespace o2::quality_control_modules::tof
{

/// \brief  A post-processing task which trends TOF hits and stores them in a TTree and produces plots.
class TrendingRate : public PostProcessingInterface
{
 public:
  TrendingRate() = default;
  ~TrendingRate() override = default;

  void configure(const boost::property_tree::ptree& config) override;
  void initialize(Trigger, framework::ServiceRegistryRef) override;
  void update(Trigger, framework::ServiceRegistryRef) override;
  void finalize(Trigger, framework::ServiceRegistryRef) override;

  std::string nameHitMap = "HitMap";
  std::string nameMultVsBC = "Multiplicity/VsBC";
  std::string nameMultVsBCpro = "Multiplicity/VsBCpro";

 private:
  struct MetaData {
    Int_t runNumber = 0;
  };

  void trendValues(const Trigger& t, repository::DatabaseInterface&);
  void generatePlots();

  void computeTOFRates(TH2F* h, std::vector<int>& bcInt, std::vector<float>& bcRate, std::vector<float>& bcPileup);

  TrendingConfigTOF mConfig;
  MetaData mMetaData;
  UInt_t mTime;
  // Extra values to trend
  float mNoiseRatePerChannel = 0.f; /// Noise rate
  float mCollisionRate = 0.f;       /// Collision rate
  float mPileupRate = 0.f;          /// Pileup
  int mActiveChannels = 0;          /// Active channels
  int mNIBC = 0;                    /// n interction bunches

  std::unique_ptr<TTree> mTrend;
  std::map<std::string, TObject*> mPlots;
  std::unordered_map<std::string, std::unique_ptr<Reductor>> mReductors;

  TH2F* mPreviousPlot = nullptr; /// to keep memory of previous plot to work only with updates

  static constexpr float orbit_lenght = 88.924596E-6;
  // These are initialized from the TrendingConfigTOF.h
  float mThresholdSgn = 0.f;
  float mThresholdBkg = 0.f;
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_TRENDING_HITS_H
