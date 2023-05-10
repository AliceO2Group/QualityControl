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
/// \file   CheckForEmptyPads.h
/// \author Laura Serksnyte
///

#ifndef QC_MODULE_TPC_CheckForEmptyPads_H
#define QC_MODULE_TPC_CheckForEmptyPads_H

#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::tpc
{

/// \brief  Check whether the cluster number for a track is smaller than 40 or 20 in Track task.
///
/// \author Laura Serksnyte
class CheckForEmptyPads : public o2::quality_control::checker::CheckInterface
{

 public:
  /// Default constructor
  CheckForEmptyPads() = default;
  /// Destructor
  ~CheckForEmptyPads() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

 private:
  ClassDefOverride(CheckForEmptyPads, 1);
  std::string summarizeMetaData(Quality quality);
  std::vector<std::string> mSectorsName;
  std::vector<Quality> mSectorsQuality;
  std::vector<std::string> mMOsToCheck2D;
  double mMediumQualityLimit;
  double mBadQualityLimit;

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

#endif // QC_MODULE_TPC_CheckForEmptyPads_H