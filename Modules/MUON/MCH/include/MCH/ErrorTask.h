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
/// \file   ErrorTask.h
/// \author Philippe Pillot
///

#ifndef QC_MODULE_MUONCHAMBERS_ERRORTASK_H
#define QC_MODULE_MUONCHAMBERS_ERRORTASK_H

#include <memory>
#include <string>

#include "QualityControl/TaskInterface.h"

class TProfile;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::muonchambers
{

/// \brief Quality Control Task for the analysis of MCH processing errors
/// \author Philippe Pillot
class ErrorTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  ErrorTask() = default;
  /// Destructor
  ~ErrorTask() override = default;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  auto createProfile(const char* name, const char* title, int nbins, double xmin, double xmax);

  std::unique_ptr<TProfile> mSummary{};
  std::unique_ptr<TProfile> mMultipleDigitsInSamePad{};
  std::unique_ptr<TProfile> mTooManyLocalMaxima{};
};

} // namespace o2::quality_control_modules::muonchambers

#endif // QC_MODULE_MUONCHAMBERS_ERRORTASK_H
