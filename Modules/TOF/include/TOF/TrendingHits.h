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
/// \file    TrendingHits.h
/// \author  Nicolò Jacazio nicolo.jacazio@cern.ch
/// \author  Francesca Ercolessi francesca.ercolessi@cern.ch
/// \brief   Header file for the trending task for the number of hits in TOF
/// \since   05/10/2021
///

#ifndef QC_MODULE_TOF_TRENDING_HITS_H
#define QC_MODULE_TOF_TRENDING_HITS_H

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
class TrendingHits : public PostProcessingInterface
{
 public:
  TrendingHits() = default;
  ~TrendingHits() override = default;

  void configure(std::string name, const boost::property_tree::ptree& config) override;
  void initialize(Trigger, framework::ServiceRegistry&) override;
  void update(Trigger, framework::ServiceRegistry&) override;
  void finalize(Trigger, framework::ServiceRegistry&) override;

 private:
  struct MetaData {
    Int_t runNumber = 0;
  };

  void trendValues(uint64_t timestamp, repository::DatabaseInterface&);
  void generatePlots();

  TrendingConfigTOF mConfig;
  MetaData mMetaData;
  UInt_t mTime;
  std::unique_ptr<TTree> mTrend;
  std::map<std::string, TObject*> mPlots;
  std::unordered_map<std::string, std::unique_ptr<Reductor>> mReductors;
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_TRENDING_HITS_H
