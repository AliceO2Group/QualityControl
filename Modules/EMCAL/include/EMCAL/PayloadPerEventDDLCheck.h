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
/// \file   CellOccupancyCheck.h
/// \author Ananya Rai
///

#ifndef QC_MODULE_EMCAL_EMCALPAYLOADPEREVENTDDLCHECK_H
#define QC_MODULE_EMCAL_EMCALPAYLOADPEREVENTDDLCHECK_H

#include "QualityControl/CheckInterface.h"
#include <TH2.h>

namespace o2::quality_control_modules::emcal
{

/// \brief  Check whether payload/event for DDL is okay
///
/// \author Ananya Rai
class PayloadPerEventDDLCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  PayloadPerEventDDLCheck() = default;
  /// Destructor
  ~PayloadPerEventDDLCheck() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;

 private:
  //    /************************************************
  //     * reference histograms                             *
  //     ************************************************/
  TH2* mCalibReference = nullptr;
  double mChiSqMedPayloadThresh = 3;    ///< If ChiSq greater than this value, quality is medium
  double mChiSqBadLowPayloadThresh = 5; ///< Low threshold of bad ChiSq

  ClassDefOverride(PayloadPerEventDDLCheck, 3);
};

} // namespace o2::quality_control_modules::emcal

#endif // QC_MODULE_EMCAL_EMCALPAYLOADPEREVENTDDLCHECK_H
