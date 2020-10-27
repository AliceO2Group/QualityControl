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
/// \file   PhysicsCheck.h
/// \author Andrea Ferrero, Sebastien Perrin
///

#ifndef QC_MODULE_MCH_PHYSICSCHECK_H
#define QC_MODULE_MCH_PHYSICSCHECK_H

#include "QualityControl/CheckInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include <string>

namespace o2::quality_control_modules::muonchambers
{

/// \brief  Check if the occupancy on each pad is between the two specified values
///
/// \author Andrea Ferrero, Sebastien Perrin
class PhysicsCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  PhysicsCheck();
  /// Destructor
  ~PhysicsCheck() override;

  // Override interface
  void configure(std::string name) override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

 private:
  int mPrintLevel;
  double minOccupancy;
  double maxOccupancy;
  ClassDefOverride(PhysicsCheck, 1);
};

} // namespace o2::quality_control_modules::muonchambers

#endif // QC_MODULE_TOF_TOFCHECKRAWSTIME_H
