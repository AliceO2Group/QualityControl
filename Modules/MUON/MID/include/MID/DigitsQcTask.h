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
/// \file   DigitsQcTask.h
/// \author Bogdan Vulpescu
/// \author Xavier Lopez
/// \author Guillaume Taillepied
/// \author Valerie Ramillien

#ifndef QC_MODULE_MID_MIDDIGITSQCTASK_H
#define QC_MODULE_MID_MIDDIGITSQCTASK_H

#include <array>
#include <memory>

#include <TH1.h>
#include <TH2.h>

#include "QualityControl/TaskInterface.h"
#include "MIDBase/Mapping.h"
#include "MID/DigitsHelper.h"

class TH1F;
class TH2F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::mid
{

/// \brief Count number of digits per detector elements

class DigitsQcTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  DigitsQcTask() = default;
  /// Destructor
  ~DigitsQcTask() override = default;

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

  std::unique_ptr<TH2F> mROFTimeDiff{ nullptr };

  std::unique_ptr<TH1F> mNbDigitTF{ nullptr };

  std::array<std::unique_ptr<TH1F>, 5> mMultHitB{};
  std::array<std::unique_ptr<TH1F>, 5> mMultHitNB{};
  std::unique_ptr<TH1F> mMeanMultiHits;

  std::array<std::unique_ptr<TH2F>, 5> mLocalBoardsMap{};
  std::unique_ptr<TH1F> mHits;

  std::array<std::unique_ptr<TH2F>, 4> mBendHitsMap{};
  std::array<std::unique_ptr<TH2F>, 4> mNBendHitsMap{};

  std::unique_ptr<TH1F> mDigitBCCounts{ nullptr };

};

} // namespace o2::quality_control_modules::mid

#endif // QC_MODULE_MID_MIDDIGITSQCTASK_H
