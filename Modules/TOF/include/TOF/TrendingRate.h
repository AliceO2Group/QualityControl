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
/// \author  Francesca Ercolessi francesca.ercolessi@cern.ch
/// \brief  
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

  void configure(std::string name, const boost::property_tree::ptree& config) override;
  void initialize(Trigger, framework::ServiceRegistry&) override;
  void update(Trigger, framework::ServiceRegistry&) override;
  void finalize(Trigger, framework::ServiceRegistry&) override;

  std::string nameHitMap = "HitMap";
  std::string nameMultVsBC = "Multiplicity/VsBC";
  std::string nameMultVsBCpro = "Multiplicity/VsBCpro";

 private:
  struct MetaData {
    Int_t runNumber = 0;
  };

  void trendValues(const Trigger& t, repository::DatabaseInterface&);
  void generatePlots();

  TrendingConfigTOF mConfig;
  MetaData mMetaData;
  UInt_t mTime;
  std::unique_ptr<TTree> mTrend;
  std::map<std::string, TObject*> mPlots;
  std::unordered_map<std::string, std::unique_ptr<Reductor>> mReductors;

  static constexpr float orbit_lenght = 90E-6;
  float mThresholdSgn = 0.15;
  float mThresholdBkg = 0.04;

};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_TRENDING_HITS_H
