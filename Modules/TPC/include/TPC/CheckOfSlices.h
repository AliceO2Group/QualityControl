// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   CheckOfSlices.h
/// \author Maximilian Horst
/// \author Laura Serksnyte
/// \author Marcel Lesch
///

#ifndef QC_MODULE_TPC_CHECKOFSLICES_H
#define QC_MODULE_TPC_CHECKOFSLICES_H
#include <string_view>
#include <string>
#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::tpc
{

/// \brief  Check if all the slices in a Trending (e.g. as a function of TPC sector) are within their uncertainty compatible with the mean or a predefined physical value
/// \author Maximilian Horst
/// \author Laura Serksnyte
/// \author Marcel Lesch
class CheckOfSlices : public o2::quality_control::checker::CheckInterface
{

 public:
  /// Default constructor
  CheckOfSlices() = default;
  /// Destructor
  ~CheckOfSlices() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override { return "TCanvas"; }

 private:
  ClassDefOverride(CheckOfSlices, 3);
  std::string createMetaData(const std::vector<std::string>& pointMetaData);
  std::string mCheckChoice;
  double mExpectedPhysicsValue;
  double mNSigmaExpectedPhysicsValue;
  double mNSigmaBadExpectedPhysicsValue;
  double mNSigmaMean;
  double mNSigmaBadMean;
  double mRangeMedium;
  double mRangeBad;
  bool mSliceTrend;

  double mMean = 0;
  double mStdev;

  std::string mBadString = "";
  std::string mMediumString = "";
  std::string mGoodString = "";
  std::string mNullString = "";

  std::string mMetadataComment;

  bool mRangeCheck = false;
  bool mExpectedValueCheck = false;
  bool mMeanCheck = false;
  bool mZeroCheck = false;
};

} // namespace o2::quality_control_modules::tpc

#endif // QC_MODULE_TPC_CHECKOFSLICES_H