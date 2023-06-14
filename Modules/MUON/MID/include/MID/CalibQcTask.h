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
/// \file   CalibQcTask.h
/// \author Valerie Ramillien

#ifndef QC_MODULE_MID_MIDCALIBQCTASK_H
#define QC_MODULE_MID_MIDCALIBQCTASK_H

#include "QualityControl/TaskInterface.h"

#include <array>
#include <memory>
#include "MID/DigitsHelper.h"

class TH1F;
class TH2F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::mid
{

/// \brief Count number of digits per detector elements

class CalibQcTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  CalibQcTask() = default;
  /// Destructor
  ~CalibQcTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  void resetDisplayHistos();

  DigitsHelper mDigitsHelper; ///! Digits helper

  std::unique_ptr<TH1F> mNbTimeFrame{ nullptr };
  std::unique_ptr<TH1F> mNbNoiseROF{ nullptr };
  std::unique_ptr<TH1F> mNbDeadROF{ nullptr };

  std::array<std::unique_ptr<TH1F>, 4> mMultNoiseB{};
  std::array<std::unique_ptr<TH1F>, 4> mMultNoiseNB{};

  std::unique_ptr<TH1F> mNoise{ nullptr };

  std::array<std::unique_ptr<TH2F>, 4> mBendNoiseMap{};
  std::array<std::unique_ptr<TH2F>, 4> mNBendNoiseMap{};

  std::array<std::unique_ptr<TH1F>, 4> mMultDeadB{};
  std::array<std::unique_ptr<TH1F>, 4> mMultDeadNB{};

  std::unique_ptr<TH1F> mDead{ nullptr };

  std::array<std::unique_ptr<TH2F>, 4> mBendDeadMap{};
  std::array<std::unique_ptr<TH2F>, 4> mNBendDeadMap{};
};

} // namespace o2::quality_control_modules::mid

#endif // QC_MODULE_MID_MIDCALIBQCTASK_H
