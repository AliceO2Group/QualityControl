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
/// \file   MeanVertexPostProcessing.h
/// \author My Name
///

#ifndef QUALITYCONTROL_MEANVERTEXPOSTPROCESSING_H
#define QUALITYCONTROL_MEANVERTEXPOSTPROCESSING_H

#include "QualityControl/PostProcessingInterface.h"
#include "CCDB/CcdbApi.h"

#include <TLine.h>
#include <TH1F.h>
#include <TGraph.h>
#include <TCanvas.h>

namespace o2::quality_control_modules::glo
{

class TrendGraph : public TCanvas
{
 public:
  TrendGraph(std::string name, std::string title, std::string label, float rangeMin, float rangeMax);

  ~TrendGraph() override {}

  void update(uint64_t time, float val);

 private:
  float mRangeMin;
  float mRangeMax;
  std::string mAxisLabel;
  std::unique_ptr<TGraph> mGraph;
  std::unique_ptr<TGraph> mGraphHist;
  std::array<std::unique_ptr<TLine>, 2> mLines;
};

/// \brief Postprocessing Task for Mean Vertex calibration

class MeanVertexPostProcessing final : public quality_control::postprocessing::PostProcessingInterface
{
 public:
  /// \brief Constructor
  MeanVertexPostProcessing() = default;
  /// \brief Destructor
  ~MeanVertexPostProcessing() override;

  /// \brief Configuration of a post-processing task.
  /// Configuration of a post-processing task. Can be overridden if user wants to retrieve the configuration of the task.
  /// \param config   ConfigurationInterface with prefix set to ""
  virtual void configure(const boost::property_tree::ptree& config) override;

  /// \brief Initialization of a post-processing task.
  /// Initialization of a post-processing task. User receives a Trigger which caused the initialization and a service
  /// registry with singleton interfaces.
  /// \param trigger  Trigger which caused the initialization, for example Trigger::SOR
  /// \param services Interface containing optional interfaces, for example DatabaseInterface
  void initialize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;

  /// \brief Update of a post-processing task.
  /// Update of a post-processing task. User receives a Trigger which caused the update and a service
  /// registry with singleton interfaces.
  /// \param trigger  Trigger which caused the initialization, for example Trigger::Period
  /// \param services Interface containing optional interfaces, for example DatabaseInterface
  void update(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;

  /// \brief Finalization of a post-processing task.
  /// Finalization of a post-processing task. User receives a Trigger which caused the finalization and a service
  /// registry with singleton interfaces.
  /// \param trigger  Trigger which caused the initialization, for example Trigger::EOR
  /// \param services Interface containing optional interfaces, for example DatabaseInterface
  void finalize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;

 private:
  float mRangeX{ 0.1 };
  float mRangeY{ 0.1 };
  float mRangeZ{ 1.0 };
  float mRangeSigmaX{ 1.0 };
  float mRangeSigmaY{ 1.0 };
  float mRangeSigmaZ{ 10.0 };
  bool mResetHistos{ false };
  std::unique_ptr<TrendGraph> mGraphX;
  std::unique_ptr<TrendGraph> mGraphY;
  std::unique_ptr<TrendGraph> mGraphZ;
  std::unique_ptr<TrendGraph> mGraphSigmaX;
  std::unique_ptr<TrendGraph> mGraphSigmaY;
  std::unique_ptr<TrendGraph> mGraphSigmaZ;
  TH1F* mX = nullptr;
  TH1F* mY = nullptr;
  TH1F* mZ = nullptr;
  TH1F* mStartValidity = nullptr;
  o2::ccdb::CcdbApi mCcdbApi;
  std::string mCcdbUrl = "https://alice-ccdb.cern.ch";
};

} // namespace o2::quality_control_modules::glo

#endif // QUALITYCONTROL_MEANVERTEXPOSTPROCESSING_H
