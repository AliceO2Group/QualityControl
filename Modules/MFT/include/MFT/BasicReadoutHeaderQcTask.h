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
/// \file   BasicReadoutHeaderQcTask.h
/// \author Tomas Herman
/// \author Guillermo Contreras
///

#ifndef QC_MODULE_MFT_MFTBasicReadoutHeaderQcTask_H
#define QC_MODULE_MFT_MFTBasicReadoutHeaderQcTask_H

// Quality Control
#include "QualityControl/TaskInterface.h"

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::mft
{

/// \brief MFT Basic Readout Header QC task
///
/// \author Tomas Herman
/// \author Guillermo Contreras
class BasicReadoutHeaderQcTask /*final*/ : public TaskInterface // todo add back the "final" when doxygen is fixed
{
 public:
  /// \brief Constructor
  BasicReadoutHeaderQcTask() = default;
  /// Destructor
  ~BasicReadoutHeaderQcTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  std::unique_ptr<TH1F> mMFT_LaneStatus_H = nullptr;
};

} // namespace o2::quality_control_modules::mft

#endif // QC_MODULE_MFT_MFTBasicReadoutHeaderQcTask_H
