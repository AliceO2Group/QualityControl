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
/// \file    TrendingTask.h
/// \author  Piotr Konopka
///

#ifndef QUALITYCONTROL_TRENDINGTASK_H
#define QUALITYCONTROL_TRENDINGTASK_H

#include "QualityControl/PostProcessingInterface.h"
#include "QualityControl/Reductor.h"
#include "QualityControl/TrendingTaskConfig.h"

#include <memory>
#include <unordered_map>
#include <TTree.h>

class TAxis;

namespace o2::quality_control::repository
{
class DatabaseInterface;
}

namespace o2::quality_control::postprocessing
{

/// \brief  A post-processing task which trends values, stores them in a TTree and produces plots.
///
/// A post-processing task which trends objects inside QC database (QCDB). It extracts some values of one or multiple
/// objects using the Reductor classes, then stores them inside a TTree. One can generate plots out the TTree - the
/// class exposes the TTree::Draw interface to the user. The TTree and plots are stored in the QCDB. The class is
/// configured with configuration files, see Framework/postprocessing.json as an example.
///
/// \author Piotr Konopka
class TrendingTask : public PostProcessingInterface
{
 public:
  TrendingTask() = default;
  ~TrendingTask() override = default;

  void configure(const boost::property_tree::ptree& config) override;
  void initialize(Trigger, framework::ServiceRegistryRef) override;
  void update(Trigger, framework::ServiceRegistryRef) override;
  void finalize(Trigger, framework::ServiceRegistryRef) override;

  static void setUserAxisLabel(TAxis* xAxis, TAxis* yAxis, const std::string& graphAxisLabel);
  static void setUserYAxisRange(TH1* hist, const std::string& graphYAxisRange);

 private:
  struct {
    Long64_t runNumber = 0;
    static const char* getBranchLeafList()
    {
      return "runNumber/L";
    }
  } mMetaData;

  void trendValues(const Trigger& t, repository::DatabaseInterface&);
  void generatePlots();
  bool canContinueTrend(TTree* tree);

  TrendingTaskConfig mConfig;
  UInt_t mTime;
  std::unique_ptr<TTree> mTrend;
  std::map<std::string, TObject*> mPlots;
  std::unordered_map<std::string, std::unique_ptr<Reductor>> mReductors;
};

} // namespace o2::quality_control::postprocessing

#endif // QUALITYCONTROL_TRENDINGTASK_H
