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
/// \file   DigitQcTask.h
/// \author Artur Furs afurs@cern.ch
/// RecoQC Task for FT0 detector

#ifndef QC_MODULE_FIT_RECOFITQCTASK_H
#define QC_MODULE_FIT_RECOFITQCTASK_H

#include <Framework/InputRecord.h>

#include "QualityControl/QcInfoLogger.h"

#include <DataFormatsFDD/RecPoint.h>
#include <DataFormatsFT0/RecPoints.h>
#include <DataFormatsFV0/RecPoints.h>
#include "CommonConstants/LHCConstants.h"

#include "QualityControl/TaskInterface.h"
#include <memory>
#include <regex>
#include <set>
#include <map>
#include <array>
#include <type_traits>
#include <boost/algorithm/string.hpp>
#include <TH1.h>
#include <TH2.h>
#include <TList.h>
#include <Rtypes.h>

#include "FITCommon/DigitSync.h"
#include <FITCommon/HelperFIT.h>
using namespace o2::quality_control::core;

namespace o2::quality_control_modules::fit
{

class RecoFITQcTask final : public TaskInterface
{
 public:
  using DigitSyncFIT = DigitSync<o2::fdd::RecPoint, o2::ft0::RecPoints, o2::fv0::RecPoints>;
  /// \brief Constructor
  RecoFITQcTask() = default;
  /// Destructor
  ~RecoFITQcTask() override;
  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  constexpr static int sNBC = o2::constants::lhc::LHCMaxBunches; // 3564 BCs

  bool mIsFDD{ false };
  bool mIsFT0{ false };
  bool mIsFV0{ false };

  const uint16_t mNTrgBitsFDD = HelperTrgFIT::sMapBasicTrgBitsFDD.size();
  const uint16_t mNTrgBitsFT0 = HelperTrgFIT::sMapBasicTrgBitsFT0.size();
  const uint16_t mNTrgBitsFV0 = HelperTrgFIT::sMapBasicTrgBitsFV0.size();
  const uint16_t mNTrgBitsFT0_FV0 = mNTrgBitsFT0 * mNTrgBitsFV0;

  const float mCFDChannel2NS = 0.01302; // CFD channel width in ns

  std::function<uint16_t(uint8_t, uint8_t)> mFuncTrgStatusFDD_FT0 = [&](uint8_t trgBitFDD, uint8_t trgBitFT0) {
    return static_cast<uint16_t>(trgBitFDD) * mNTrgBitsFT0 + static_cast<uint16_t>(trgBitFT0);
  };
  std::function<uint16_t(uint8_t, uint8_t)> mFuncTrgStatusFDD_FV0 = [&](uint8_t trgBitFDD, uint8_t trgBitFV0) {
    return static_cast<uint16_t>(trgBitFDD) * mNTrgBitsFV0 + static_cast<uint16_t>(trgBitFV0);
  };
  std::function<uint16_t(uint8_t, uint8_t)> mFuncTrgStatusFT0_FV0 = [&](uint8_t trgBitFT0, uint8_t trgBitFV0) {
    return static_cast<uint16_t>(trgBitFT0) * mNTrgBitsFV0 + static_cast<uint16_t>(trgBitFV0);
  };
  std::function<uint16_t(uint8_t, uint8_t, uint8_t)> mFuncTrgStatusFDD_FT0_FV0 = [&](uint8_t trgBitFDD, uint8_t trgBitFT0, uint8_t trgBitFV0) {
    return static_cast<uint16_t>(trgBitFDD) * mNTrgBitsFT0_FV0 + static_cast<uint16_t>(trgBitFT0) * mNTrgBitsFV0 + static_cast<uint16_t>(trgBitFV0);
  };

  // Objects which will be published
  std::unique_ptr<TH2F> mHistTrgCorrelationFDD_FT0;
  std::unique_ptr<TH2F> mHistTrgCorrelationFDD_FV0;
  std::unique_ptr<TH2F> mHistTrgCorrelationFT0_FV0;
  // std::unique_ptr<TH2F> mHistTrgCorrelationFDD_FT0_FV0;
  std::array<std::unique_ptr<TH2F>, 5> mHistTrgCorrelationFT0_FDD_FV0;
};

} // namespace o2::quality_control_modules::fit

#endif // QC_MODULE_FT0_FT0RecoQcTask_H
