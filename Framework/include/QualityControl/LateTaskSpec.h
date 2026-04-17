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

#ifndef QC_CORE_LATETASKSPEC_H
#define QC_CORE_LATETASKSPEC_H

///
/// \file   LateTaskSpec.h
/// \author Piotr Konopka
///

#include <string>
#include <vector>

#include "QualityControl/DataSourceSpec.h"
#include "QualityControl/CustomParameters.h"
#include "QualityControl/UpdatePolicyType.h"
#include "QualityControl/OutputActivityStrategy.h"

namespace o2::quality_control::core
{

/// \brief Specification of a Task, which should map the JSON configuration structure.
struct LateTaskSpec {
  // default, invalid spec
  LateTaskSpec() = default;
  // basic
  std::string taskName = "Invalid";
  std::string className = "Invalid";
  std::string moduleName = "Invalid";
  std::string detectorName = "Invalid";
  std::vector<DataSourceSpec> dataSources;

  // advanced
  bool active = true;
  bool critical = true;
  OutputActivityStrategy outputActivityStrategy = OutputActivityStrategy::Integrated;
  CustomParameters customParameters;

  // reco
  // GRPGeomRequestSpec grpGeomRequestSpec;
  // GlobalTrackingDataRequestSpec globalTrackingDataRequest;
};
} // namespace o2::quality_control::core
#endif // QC_CORE_LATETASKSPEC_H
