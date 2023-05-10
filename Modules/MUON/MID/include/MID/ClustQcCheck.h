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
/// \file   ClustQcCheck.h
/// \author Valerie Ramillien
///

#ifndef QC_MODULE_MID_MIDCLUSTQCCHECK_H
#define QC_MODULE_MID_MIDCLUSTQCCHECK_H

#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::mid
{

class ClustQcCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  ClustQcCheck() = default;
  /// Destructor
  ~ClustQcCheck() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

 private:
  float mClusterTF = 0;
  int mOrbTF = 32;
  // float scaleTime = 0.0114048; // 128 orb/TF * 3564 BC/orb * 25ns
  float scaleTime = 0.0000891; // 3564 BC/orb * 25ns
  float mClusterScale = 100;

  ClassDefOverride(ClustQcCheck, 3);
};

} // namespace o2::quality_control_modules::mid

#endif // QC_MODULE_MID_MIDCLUSTQCCHECK_H
