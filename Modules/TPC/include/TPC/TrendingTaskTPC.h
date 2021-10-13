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
/// \file   TrendingTaskTPC.h
/// \author Marcel Lesch
/// \author Cindy Mordasini
/// \author Based on the work from Piotr Konopka
///

#ifndef QUALITYCONTROL_TRENDINGTASKTPC_H
#define QUALITYCONTROL_TRENDINGTASKTPC_H

#include "TPC/SliceInfo.h"
#include "TPC/TrendingTaskConfigTPC.h"
#include "TPC/ReductorTPC.h"
#include "QualityControl/PostProcessingInterface.h"

#include <memory>
#include <unordered_map>
#include <TTree.h>
#include <TCanvas.h>

namespace o2::quality_control::repository
{
class DatabaseInterface;
} // namespace o2::quality_control::repository

namespace o2::quality_control_modules::tpc
{
/// \brief  A post-processing task tuned for the needs of the trending of the TPC.
///
/// A post-processing task which trends TPC related objects inside QC database (QCDB).
/// It extracts some values of one or multiple objects using the Reductor classes,
/// then stores them inside a TTree. The class exposes the TTree::Draw interface
/// to the user tp generate the plots out of the TTree.
/// This class is specific to the TPC: a subrange slicer is available in the json,
/// and input/output canvas can be dealt with alongside normal histograms.
///
/// \author Marcel Lesch
/// \author Cindy Mordasini
/// \author Based on the work from Piotr Konopka

class TrendingTaskTPC : public o2::quality_control::postprocessing::PostProcessingInterface
{
 public:
  /// \brief Constructor.
  TrendingTaskTPC() = default;
  /// \brief Destructor.
  ~TrendingTaskTPC() final = default;

  /// \brief Definitions of the methods used for the trending.
  void configure(std::string name, const boost::property_tree::ptree& config) final;
  void initialize(o2::quality_control::postprocessing::Trigger, framework::ServiceRegistry&) final;
  void update(o2::quality_control::postprocessing::Trigger, framework::ServiceRegistry&) final;
  void finalize(o2::quality_control::postprocessing::Trigger, framework::ServiceRegistry&) final;

 private:
  struct MetaData {
    Int_t runNumber = 0;
  };

  void trendValues(uint64_t timestamp, o2::quality_control::repository::DatabaseInterface&);
  void generatePlots();
  void drawCanvas(TCanvas* thisCanvas, const std::string& var, const std::string& sel,
    const std::string& opt, const std::string& err, const std::string& name);

  TrendingTaskConfigTPC mConfig;
  MetaData mMetaData;
  UInt_t mTime;
  std::unique_ptr<TTree> mTrend;
  std::map<std::string, TObject*> mPlots;
  std::unordered_map<std::string, std::unique_ptr<ReductorTPC>> mReductors;
  int mNumberPads;    // LOKI: Needed?
  std::unordered_map<std::string, std::vector<SliceInfo>> mSources;
};

} // namespace o2::quality_control_modules::tpc

#endif //QUALITYCONTROL_TRENDINGTASKTPC_H
