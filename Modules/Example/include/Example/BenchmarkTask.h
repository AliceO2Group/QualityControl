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
/// \file   BenchmarkTask.h
/// \author Barthelemy von Haller
///

#ifndef QC_MODULE_EXAMPLE_BENCHMARKTASK_H
#define QC_MODULE_EXAMPLE_BENCHMARKTASK_H

#include "QualityControl/TaskInterface.h"

#include <vector>
#include <memory>
#include <string>

namespace o2::configuration
{
class ConfigurationInterface;
}

class TH1F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::example
{

/// \brief Quality Control Task for benchmarking
/// It publishes a number of TH1F (configurable, see example.ini in module QualityControl).
/// The histos are reset and refilled (1000 random) at EOC. They have 1000 bins.
/// The monitoring of data blocks is empty (sleep 100 ms).
/// \author Barthelemy von Haller
class BenchmarkTask : public TaskInterface
{
 public:
  /// \brief Constructor
  BenchmarkTask();
  /// Destructor
  ~BenchmarkTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  std::vector<TH1F*> mHistos;
  std::unique_ptr<o2::configuration::ConfigurationInterface> mConfigFile;
  size_t mNumberHistos;
  size_t mNumberChecks;
  std::string mTypeOfChecks;
  std::string mModuleOfChecks;

  //    ClassDef(BenchmarkTask,1);
};

} // namespace o2::quality_control_modules::example

#endif // QC_MODULE_EXAMPLE_BENCHMARKTASK_H
