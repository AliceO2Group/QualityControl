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
/// \file   PolicyManager.h
/// \author Barthelemy von Haller
///

#ifndef QC_CHECKER_POLICYMANAGER_H
#define QC_CHECKER_POLICYMANAGER_H

#include <string>
#include <map>
#include <vector>

namespace o2::quality_control::checker
{

using PolicyType = std::function<bool()>;
typedef uint32_t RevisionType;

struct PolicyActor {
  std::string name;
  PolicyType policy;
  std::vector<std::string> objects;
  bool allObjects;
  // TODO this line makes me think that lambdas are not enough because we actually need to store a state...
  bool policyHelper; // Depending on policy, the purpose might change
  RevisionType revision = 0;
};

/**
 * The PolicyManager is in charge of keeping track of all the policies needed by the various actors (Check, Aggregator).
 *
 * A policy defines when to trigger an action (`isReady()` returns true) for an actor (e.g. Check or Aggregator) based on a list of objects.
 * The caller (e.g. CheckRunner or AggregatorRunner) should call the methods like this:
 * \code{.cpp}
 *  // when initializing
 * policyManager.addPolicy("actor1", "OnAny", {"object1"}, false, false);*
 *
 *  // in run() loop :
 *  // upon receiving new data, i.e. object1
 *  policyManager.updateObjectRevision("object1");
 *  // check if we should do something
 *  if (policyManager.isReady("object1") {
 *    doSomething();
 *    policyManager.updateActorRevision("actor1");
 *  }
 *
 *  policyManager.updateGlobalRevision();
 *  // end run() loop
 * \endcode
 *
 * The following policies are available:
 *    - OnAny: triggers when an object is received that matches ANY object listed as a data source of the policy.
 *    - OnAnyNonZero: triggers only if all objects have been received at least once, then trigger the same way as onAny.
 *    - onAll: triggers when ALL objects listed as data source of the policy have been updated at least once.
 *    - onEachSeparately: synonym of 'onEach'.
 *
 * If "all" is specified as list of object, or the list is empty, we always trigger.
 */
class PolicyManager
{
 public:
  /**
   * \brief Update the global revision number.
   *
   * This function function should be called at the end of a processing loop (typically the run() method).
   */
  void updateGlobalRevision();
  /**
   * \brief Update the revision number associated with an actor.
   *
   * This function is typically called after the actor has been triggered based on its policy and its work is done.
   * @param actorName
   * @param revision
   */
  void updateActorRevision(std::string actorName, RevisionType revision);
  void updateActorRevision(std::string actorName);
  /**
   * \brief Update the revision number associated with an object.
   *
   * This function is typically called after a new object has been received.
   * @param objectName
   * @param revision
   */
  void updateObjectRevision(std::string objectName, RevisionType revision);
  void updateObjectRevision(std::string objectName);
  /**
   * Add a policy for the given actor.
   * @param actorName
   * @param policyType One of the policy names: OnAll, OnAnyNonZero, OnEachSeparately, OnAny
   */
  void addPolicy(std::string actorName, std::string policyType, std::vector<std::string> objectNames, bool allObjects, bool policyHelper);
  /**
   * Checks whether the given actor is ready or not based on its policy and the revisions.
   * @param actorName
   * @return
   */
  bool isReady(std::string actorName);

 private:
  std::map<std::string, PolicyActor> mActors; // actor name -> struct
  RevisionType mGlobalRevision = 1;
  std::map<std::string, RevisionType> mObjectsRevision; // object name -> revision
};

}

#endif // QC_CHECKER_POLICYMANAGER_H