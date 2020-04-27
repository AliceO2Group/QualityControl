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
/// \file   MonitorObjectCollection.cxx
/// \author Piotr Konopka
///

#include "QualityControl/MonitorObjectCollection.h"

#include "QualityControl/MonitorObject.h"

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
      } else {
        throw std::runtime_error("The target object or the other object could not be casted to MonitorObject.");
      }
    } else {
      // We prefer to clone instead of passing the pointer in order to simplify deleting the `other`.
      this->Add(otherObject->Clone());
    }
  }
}

} // namespace o2::quality_control::core
