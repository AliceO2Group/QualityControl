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
#include "QualityControl/ActivityHelpers.h"

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

  bool reportedMismatchingRunNumbers = false;
  auto otherIterator = otherCollection->MakeIterator();
  while (auto otherObject = otherIterator->Next()) {
    auto otherObjectName = otherObject->GetName();
    if (std::strlen(otherObjectName) == 0) {
      ILOG(Warning, Devel) << "The other object does not have a name, probably it is empty. Skipping..." << ENDM;
    } else if (auto targetObject = this->FindObject(otherObjectName)) {
      // A corresponding object in the target collection was found, we try to merge.
      auto otherMO = dynamic_cast<MonitorObject*>(otherObject);
      auto targetMO = dynamic_cast<MonitorObject*>(targetObject);
      if (!otherMO || !targetMO) {
        throw std::runtime_error("The target object or the other object could not be casted to MonitorObject.");
      }
      if (!reportedMismatchingRunNumbers && targetMO->getActivity().mId != otherMO->getActivity().mId) {
        ILOG(Error, Ops) << "The run number of the input object '" << otherMO->GetName() << "' ("
                         << otherMO->getActivity().mId << ") "
                         << "does not match the run number of the target object '"
                         << targetMO->GetName() << "' (" << targetMO->getActivity().mId
                         << "). Trying to continue, but THIS SHOULD BE IMMEDIATELY ADDRESSED IN PRODUCTION. "
                         << "QC objects from other setups are reaching this one. Will not report more mismatches in this collection."
                         << ENDM;
        reportedMismatchingRunNumbers = true;
      }
      // That might be another collection or a concrete object to be merged, we walk on the collection recursively.
      algorithm::merge(targetMO->getObject(), otherMO->getObject());
      targetMO->updateValidity(otherMO->getValidity().getMin());
      targetMO->updateValidity(otherMO->getValidity().getMax());
    } else {
      // A corresponding object in the target collection could not be found.
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
      ILOG(Warning) << "Could not cast an object of type '" << obj->ClassName()
                    << "' in MonitorObjectCollection to MonitorObject, skipping." << ENDM;
      continue;
    }
    mo->setIsOwner(true);
    if (auto objPtr = mo->getObject(); objPtr != nullptr) {
      if (objPtr->InheritsFrom(MergeInterface::Class())) {
        auto mergeable = dynamic_cast<MergeInterface*>(mo->getObject());
        mergeable->postDeserialization();
      } else if (objPtr->InheritsFrom(TCollection::Class())) {
        // if a class inherits from both MergeInterface and TCollection, we assume that MergeInterface does the correct job of setting the ownership.
        auto collection = dynamic_cast<TCollection*>(mo->getObject());
        collection->SetOwner(true);
      }
    }
  }
  this->SetOwner(true);
  delete it;
}

void MonitorObjectCollection::setDetector(const std::string& detector)
{
  mDetector = detector;
}

const std::string& MonitorObjectCollection::getDetector() const
{
  return mDetector;
}

} // namespace o2::quality_control::core
