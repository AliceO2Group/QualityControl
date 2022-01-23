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
/// \file   EveryObject.h
/// \author Piotr Konopka
///

#ifndef QC_MODULE_EXAMPLE_EXAMPLEEVERYOBJECT_H
#define QC_MODULE_EXAMPLE_EXAMPLEEVERYOBJECT_H

#include "QualityControl/TaskInterface.h"
#include <array>

class TH1F;
class TH2F;
class TH3F;
class THnSparse;
class TCanvas;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::example
{

/// \brief Task which publishes (not exactly) every class object used as MO. Can be used to test memory leaks.
/// \author Piotr Konopka
class EveryObject final : public TaskInterface
{
 public:
  /// \brief Constructor
  EveryObject() = default;
  /// Destructor
  ~EveryObject() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  TH1F* mTH1F = nullptr;
  TH2F* mTH2F = nullptr;
  TH3F* mTH3F = nullptr;
  THnSparse* mTHnSparseF = nullptr;
  TCanvas* mTCanvas = nullptr;
  std::array<TH2F*, 4> mTCanvasMembers = { nullptr };
};

} // namespace o2::quality_control_modules::example

#endif // QC_MODULE_EXAMPLE_EXAMPLEEVERYOBJECT_H
