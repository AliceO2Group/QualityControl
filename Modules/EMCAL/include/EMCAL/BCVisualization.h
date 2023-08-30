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
/// \file   BCVisualization.h
/// \author Markus Fasel
///

#ifndef QUALITYCONTROL_BCVISUALIZATION_H
#define QUALITYCONTROL_BCVISUALIZATION_H

// QC includes
#include "QualityControl/PostProcessingInterface.h"
#include <map>
#include <memory>
#include <vector>
#include <string>
#include <tuple>
#include <cfloat>

class TCanvas;
class TH1;

namespace o2::quality_control_modules::emcal
{

/// \brief Visualization of BC distributions and determination of Shift between EMCAL and CTP
class BCVisualization final : public quality_control::postprocessing::PostProcessingInterface
{
 public:
  enum class MethodBCShift_t {
    LEADING_BC,
    ISOLATED_BC,
    UNKNOWN
  };
  /// \brief Constructor
  BCVisualization() = default;
  /// \brief Destructor
  ~BCVisualization() = default;

  /// \brief Configuration of a post-processing task.
  /// Configuration of a post-processing task. Can be overridden if user wants to retrieve the configuration of the task.
  /// \param config   ConfigurationInterface with prefix set to ""
  void configure(const boost::property_tree::ptree& config) override;
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
  void update(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef services) override;
  /// \brief Finalization of a post-processing task.
  /// Finalization of a post-processing task. User receives a Trigger which caused the finalization and a service
  /// registry with singleton interfaces.
  /// \param trigger  Trigger which caused the initialization, for example Trigger::EOR
  /// \param services Interface containing optional interfaces, for example DatabaseInterface
  void finalize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;

  void reset();

 private:
  std::tuple<bool, int> determineBCShift(const TH1* emchist, const TH1* ctphist, MethodBCShift_t method) const;
  std::tuple<bool, int> determineBCShiftIsolated(const TH1* emchist, const TH1* ctphist) const;
  std::tuple<bool, int> determineBCShiftLeading(const TH1* emchist, const TH1* ctphist) const;
  std::vector<int> getIsolatedBCs(const TH1* bchist) const;
  std::vector<int> getShifted(const std::vector<int>& bcEMC, int shift) const;
  int getNMatching(const std::vector<int>& bcShiftedEMC, const std::vector<int>& bcCTP) const;
  int getLeadingBC(const TH1* histogram) const;
  MethodBCShift_t getMethod(const std::string_view methodstring) const;
  std::string getMethodString(MethodBCShift_t method) const;
  TCanvas* mOutputCanvas = nullptr;                                ///< output plot
  TH1* mFrame = nullptr;                                           ///< Frame for output canvas
  std::string mDataPath;                                           ///< Path in QCDB with histos
  MethodBCShift_t mShiftEvaluation = MethodBCShift_t::ISOLATED_BC; ///< Method used to determine the BC shift
  int mMinNumberOfEntriesBCShift = 100;                            ///< Min. number of entries for BC shift calculation
};

} // namespace o2::quality_control_modules::emcal

#endif // QUALITYCONTROL_BCVISUALIZATION_H
