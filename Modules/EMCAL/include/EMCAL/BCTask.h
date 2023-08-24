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
/// \file   BCTask.h
/// \author Markus Fasel
///

#ifndef QC_MODULE_EMCAL_EMCALBCTASK_H
#define QC_MODULE_EMCAL_EMCALBCTASK_H

#include "QualityControl/TaskInterface.h"

class TH1F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::emcal
{

/// \brief Task monitoring the BC distribution of EMCAL objects and triggers in CTP
/// \author Markus Fasel
///
/// Monitoring the BCs from EMCAL readout and the various EMCAL triggers from CTP
/// readout.
///
/// Attention: Task requires to have CTP readout in the data.
class BCTask final : public TaskInterface
{
 public:
  /// \enum TriggerClassIndex
  /// \brief Index of the a given trigger class mask in the class mask array
  enum TriggerClassIndex {
    EMCMinBias,     ///< EMCAL min bias trigger
    EMCL0,          ///< EMCAL Level-0 trigger
    DMCL0,          ///< DCAL Level-0 trigger
    NTriggerClasses ///< All triggers
  };
  /// \brief Constructor
  BCTask() = default;
  /// Destructor
  ~BCTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 protected:
  /// \brief Load trigger configuration for current run and timestamp
  /// \param timestamp Timestamp from ProcessingContext
  /// \return True if classes were successfully loaded, false otherwise
  bool loadTriggerClasses(uint64_t timestamp);

  /// \brief Check if the class masks are initialized
  /// \return True if the class masks are initialized, false otherwise
  ///
  /// We expect at least one class mask to be non-0 (usually the min. bias
  /// trigger which is guaranteed in all runs with EMCAL readout)
  bool hasClassMasksLoaded() const;

 private:
  TH1F* mBCReadout = nullptr; ///< BC distribution from EMCAL readout
  TH1F* mBCEMCAny = nullptr;  ///< BC distribution from CTP, any trigger
  TH1F* mBCMinBias = nullptr; ///< BC distribution from CTP, EMCAL min. bias trigger
  TH1F* mBCL0EMCAL = nullptr; ///< BC distribution from CTP, EMCAL L0 trigger
  TH1F* mBCL0DCAL = nullptr;  ///< BC distribution from CTP, DCAL trigger

  uint32_t mCurrentRun = -1;                                  ///< Current run
  std::array<uint64_t, NTriggerClasses> mTriggerClassIndices; ///< Trigger class mask from for different EMCAL triggers from CTP configuration
};

} // namespace o2::quality_control_modules::emcal

#endif // QC_MODULE_EMCAL_EMCALBCTASK_H
