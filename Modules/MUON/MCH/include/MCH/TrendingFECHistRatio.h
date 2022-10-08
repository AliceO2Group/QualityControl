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
/// \file    TrendingFECHistRatio.h
/// \author  Andrea Ferrero andrea.ferrero@cern.ch
/// \brief   Trending of the MCH FEC histogram ratios (rates, efficiencies, etc...)
/// \since   06/06/2022
///

#ifndef QC_MODULE_MCH_TRENDING_FEC_HIST_RATIO_H
#define QC_MODULE_MCH_TRENDING_FEC_HIST_RATIO_H

#include "QualityControl/PostProcessingInterface.h"
#include "QualityControl/Reductor.h"

#include "MCH/TrendingConfigMCH.h"

#include "MCHRawCommon/DataFormats.h"
#include "MCHRawElecMap/Mapper.h"

#include <memory>
#include <unordered_map>
#include <TTree.h>

class TH2F;

namespace o2::quality_control::repository
{
class DatabaseInterface;
}

using namespace o2::quality_control;
using namespace o2::quality_control::postprocessing;
namespace o2::quality_control_modules::muonchambers
{

/// \brief  A post-processing task which trends MCH hits and stores them in a TTree and produces plots.
class TrendingFECHistRatio : public PostProcessingInterface
{
 public:
  TrendingFECHistRatio() = default;
  ~TrendingFECHistRatio() override = default;

  void configure(std::string name, const boost::property_tree::ptree& config) override;
  void initialize(Trigger, framework::ServiceRegistryRef) override;
  void update(Trigger, framework::ServiceRegistryRef) override;
  void finalize(Trigger, framework::ServiceRegistryRef) override;

 private:
  struct MetaData {
    Int_t runNumber = 0;
  };

  int checkPadMapping(uint16_t feeId, uint8_t linkId, uint8_t eLinkId, o2::mch::raw::DualSampaChannelId channel);
  void trendValues(const Trigger& t, repository::DatabaseInterface&);
  void generatePlots();

  void computeMCHFECHistRatios(TH2F* hNum, TH2F* hDen);

  o2::mch::raw::Elec2DetMapper mElec2DetMapper;
  o2::mch::raw::Det2ElecMapper mDet2ElecMapper;
  o2::mch::raw::FeeLink2SolarMapper mFeeLink2SolarMapper;
  o2::mch::raw::Solar2FeeLinkMapper mSolar2FeeLinkMapper;

  TrendingConfigMCH mConfig;
  MetaData mMetaData;
  UInt_t mTime;
  float mTrendCH[10];   /// average rate in each chamber
  float mTrendDE[1100]; /// average rate in detection element

  std::unique_ptr<TTree> mTrend;
  std::map<std::string, TObject*> mPlots;
  std::unordered_map<std::string, std::unique_ptr<Reductor>> mReductors;

  TH2F* mPreviousNum = nullptr; /// to keep memory of previous plot to work only with updates
  TH2F* mPreviousDen = nullptr; /// to keep memory of previous plot to work only with updates

  // These are initialized from the TrendingConfigMCH.h
};

} // namespace o2::quality_control_modules::muonchambers

#endif // QC_MODULE_MCH_TRENDING_FEC_HIST_RATIO_H
