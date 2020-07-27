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
/// \file   CheckRawMultiplicity.h
/// \author Nicolo' Jacazio
/// \brief  Checker for the raw hit multiplicity obtained with the TaskDigits
///

#ifndef QC_MODULE_TOF_TOFCHECKRAWSMULTI_H
#define QC_MODULE_TOF_TOFCHECKRAWSMULTI_H

#include "QualityControl/CheckInterface.h"
#include "TString.h"

namespace o2::quality_control_modules::tof
{

/// \brief  Check whether a plot is empty or not.
///
/// \author Nicolo' Jacazio
class CheckRawMultiplicity : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  CheckRawMultiplicity();
  /// Destructor
  ~CheckRawMultiplicity() override;

  // Override interface
  void configure(std::string name) override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult) override;
  std::string getAcceptedType() override;

  /// Minimum value of TOF raw hit multiplicity
  Float_t minTOFrawhits;
  /// Maximum value of TOF raw hit multiplicity
  Float_t maxTOFrawhits;
  /// Fraction of the total integral which are considered Ok at 0 mult
  const Float_t fracAtZeroMult = 0.75;
  /// Fraction of the total integral which are considered Ok at low mult
  const Float_t fracAtLowMult = 0.75;
  /// Maximum average TOF raw hit multiplicity in Pb-Pb
  const Float_t maxTOFrawhitsPbPb = 500;

 private:
  /// Mean of the TOF hit multiplicity histogram
  Float_t multiMean;
  /// Number of events with 0 TOF hits
  Float_t zeroBinIntegral;
  /// Number of events with low TOF hits multiplicity
  Float_t lowMIntegral;
  /// Number of events with TOF hits multiplicity > 0
  Float_t totIntegral;
  /// Message to print
  TString shifter_msg = "";

  ClassDefOverride(CheckRawMultiplicity, 1);
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_TOFCHECKRAWSMULTI_H
