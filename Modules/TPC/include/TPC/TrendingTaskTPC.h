// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file    TrendingTaskTPC.h
/// \author  Marcel Lesch
/// \author  Cindy Mordasini
/// \author  Based on the work from Piotr Konopka
///

#ifndef QUALITYCONTROL_TRENDINGTASKTPC_H
#define QUALITYCONTROL_TRENDINGTASKTPC_H

#include "QualityControl/PostProcessingInterface.h"
#include "TPC/TrendingTaskConfigTPC.h"
#include "TPC/ReductorTPC.h"

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

/// \brief  A post-processing task which trends values, stores them in a TTree and produces plots.
///
/// A post-processing task which trends objects inside QC database (QCDB). It extracts some values of one or multiple objects using the Reductor classes, then stores them inside a TTree.
/// One can generate plots out the TTree - the class exposes the TTree::Draw interface to the user. The TTree and plots are stored in the QCDB. The class is configured with configuration files, see Framework/postprocessing.json as an example.
///
/// \author based on work of Piotr Konopka
class TrendingTaskTPC : public quality_control::postprocessing::PostProcessingInterface
{
 public:
  /// \brief Constructor.
  TrendingTaskTPC() = default;
  /// \brief Destructor.
  ~TrendingTaskTPC() override = default;

  /// \brief Definitions of the methods used for the trending.
  void configure(std::string name, const boost::property_tree::ptree& config) override;
  void initialize(quality_control::postprocessing::Trigger, framework::ServiceRegistry&) override;
  void update(quality_control::postprocessing::Trigger, framework::ServiceRegistry&) override;
  void finalize(quality_control::postprocessing::Trigger, framework::ServiceRegistry&) override;

 private:
  struct MetaData {
    Int_t runNumber = 0;
  };

  // Method to access the histograms to trend and publish their outputs.
  void trendValues(uint64_t timestamp, o2::quality_control::repository::DatabaseInterface&);
  void generatePlots();
  void drawCanvas(TCanvas* thisCanvas, std::string var, std::string sel, std::string opt, std::string err, std::string name);

  TrendingTaskConfigTPC mConfig;                                            ///< JSON configuration of source and plots.
  MetaData mMetaData;                                                       ///<
  UInt_t mTime;                                                             ///<
  std::unique_ptr<TTree> mTrend;                                            ///< Trending values at a given time.
  std::map<std::string, TObject*> mPlots;                                   ///<
  std::unordered_map<std::string, std::unique_ptr<ReductorTPC>> mReductors; ///< Reductors for all the sources in the post-processing json.
  int mNumberPads;                                                          ///< Number of pads in the output canvas.
};

} // namespace o2::quality_control_modules::tpc

#endif //QUALITYCONTROL_TRENDINGTASKTPC_H
