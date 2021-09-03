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
/// \file   MonitorObjectCollection.cxx
/// \author Piotr Konopka
///

#include "QualityControl/MonitorObjectCollection.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QcInfoLogger.h"

#include <Mergers/MergerAlgorithm.h>

using namespace o2::mergers;

namespace o2::quality_control::core
{

void MonitorObjectCollection::merge(mergers::MergeInterface* const other)
{
  auto otherCollection = dynamic_cast<MonitorObjectCollection*>(other); // reinterpret_cast maybe?
  if (otherCollection == nullptr) {
    throw std::runtime_error("The other object is not a MonitorObjectCollection");
  }

  auto otherIterator = otherCollection->MakeIterator();
  while (auto otherObject = otherIterator->Next()) {
    if (auto targetObject = this->FindObject(otherObject->GetName())) {
      auto otherMO = dynamic_cast<MonitorObject*>(otherObject);
      auto targetMO = dynamic_cast<MonitorObject*>(targetObject);
      if (otherMO && targetMO) {
        // That might be another collection or a concrete object to be merged, we walk on the collection recursively.
        algorithm::merge(targetMO->getObject(), otherMO->getObject());
        // we extend the validity interval to the largest of the two objects.
        auto otherValidity = otherMO->getValidity();
        targetMO->updateValidity(otherValidity.getMin());
        targetMO->updateValidity(otherValidity.getMax());
      } else {
        throw std::runtime_error("The target object or the other object could not be casted to MonitorObject.");
      }
    } else {
      // We prefer to clone instead of passing the pointer in order to simplify deleting the `other`.
      this->Add(otherObject->Clone());
    }
  }
  delete otherIterator;
}

void MonitorObjectCollection::postDeserialization()
{
  auto it = this->MakeIterator();
  while (auto obj = it->Next()) {
    auto mo = dynamic_cast<MonitorObject*>(obj);
    if (mo == nullptr) {
      ILOG(Warning) << "Could not cast an object of type '" << obj->ClassName() << "' in MonitorObjectCollection to MonitorObject, skipping." << ENDM;
      continue;
    }
    mo->setIsOwner(true);
    if (mo->getObject() != nullptr && mo->getObject()->InheritsFrom(TCollection::Class())) {
      auto collection = dynamic_cast<TCollection*>(mo->getObject());
      collection->SetOwner(true);
    }
  }
  this->SetOwner(true);
  delete it;
}

} // namespace o2::quality_control::core
