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
/// \file   TOFCheckDiagnostic.h
/// \author Nicolo' Jacazio
///

#ifndef QC_MODULE_TOF_TOFCHECKDIAGNOSTIC_H
#define QC_MODULE_TOF_TOFCHECKDIAGNOSTIC_H

#include "QualityControl/CheckInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

namespace o2::quality_control_modules::tof
{

/// \brief  Check whether a plot is empty or not.
///
/// \author Barthelemy von Haller
class TOFCheckDiagnostic : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  TOFCheckDiagnostic();
  /// Destructor
  ~TOFCheckDiagnostic() override;

  // Override interface
  void configure(std::string name) override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult) override;
  std::string getAcceptedType() override;

  //   /// Minimum value for TOF raw time
  //   Float_t minTOFrawTime;
  //   /// Maximum value for TOF raw time
  //   Float_t maxTOFrawTime;

 private:
  //   /// Mean of the TOF raw time distribution
  //   Float_t timeMean;
  //   /// Integral of the TOF raw time distribution in the peak region i.e. within minTOFrawTime and maxTOFrawTime
  //   Float_t peakIntegral;
  //   /// Integral of the TOF raw time distribution in the whole histogram range
  //   Float_t totIntegral;

  ClassDefOverride(TOFCheckDiagnostic, 1);
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_TOFCHECKDIAGNOSTIC_H
