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
/// \file   ExampleTask.h
/// \author Barthelemy von Haller
///

#ifndef QC_MODULE_EXAMPLE_EXAMPLETASK_H
#define QC_MODULE_EXAMPLE_EXAMPLETASK_H

#include "QualityControl/TaskInterface.h"

class TH1F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::example
{

class CustomTH2F;

/// \brief Example Quality Control Task
/// It is final because there is no reason to derive from it. Just remove it if needed.
/// \author Barthelemy von Haller
class ExampleTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  ExampleTask();
  /// Destructor
  ~ExampleTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

  // Accessors
  TH1F*& getHisto1() { return mHistos[0]; }
  TH1F*& getHisto2() { return mHistos[1]; }

 private:
  int mNumberCycles;
  TH1F* mHistos[25];
  void publishHisto(int i);
  CustomTH2F* mCustomTH2F;
};

} // namespace o2::quality_control_modules::example

#endif // QC_MODULE_EXAMPLE_EXAMPLETASK_H
