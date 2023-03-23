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
/// \file    TRDTrending.h
/// \author    based on  Piotr Konopka work
///

#ifndef QUALITYCONTROL_TRDTRENDING_H
#define QUALITYCONTROL_TRDTRENDING_H

#include "QualityControl/PostProcessingInterface.h"
#include "QualityControl/Reductor.h"
#include "TRD/TrendingTaskConfigTRD.h"

#include <memory>
#include <unordered_map>
#include <TTree.h>

namespace o2::quality_control::repository
{
class DatabaseInterface;
}

namespace o2::quality_control::postprocessing
{

class TRDTrending : public PostProcessingInterface
{
 public:
  TRDTrending() = default;
  ~TRDTrending() override = default;

  void configure(const boost::property_tree::ptree& config) override;
  void initialize(Trigger, framework::ServiceRegistryRef) override;
  void update(Trigger, framework::ServiceRegistryRef) override;
  void finalize(Trigger, framework::ServiceRegistryRef) override;

 private:
  struct {
    Long64_t runNumber = 0;

  } mMetaData;

  void trendValues(const Trigger& t, repository::DatabaseInterface& qcdb);
  void generatePlots(repository::DatabaseInterface& qcdb);

  TrendingTaskConfigTRD mConfig;
  Int_t ntreeentries = 0;
  UInt_t mTime;
  std::vector<std::string> runlist;
  std::unique_ptr<TTree> mTrend;
  std::map<std::string, TObject*> mPlots;
  std::unordered_map<std::string, std::unique_ptr<Reductor>> mReductors;
};

} // namespace o2::quality_control::postprocessing

#endif // QUALITYCONTROL_TRDTRENDING_H
