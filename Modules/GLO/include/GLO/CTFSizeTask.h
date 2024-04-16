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
/// \file   CTFSizeTask.h
/// \author Ole Schmidt
///

#ifndef QC_MODULE_GLO_CTFSIZETASK_H
#define QC_MODULE_GLO_CTFSIZETASK_H

#include "QualityControl/TaskInterface.h"
#include <DetectorsCommonDataFormats/DetID.h>

#include <tuple>
#include <array>
#include <string>

class TH1F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::glo
{

class CTFSize final : public TaskInterface
{
 public:
  /// \brief Constructor
  CTFSize() = default;
  /// Destructor
  ~CTFSize() override;

  std::tuple<int, float, float> getBinningFromConfig(o2::detectors::DetID::ID iDet, const Activity& activity) const;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  std::array<TH1F*, o2::detectors::DetID::CTP + 1> mHistSizes{ nullptr };       // CTF size per TF for each detector with different binning per detector
  std::array<TH1F*, o2::detectors::DetID::CTP + 1> mHistSizesLog{ nullptr };    // CTF size per TF with same axis scale for all detectors
  std::array<std::string, o2::detectors::DetID::CTP + 1> mDefaultBinning{ "" }; // default number of bins and limits for mHistSizes (customizable)
  bool mPublishingDone{ false };
};

} // namespace o2::quality_control_modules::glo

#endif // QC_MODULE_GLO_CTFSIZETASK_H
