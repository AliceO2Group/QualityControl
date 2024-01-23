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
/// \file     CTPTrendingTask.h
/// \author   Lucia Anna Tarasovicova
///

#ifndef QUALITYCONTROL_CTPTTRENDINGTASK_H
#define QUALITYCONTROL_CTPTTRENDINGTASK_H

#include "QualityControl/PostProcessingInterface.h"
#include "CTP/TrendingConfigCTP.h"
#include "QualityControl/Reductor.h"
#include <DataFormatsCTP/Configuration.h>

#include <string>
#include <vector>
#include <TTree.h>

namespace o2::quality_control::repository
{
class DatabaseInterface;
} // namespace o2::quality_control::repository

namespace o2::quality_control::postprocessing
{
/// \brief  A post-processing task for trending ctp input and class rates
///

class CTPTrendingTask : public PostProcessingInterface
{
 public:
  /// \brief Constructor.
  CTPTrendingTask() = default;
  /// \brief Destructor.
  ~CTPTrendingTask() override = default;

  /// \brief Post-processing methods inherited from 'PostProcessingInterface'.
  void configure(const boost::property_tree::ptree& config) override;
  void initialize(Trigger, framework::ServiceRegistryRef) override;
  void update(Trigger, framework::ServiceRegistryRef) override;
  void finalize(Trigger, framework::ServiceRegistryRef) override;

 private:
  struct MetaData {
    Int_t runNumber = 0;
  };

  void trendValues(const Trigger& t, quality_control::repository::DatabaseInterface& qcdb);
  void generatePlots();

  o2::ctp::CTPConfiguration mCTPconfig;
  TrendingConfigCTP mConfig;
  MetaData mMetaData;
  UInt_t mTime;

  std::map<std::string, TObject*> mPlots;

  std::unique_ptr<TTree> mTrend;
  std::unordered_map<std::string, std::unique_ptr<Reductor>> mReductors;
};

} // namespace o2::quality_control::postprocessing

#endif // QUALITYCONTROL_CTPTTRENDINGTASK_H
