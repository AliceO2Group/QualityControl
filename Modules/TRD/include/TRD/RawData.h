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
/// \file   RawData.h
/// \author My Name
///

#ifndef QC_MODULE_TRD_TRDRAWDATA_H
#define QC_MODULE_TRD_TRDRAWDATA_H

#include "QualityControl/TaskInterface.h"
#include "DataFormatsTRD/RawDataStats.h"
#include <array>

class TH1F;
class TH2F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::trd
{

/// \brief Example Quality Control DPL Task
/// \author My Name
class RawData final : public TaskInterface
{
 public:
  /// \brief Constructor
  RawData() = default;
  /// Destructor
  ~RawData() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

  void buildHistograms();
  void resetHistograms();

 private:
  TH1F* mDataAcceptance = nullptr;
  TH2F* mDataVolumePerHalfSector = nullptr;
  TH2F* mDataVolumePerHalfSectorCru = nullptr;
  TH1F* mTotalChargevsTimeBin = nullptr; //
  TH1F* mDigitHCID = nullptr;
  TH1F* mTimeFrameTime = nullptr;
  TH1F* mTrackletParsingTime = nullptr;
  TH1F* mDigitParsingTime = nullptr;
  TH1F* mDataVersions = nullptr;
  TH1F* mDataVersionsMajor = nullptr;
  TH1F* mParsingErrors = nullptr;
  std::array<TH2F*, 10> mLinkErrors;
  std::array<TH2F*, o2::trd::ParsingErrors::TRDLastParsingError> mParsingErrors2d;

  std::array<TH1F*, 540> fClusterChamberAmplitude;
};

} // namespace o2::quality_control_modules::trd

#endif // QC_MODULE_TRD_TRDRAWDATA_H
