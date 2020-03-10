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
/// \file   TOFCheckRawsMulti.h
/// \author Nicolo' Jacazio
///

#ifndef QC_MODULE_TOF_TOFCHECKRAWSMULTI_H
#define QC_MODULE_TOF_TOFCHECKRAWSMULTI_H

#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::tof
{

/// \brief  Check whether a plot is empty or not.
///
/// \author Nicolo' Jacazio
class TOFCheckRawsMulti : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  TOFCheckRawsMulti();
  /// Destructor
  ~TOFCheckRawsMulti() override;

  // Override interface
  void configure(std::string name) override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult) override;
  std::string getAcceptedType() override;

  /// Minumum value of TOF raw hit multiplicity
  Float_t minTOFrawhits;
  /// Maximum value of TOF raw hit multiplicity
  Float_t maxTOFrawhits;

 private:
  /// Mean of the TOF hit multiplicity histogram
  Float_t multiMean;
  /// Number of events with 0 TOF hits
  Float_t zeroBinIntegral;
  /// Number of events with low TOF hits multiplicity
  Float_t lowMIntegral;
  /// Number of events with TOF hits multiplicity > 0
  Float_t totIntegral;

  ClassDefOverride(TOFCheckRawsMulti, 1);
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_TOFCHECKRAWSMULTI_H
