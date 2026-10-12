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

#ifndef QC_CORE_LATETASKRUNNERCONFIG_H
#define QC_CORE_LATETASKRUNNERCONFIG_H

///
/// \file   LateTaskRunnerConfig.h
/// \author Piotr Konopka
///

#include <Framework/DataProcessorSpec.h>

#include "QualityControl/UserCodeConfig.h"
#include "QualityControl/DataSourceSpec.h"
#include "QualityControl/OutputActivityStrategy.h"

namespace o2::quality_control::core
{

struct LateTaskConfig : public UserCodeConfig {
  bool critical = true;
  OutputActivityStrategy outputActivityStrategy = OutputActivityStrategy::Integrated;
};

} // namespace o2::quality_control::core

#endif // QC_CORE_LATETASKRUNNERCONFIG_H
