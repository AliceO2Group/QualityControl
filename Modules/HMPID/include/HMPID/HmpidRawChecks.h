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
/// \file   HmpidRawChecks.h
/// \author Nicola Nicassio
///

#ifndef QC_MODULE_HMPID_RAWCHECK_H
#define QC_MODULE_HMPID_RAWCHECK_H

#include "HMPID/Helpers.h"
#include "QualityControl/CheckInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include <TH1.h>
#include <TH2.h>
#include <TProfile.h>
#include <TProfile2D.h>
#include <string>
#include <TColor.h>

// using namespace o2::quality_control::core;

namespace o2::quality_control_modules::hmpid
{

/// \brief  Check if the occupancy on each equipment is between the two specified values
///
/// \author Nicola Nicassio
class HmpidRawChecks : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  HmpidRawChecks() = default;
  /// Destructor
  ~HmpidRawChecks() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  // private:
  std::array<Quality, 14> check_hHmpBigMap(TProfile2D* h); // <-- Dimensione da chiarire
  std::array<Quality, 42> check_hHmpHvSectorQ(TH2F* h);
  std::array<Quality, 14> check_hHmpPadOccPrf(TProfile* h);
  std::array<Quality, 14> check_hBusyTime(TProfile* h);
  std::array<Quality, 14> check_hEventSize(TProfile* h);

  std::string m_hHmpBigMap_HistName{ "hHmpBigMap_profile" };
  std::string m_hHmpHvSectorQ_HistName{ "hHmpHvSectorQ" };
  std::string m_hHmpPadOccPrf_HistName{ "hHmpPadOccPrf" };
  std::string m_hBusyTime_HistName{ "hBusyTime" };
  std::string m_hEventSize_HistName{ "hEventSize" };
  std::string m_hCheckHV_HistName{ "hCheckHV" };

  // Variables for min/max plot

  double mMinOccupancy{ 0. };
  double mMaxOccupancy{ 10 };

  double mMinEventSize{ 0.2 };
  double mMaxEventSize{ 0.9 };

  double mMinBusyTime{ 0.2 };
  double mMaxBusyTime{ 0.9 };

  double mMinHVTotalEntriesToCheckQuality{ 1000 };
  double mFractionXBinsHVSingleModuleEntriesToLabelGoodBadQuality{ 0.005 };

  // Checker identifiers

  double mMaxBadDDLForMedium{ 1 };
  double mMaxBadDDLForBad{ 3 };

  double mMaxBadHVForMedium{ 7 };
  double mMaxBadHVForBad{ 10 };

  std::vector<std::string> mErrorMessages;
  std::vector<Color_t> mErrorMessagesColor;

  Quality mQualityOccupancy;
  Quality mQualityBusyTime;
  Quality mQualityEventSize;
  Quality mQualityBigMap;
  Quality mQualityHvSectorQ;
  std::array<Quality, 42> qualityHvSectorQ;

  ClassDefOverride(HmpidRawChecks, 1);
};

} // namespace o2::quality_control_modules::hmpid
//} // namespace o2::quality_control::core
#endif // QC_MODULE_HMPID_RAWCHECK_H
