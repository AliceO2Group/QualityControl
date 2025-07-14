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
/// \file   ReductorHelpers.cxx
/// \author Piotr Konopka
///

#include "QualityControl/ReductorHelpers.h"

#include "QualityControl/Reductor.h"
#include "QualityControl/ReductorTObject.h"
#include "QualityControl/ReductorConditionAny.h"
#include "QualityControl/Triggers.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/ConditionAccess.h"

namespace o2::quality_control::postprocessing::reductor_helpers::implementation
{

bool updateReductorImpl(Reductor* r, const Trigger& t, const std::string& path, const std::string& name, const std::string& type,
                        repository::DatabaseInterface& qcdb, core::ConditionAccess& ccdbAccess)
{
  if (r == nullptr) {
    return false;
  }

  if (type == "repository") {
    auto mo = qcdb.retrieveMO(path, name, t.timestamp, t.activity, t.metadata);
    TObject* obj = mo ? mo->getObject() : nullptr;
    auto reductorTObject = dynamic_cast<ReductorTObject*>(r);
    if (obj && reductorTObject) {
      reductorTObject->update(obj);
      return true;
    }
  } else if (type == "repository-quality") {
    auto qo = qcdb.retrieveQO(path + "/" + name, t.timestamp, t.activity, t.metadata);
    auto reductorTObject = dynamic_cast<ReductorTObject*>(r);
    if (qo && reductorTObject) {
      reductorTObject->update(qo.get());
      return true;
    }
  } else if (type == "condition") {
    auto reductorConditionAny = dynamic_cast<ReductorConditionAny*>(r);
    if (reductorConditionAny) {
      auto conditionPath = name.empty() || path.empty() ? path + name : path + "/" + name;
      reductorConditionAny->update(ccdbAccess, t.timestamp, conditionPath);
      return true;
    }
  }
  return false;
}

} // namespace o2::quality_control::postprocessing::reductor_helpers::implementation
