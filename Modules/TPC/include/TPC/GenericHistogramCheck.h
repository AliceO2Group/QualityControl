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
/// \file   GenericHistogramCheck.h
/// \author Maximilian Horst
///

#ifndef QC_MODULE_TPC_GENERICHISTOGRAMCHECK_H
#define QC_MODULE_TPC_GENERICHISTOGRAMCHECK_H

#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::tpc
{

/// \brief  Checks any 1D and 2D Histogram for their mean in X and Y agains an expected Value with a range or their StdDeviation
/// \author Maximilian Horst
class GenericHistogramCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  GenericHistogramCheck() = default;
  /// Destructor
  ~GenericHistogramCheck() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

 private:
  ClassDefOverride(GenericHistogramCheck, 2);
  std::string mAxisName;
  float mMeanX = 0;
  float mMeanY = 0;
  float mStdevX = 0;
  float mStdevY = 0;
  int mHistDimension = 0;
  bool mCheckXAxis = false;
  bool mCheckYAxis = false;
  bool mCheckRange = false;
  bool mCheckStdDev = false;
  float mExpectedValueX;
  float mRangeX;
  float mExpectedValueY;
  float mRangeY;

  std::string mBadString = "";
  std::string mMediumString = "";
  std::string mGoodString = "";
  std::string mNullString = "";

  std::string mBadStringMeta = "";
  std::string mMediumStringMeta = "";
  std::string mGoodStringMeta = "";
  std::string mNullStringMeta = "";

  std::string mMetadataComment = "";
};

} // namespace o2::quality_control_modules::tpc

#endif // QC_MODULE_TPC_GENERICHISTOGRAMCHECK_H
