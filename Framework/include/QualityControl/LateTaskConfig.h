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

#include "QualityControl/UserCodeConfig.h"
#include "QualityControl/DataSourceSpec.h"

#include <Framework/DataProcessorSpec.h>


namespace o2::quality_control::core {

// fixme: should it be LateTaskConfig instead?
struct LateTaskConfig : public UserCodeConfig {

  // fixme: we should think what should happen when a user wants a critical late task relying on a non-critical QC task.
  //  should it be marked as resilient?
  bool critical = true;
};

}

#endif //QC_CORE_LATETASKRUNNERCONFIG_H
