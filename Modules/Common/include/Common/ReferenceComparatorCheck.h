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
/// \file   ReferenceComparatorCheck.h
/// \author Andrea Ferrero
/// \brief  A generic QC check that compares a given set of histograms with their corresponding references
///

#ifndef QUALITYCONTROL_ReferenceComparatorCheck_H
#define QUALITYCONTROL_ReferenceComparatorCheck_H

#include "QualityControl/CheckInterface.h"
#include "Common/ObjectComparatorInterface.h"

#include <sstream>
#include <unordered_map>

class TPaveText;

namespace o2::quality_control_modules::common
{

/// \brief  A generic QC check that compares a given set of histograms with their corresponding references
/// \author Andrea Ferrero
class ReferenceComparatorCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  ReferenceComparatorCheck() = default;
  /// Destructor
  ~ReferenceComparatorCheck() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void reset() override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

  void startOfActivity(const Activity& activity) override;
  void endOfActivity(const Activity& activity) override;

 private:
  Quality getSinglePlotQuality(std::shared_ptr<MonitorObject> mo, std::string& message);

  std::unique_ptr<ObjectComparatorInterface> mComparator;
  std::map<std::string, Quality> mQualityFlags;
  std::map<std::string, std::shared_ptr<TPaveText>> mQualityLabels;
  quality_control::core::Activity mReferenceActivity;
  bool mIgnorePeriodForReference{ true }; /// whether to specify the period name in the reference run query
  bool mIgnorePassForReference{ true };   /// whether to specify the pass name in the reference run query
  size_t mReferenceRun;
  std::unordered_map<std::string, std::shared_ptr<MonitorObject>> mReferencePlots;
};

} // namespace o2::quality_control_modules::common

#endif // QC_MODULE_SKELETON_ReferenceComparatorCheck_H
