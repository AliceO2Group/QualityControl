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
/// \file    TrendingCalibLHCphase.h
/// \author  Francesca Ercolessi

#ifndef QC_MODULE_TOF_TRENDING_CALIB_LHCPHASE_H
#define QC_MODULE_TOF_TRENDING_CALIB_LHCPHASE_H

#include "QualityControl/PostProcessingInterface.h"
#include "QualityControl/Reductor.h"
#include "CCDB/CcdbApi.h"

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

class TrendingCalibLHCphase : public PostProcessingInterface
{
 public:
  TrendingCalibLHCphase() = default;
  ~TrendingCalibLHCphase() override = default;

  void configure(const boost::property_tree::ptree& config) override;
  void initialize(Trigger, framework::ServiceRegistryRef) override;
  void update(Trigger, framework::ServiceRegistryRef) override;
  void finalize(Trigger, framework::ServiceRegistryRef) override;

 private:
  struct MetaData {
    Int_t runNumber = 0;
  };

  void trendValues(const Trigger& t, repository::DatabaseInterface&);
  void generatePlots();

  TrendingConfigTOF mConfig;
  MetaData mMetaData;
  UInt_t mTime;
  float mPhase = 0.;
  o2::ccdb::CcdbApi mCdbApi;
  std::string mHost;

  std::unique_ptr<TTree> mTrend;
  std::map<std::string, TObject*> mPlots;
  std::unordered_map<std::string, std::unique_ptr<Reductor>> mReductors;
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_TRENDING_CALIB_LHCPHASE_H
