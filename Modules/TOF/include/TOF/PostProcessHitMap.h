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
/// \file   PostProcessHitMap.h
/// \brief  Post processing to produce a plot of the TOF hit map with the reference enabled channels
/// \author Nicolò Jacazio <nicolo.jacazio@cern.ch>
/// \since  09/07/2022
///

#ifndef QUALITYCONTROL_TOF_POSTPROCESSHITMAP_H
#define QUALITYCONTROL_TOF_POSTPROCESSHITMAP_H

// QC includes
#include "QualityControl/PostProcessingInterface.h"
#include "QualityControl/DatabaseInterface.h"

#include <array>
#include <memory>
#include <string>

class TH1F;
class TH2F;

namespace o2::quality_control_modules::tof
{

class PostProcessHitMap final : public quality_control::postprocessing::PostProcessingInterface
{
 public:
  /// \brief Constructor
  PostProcessHitMap() = default;
  /// \brief Destructor
  ~PostProcessHitMap() override = default;

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
  void finalize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override{};

 private:
  o2::quality_control::repository::DatabaseInterface* mDatabase = nullptr;
  int mRefMapTimestamp;        /// Timestamp of the hitmap to fetch (initialized from the configure method)
  std::string mCCDBPath;       /// CCDB path of the MO (initialized from the configure method)
  std::string mCCDBPathObject; /// CCDB name of the MO (initialized from the configure method)
  std::string mRefMapCcdbPath; /// CCDB path of the RefMap (initialized from the configure method)
  /// Reference hit map taken from the CCDB and translated into QC binning
  std::shared_ptr<TH2F> mHistoRefHitMap = nullptr; /// TOF reference hit map
  std::shared_ptr<TH2F> mHistoHitMap = nullptr;    /// TOF hit map
  bool mDrawRefOnTop;                              /// flag to enable the drawing of the refmap on top of the hit map. if false drawing on top the hitmap (initialized from the configure method)
};

} // namespace o2::quality_control_modules::tof

#endif // QUALITYCONTROL_TOF_POSTPROCESSHITMAP_H
