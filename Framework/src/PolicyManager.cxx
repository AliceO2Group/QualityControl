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
/// \file   PolicyManager.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/PolicyManager.h"

#include "QualityControl/QcInfoLogger.h"
#include "Common/Exceptions.h"

using namespace AliceO2::Common;

namespace o2::quality_control::checker
{

void PolicyManager::updateGlobalRevision()
{
  ++mGlobalRevision;
  if (mGlobalRevision == 0) {
    // mGlobalRevision cannot be 0
    // 0 means overflow, increment and update all check revisions to 0
    ++mGlobalRevision;
    for (auto& actor : mPoliciesByActor) {
      updateActorRevision(actor.second.actorName, 0);
    }
  }
}

void PolicyManager::updateActorRevision(const std::string& actorName, RevisionType revision)
{
  for (auto actor : mPoliciesByActor) {
  }
  if (mPoliciesByActor.count(actorName) == 0) {
    ILOG(Error) << "Cannot update revision for " << actorName << " : object not found" << ENDM;
    BOOST_THROW_EXCEPTION(ObjectNotFoundError() << errinfo_object_name(actorName));
  }
  mPoliciesByActor.at(actorName).revision = revision;
}

void PolicyManager::updateActorRevision(std::string actorName)
{
  updateActorRevision(actorName, mGlobalRevision);
}

void PolicyManager::updateObjectRevision(std::string objectName, RevisionType revision)
{
  mObjectsRevision[objectName] = revision;
}

void PolicyManager::updateObjectRevision(std::string objectName)
{
  updateObjectRevision(objectName, mGlobalRevision);
}

void PolicyManager::addPolicy(std::string actorName, std::string policyType, std::vector<std::string> objectNames, bool allObjects, bool policyHelper)
{
  FunctionType policy;
  if (policyType == "OnAll") {
    /** 
     * Run check if all MOs are updated 
     */
    policy = [&, actorName]() {
      for (const auto& objectName : mPoliciesByActor.at(actorName).inputObjects) {
        if (mObjectsRevision[objectName] <= mPoliciesByActor.at(actorName).revision) {
          // Expect: mObjectsRevision[notExistingKey] == 0
          return false;
        }
      }
      return true;
    };
  } else if (policyType == "OnAnyNonZero") {
    /**
     * Return true if any declared MOs were updated
     * Guarantee that all declared MOs are available
     */
    policy = [&, actorName]() {
      if (!mPoliciesByActor.at(actorName).policyHelper) {
        // Check if all monitor objects are available
        for (const auto& objectName : mPoliciesByActor.at(actorName).inputObjects) {
          if (!mObjectsRevision.count(objectName)) {
            return false;
          }
        }
        // From now on all MOs are available
        mPoliciesByActor.at(actorName).policyHelper = true;
      }

      for (const auto& objectName : mPoliciesByActor.at(actorName).inputObjects) {
        if (mObjectsRevision[objectName] > mPoliciesByActor.at(actorName).revision) {
          return true;
        }
      }
      return false;
    };

  } else if (policyType == "OnEachSeparately") {
    /**
     * Return true if any declared object were updated.
     * This is the same behaviour as OnAny.
     */
    policy = [&, actorName]() {
      if (mPoliciesByActor.at(actorName).allInputObjects) {
        return true;
      }

      for (const auto& objectName : mPoliciesByActor.at(actorName).inputObjects) {
        if (mObjectsRevision.count(objectName) && mObjectsRevision[objectName] > mPoliciesByActor.at(actorName).revision) {
          return true;
        }
      }
      return false;
    };

  } else if (policyType == "_OnGlobalAny") {
    /**
     * Return true if any MOs were updated.
     * Inner policy - used for `"MOs": "all"`
     * Might return true even if MO is not used in Check
     */

    policy = []() {
      // Expecting check of this policy only if any change
      return true;
    };

  } else if (policyType == "OnAny") {
    /**
     * Default behaviour
     *
     * Run check if any declared MOs are updated
     * Does not guarantee to contain all declared MOs 
     */
    policy = [&, actorName]() {
      for (const auto& objectName : mPoliciesByActor.at(actorName).inputObjects) {
        if (mObjectsRevision.count(objectName) && mObjectsRevision[objectName] > mPoliciesByActor.at(actorName).revision) {
          return true;
        }
      }
      return false;
    };
  } else {
    ILOG(Fatal) << "No policy named '" << policyType << "'" << ENDM;
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("No policy named '" + policyType + "'"));
  }

  mPoliciesByActor[actorName] = { actorName, policy, objectNames, allObjects, policyHelper };
}

bool PolicyManager::isReady(const std::string& actorName)
{
  if (mPoliciesByActor.count(actorName) == 0) {
    ILOG(Error) << "Cannot check if " << actorName << " is ready : object not found" << ENDM;
    BOOST_THROW_EXCEPTION(ObjectNotFoundError() << errinfo_object_name(actorName));
  }
  return mPoliciesByActor.at(actorName).isReady();
}

} // namespace o2::quality_control::checker