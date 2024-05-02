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
/// \file   RawDataReaderCheck.h
/// \author Lucia Anna Tarasovicova
///

#ifndef QC_MODULE_CTP_CTPRAWDATAREADERCHECK_H
#define QC_MODULE_CTP_CTPRAWDATAREADERCHECK_H

#include "QualityControl/CheckInterface.h"
#include "CommonConstants/LHCConstants.h"
#include <bitset>
class TH1F;

namespace o2::quality_control_modules::ctp
{

/// \brief  This class is checking the expected BC filling scheme
/// \author Lucia Anna Tarasovicova
class RawDataReaderCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  RawDataReaderCheck() = default;
  /// Destructor
  ~RawDataReaderCheck() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;
  void startOfActivity(const Activity& activity) override;
  static constexpr double_t nofOrbitsPerTF = 32;
  static constexpr double_t TimeTF = nofOrbitsPerTF*o2::constants::lhc::LHCOrbitMUS/1e6; // in seconds
  ClassDefOverride(RawDataReaderCheck, 5);

 private:
  int getRunNumberFromMO(std::shared_ptr<MonitorObject> mo);
  int getNumberFilledBins(TH1F* hist);
  int checkChange(TH1F* fHistDiference,TH1F* fHistPrev, std::vector<int>& vIndexBad, std::vector<int>& vIndexMedium);
  int mRunNumber;
  long int mTimestamp;
  float mThreshold;
  float mThresholdRateBad;
  float mThresholdRateMedium;
  float mThresholdRateRatioBad;
  float mThresholdRateRatioMedium;
  int cycleCounter;
  int mIndexMBclass = -1;
  float mFraction;
  int mCycleDuration;
  bool flagRatio = false;
  bool flagInput = false;
  TH1F* fHistInputPrevious = nullptr;
  TH1F* fHistClassesPrevious = nullptr;
  TH1F* fHistInputRatioPrevious = nullptr;
  TH1F* fHistClassRatioPrevious = nullptr;
  std::vector<int> vGoodBC;
  std::vector<int> vMediumBC;
  std::vector<int> vBadBC;
  std::vector<int> vIndexBad;
  std::vector<int> vIndexMedium;
  std::bitset<o2::constants::lhc::LHCMaxBunches> mLHCBCs;
};

} // namespace o2::quality_control_modules::ctp

#endif // QC_MODULE_CTP_CTPRAWDATAREADERCHECK_H
