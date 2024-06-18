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
#include "DetectorsBase/GRPGeomHelper.h"
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
  const double_t nofOrbitsPerTF = o2::base::GRPGeomHelper::instance().getNHBFPerTF();
  const double_t TimeTF = nofOrbitsPerTF * o2::constants::lhc::LHCOrbitMUS / 1e6; // in seconds

 private:
  int getRunNumberFromMO(std::shared_ptr<MonitorObject> mo);
  int getNumberFilledBins(TH1F* hist);
  int checkChange(TH1F* fHist, TH1F* fHistPrev, std::vector<int>& vIndexBad, std::vector<int>& vIndexMedium);
  Quality SetQualityResult(std::vector<int>& vBad, std::vector<int>& vMedium); // methods start with a lowercase
  void ClearIndexVectors(); // methods start with a lowercase

  long int mTimestamp;
  float mThreshold = -1;
  float mThresholdRateBad;
  float mThresholdRateMedium;
  float mThresholdRateRatioBad;
  float mThresholdRateRatioMedium;
  float mFraction;
  int mCycleDuration;
  bool flagRatio = false; // member variables start with a `m`
  bool flagInput = false;
  TH1F* fHistInputPrevious = nullptr; // member variables start with a `m`
  TH1F* fHistClassesPrevious = nullptr;
  TH1F* fHistInputRatioPrevious = nullptr;
  TH1F* fHistClassRatioPrevious = nullptr;
  std::vector<int> vGoodBC; // member variables start with a `m`
  std::vector<int> vMediumBC;
  std::vector<int> vBadBC;
  std::vector<int> vIndexBad;
  std::vector<int> vIndexMedium;
  std::bitset<o2::constants::lhc::LHCMaxBunches> mLHCBCs;
  const char* ctpinputs[49] = { " T0A", " T0C", " TVX", " TSC", " TCE", " VBA", " VOR", " VIR", " VNC", " VCH", "11", "12", " UCE", "DMC", " USC", " UVX", " U0C", " U0A", "COS", "LAS", "EMC", " PH0", "23", "24", "ZED", "ZNC", "PHL", "PHH", "PHM", "30", "31", "32", "33", "34", "35", "36", "EJ1", "EJ2", "EG1", "EG2", "DJ1", "DG1", "DJ2", "DG2", "45", "46", "47", "48", "49" };

  ClassDefOverride(RawDataReaderCheck, 5);
};

} // namespace o2::quality_control_modules::ctp

#endif // QC_MODULE_CTP_CTPRAWDATAREADERCHECK_H
