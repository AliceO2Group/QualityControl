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
///

#ifndef QC_MODULE_TPC_CHECKOFSLICES_H
#define QC_MODULE_TPC_CHECKOFSLICES_H

#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::tpc
{

/// \brief  Check if all the slices in a Trending (e.g. as a function of TPC Pad) are within their uncertainty compatible with the mean or a predefined physical value
/// \author Maximilian Horst
/// \author Laura Serksnyte
class CheckOfSlices : public o2::quality_control::checker::CheckInterface
{

  // ILOG(Warning,Support) << "##########################################################################################Start Check Header file:" << ENDM;

 public:
  /// Default constructor
  CheckOfSlices() = default;
  /// Destructor
  ~CheckOfSlices() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

 private:
  ClassDefOverride(CheckOfSlices, 2);
  std::string mCheckChoice;
  float mExpectedPhysicsValue;
  float mNSigmaExpectedPhysicsValue;
  float mNSigmaMediumExpectedPhysicsValue;
  float mNSigmaBadExpectedPhysicsValue;
  float mNSigmaMean;
  float mNSigmaMediumMean;
  float mNSigmaBadMean;
  float meanFull;
  static constexpr std::string_view mCheckChoiceMean = "Mean";
  static constexpr std::string_view mCheckChoiceExpectedPhysicsValue = "ExpectedPhysicsValue";
  static constexpr std::string_view mCheckChoiceBoth = "Both";
};

} // namespace o2::quality_control_modules::tpc

#endif // QC_MODULE_TPC_CHECKOFSLICES_H