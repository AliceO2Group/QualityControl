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
class TH1D;

namespace o2::quality_control_modules::ctp
{

/// \brief  This class is checking the expected BC filling scheme
/// \brief This class is checking the relative change of ctp input and ctp class rates and ratios to MB
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
  int checkChange(TH1D* mHist, TH1D* mHistPrev);
  int checkChangeOfRatio(TH1D* mHist, TH1D* mHistPrev, TH1D* mHistAbs);
  Quality setQualityResult(std::vector<int>& vBad, std::vector<int>& vMedium);
  void clearIndexVectors();
  long int mTimestamp;
  float mThreshold = -1;                   // threshold for BCs
  float mThresholdRateBad = -1;            // threshold for the relative change in ctp input and class rates
  float mThresholdRateMedium = -1;         // threshold for the relative change in ctp input and class rates
  float mThresholdRateRatioBad = -1;       // threshold for the relative change in ctp input and class ratios
  float mThresholdRateRatioMedium = -1;    // threshold for the relative change in ctp input and class ratios
  float mNSigBC = -1;                      // n sigma for BC threshold
  bool mFlagRatio = false;                 // flag that a ratio plot is checked
  bool mFlagInput = false;                 // flag that an input plot is checked
  TH1D* mHistInputPrevious = nullptr;      // histogram storing ctp input rates from previous cycle
  TH1D* mHistClassesPrevious = nullptr;    // histogram storing ctp class rates from previous cycle
  TH1D* mHistInputRatioPrevious = nullptr; // histogram storing ctp input ratios to MB from previous cycle
  TH1D* mHistClassRatioPrevious = nullptr;
  TH1D* mHistAbsolute = nullptr;                                                                                                                                                                                                                                                                                                                                         // histogram storing ctp class ratios to MB from previous cycle
  std::vector<int> mVecGoodBC;                                                                                                                                                                                                                                                                                                                                           // vector of good BC positions
  std::vector<int> mVecMediumBC;                                                                                                                                                                                                                                                                                                                                         // vector of medium BC positions, we expect a BC at this position, but inputs are below mThreshold
  std::vector<int> mVecBadBC;                                                                                                                                                                                                                                                                                                                                            // vector of bad BC positions, we don't expect a BC at this position, but inputs are abow mThreshold
  std::vector<int> mVecIndexBad;                                                                                                                                                                                                                                                                                                                                         // vector of ctp input and class indices, which had a big relative change
  std::vector<int> mVecIndexMedium;                                                                                                                                                                                                                                                                                                                                      // vector of ctp input and class indices, which had a relative change
  std::bitset<o2::constants::lhc::LHCMaxBunches> mLHCBCs;                                                                                                                                                                                                                                                                                                                // LHC filling scheme
  const char* ctpinputs[49] = { "T0A", "T0C", "TVX", "TSC", "TCE", "VBA", "VOR", "VIR", "VNC", "VCH", "11", "12", "UCE", "DMC", "USC", "UVX", "U0C", "U0A", "COS", "LAS", "EMC", "PH0", "23", "24", "ZED", "ZNC", "PHL", "PHH", "PHM", "30", "31", "32", "33", "34", "35", "36", "EJ1", "EJ2", "EG1", "EG2", "DJ1", "DG1", "DJ2", "DG2", "45", "46", "47", "48", "49" }; // ctp input names

  ClassDefOverride(RawDataReaderCheck, 9);
};

} // namespace o2::quality_control_modules::ctp

#endif // QC_MODULE_CTP_CTPRAWDATAREADERCHECK_H
