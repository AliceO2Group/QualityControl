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
/// \file   NumPhysTriggCheck.h
/// \author Joshua Koenig
///

#ifndef QC_MODULE_EMCAL_EMCALNUMPHYSTRIGCHECK_HH
#define QC_MODULE_EMCAL_EMCALNUMPHYSTRIGCHECK_HH

#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::emcal
{

/// \brief  Check whether a plot is empty or not.
///
/// \author Barthelemy von Haller
class NumPhysTriggCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// \brief Default constructor
  NumPhysTriggCheck() = default;
  /// \brief Destructor
  ~NumPhysTriggCheck() override = default;

  /// \brief Configure checker setting thresholds from taskParameters where specified
  void configure() override;

  /// \brief Check whether the number of physics triggers for the current trigger falls below a certain threshold copared to the maximum in the time range
  /// \param moMap List of histos to check
  /// \return Quality of the selection
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;

  /// \brief Beautify the monitoring objects
  /// \param mo Monitoring object to beautify
  /// \param checkResult Quality status of this checker
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;

  /// \brief Accept only TH1 histograms as input
  /// \return Name of the accepted object: TH1  ClassDefOverride(NumPhysTriggCheck, 1);

 private:
  double mFracToMaxGood = 0.5; ///< Thresholds for minimum fraction of physics triggers compared to maximum
};

} // namespace o2::quality_control_modules::emcal

#endif // QC_MODULE_EMCAL_EMCALNUMPHYSTRIGCHECK_HH
