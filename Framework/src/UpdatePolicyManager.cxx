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
/// \file   UpdatePolicyManager.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/UpdatePolicyManager.h"

#include "QualityControl/QcInfoLogger.h"
#include "Common/Exceptions.h"

using namespace AliceO2::Common;

namespace o2::quality_control::checker
{

void UpdatePolicyManager::updateGlobalRevision()
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

void UpdatePolicyManager::updateActorRevision(const std::string& actorName, RevisionType revision)
{
  if (mPoliciesByActor.count(actorName) == 0) {
    ILOG(Error, Support) << "Cannot update revision for " << actorName << " : object not found" << ENDM;
    BOOST_THROW_EXCEPTION(ObjectNotFoundError() << errinfo_object_name(actorName));
  }
  mPoliciesByActor.at(actorName).revision = revision;
}

void UpdatePolicyManager::updateActorRevision(std::string actorName)
{
  updateActorRevision(actorName, mGlobalRevision);
}

void UpdatePolicyManager::updateObjectRevision(std::string objectName, RevisionType revision)
{
  mObjectsRevision[objectName] = revision;
}

void UpdatePolicyManager::updateObjectRevision(std::string objectName)
{
  updateObjectRevision(objectName, mGlobalRevision);
}

void UpdatePolicyManager::addPolicy(std::string actorName, UpdatePolicyType policyType, std::vector<std::string> objectNames, bool allObjects, bool policyHelper)
{
  UpdatePolicyFunctionType policy;
  switch (policyType) {
    case UpdatePolicyType::OnAll: {
      /**
       * Run check if all MOs are updated
       */
      policy = [&, actorName]() {
        for (const auto& objectName : mPoliciesByActor.at(actorName).inputObjects) {
          if (mObjectsRevision.count(objectName) == 0 || mObjectsRevision.at(objectName) <= mPoliciesByActor.at(actorName).revision) {
            return false;
          }
        }
        return true;
      };
      break;
    }
    case UpdatePolicyType::OnAnyNonZero: {
      /**
       * Return true if any declared MOs were updated
       * Guarantee that all declared MOs are available
       */
      policy = [&, actorName]() {
        if (!mPoliciesByActor.at(actorName).policyHelperFlag) {
          // Check if all monitor objects are available
          for (const auto& objectName : mPoliciesByActor.at(actorName).inputObjects) {
            if (!mObjectsRevision.count(objectName)) {
              return false;
            }
          }
          // From now on all MOs are available
          mPoliciesByActor.at(actorName).policyHelperFlag = true;
        }

        for (const auto& objectName : mPoliciesByActor.at(actorName).inputObjects) {
          if (mObjectsRevision[objectName] > mPoliciesByActor.at(actorName).revision) {
            return true;
          }
        }
        return false;
      };
      break;
    }
    case UpdatePolicyType::OnEachSeparately: {
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
      break;
    }
    case UpdatePolicyType::OnGlobalAny: {
      /**
       * Return true if any MOs were updated.
       * Inner policy - used for `"MOs": "all"`
       * Might return true even if MO is not used in Check
       */

      policy = []() {
        // Expecting check of this policy only if any change
        return true;
      };
      break;
    }
    case UpdatePolicyType::OnAny: {
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
      break;
    }
  }

  mPoliciesByActor[actorName] = { actorName, policy, objectNames, allObjects, policyHelper };

  ILOG(Info, Devel) << "Added a policy : " << mPoliciesByActor[actorName] << ENDM;
}

bool UpdatePolicyManager::isReady(const std::string& actorName)
{
  if (mPoliciesByActor.count(actorName) == 0) {
    ILOG(Error, Support) << "Cannot check if " << actorName << " is ready : object not found" << ENDM;
    BOOST_THROW_EXCEPTION(ObjectNotFoundError() << errinfo_object_name(actorName));
  }
  return mPoliciesByActor.at(actorName).isReady();
}

std::ostream& operator<<(std::ostream& out, const UpdatePolicy& updatePolicy) // output
{
  out << "actorName: " << updatePolicy.actorName
      << "; allInputObjects: " << updatePolicy.allInputObjects
      << "; policyHelperFlag: " << updatePolicy.policyHelperFlag
      << "; revision: " << updatePolicy.revision
      << "; inputObjects: ";
  for (const auto& item : updatePolicy.inputObjects) {
    out << item << ", ";
  }
  return out;
}

void UpdatePolicyManager::reset()
{
  mPoliciesByActor.clear();
  mObjectsRevision.clear();
  mGlobalRevision = 1;
}

} // namespace o2::quality_control::checker