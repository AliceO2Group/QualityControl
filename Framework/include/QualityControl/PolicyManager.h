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

using FunctionType = std::function<bool()>;
typedef uint32_t RevisionType;

/**
 * Represents 1 policy and all its associated elements.
 */
struct Policy {
  std::string actorName;
  FunctionType isReady;
  std::vector<std::string> inputObjects;
  bool allInputObjects;
  // TODO this line makes me think that lambdas are not enough because we actually need to store a state...
  bool policyHelper; // the purpose might change depending on policy,
  RevisionType revision = 0;
};

/**
 * The PolicyManager is in charge of instantiating and keeping track of policies.
 *
 * Naming:
 *   - a `caller` (e.g. CheckRunner or AggregatorRunner) holds an instance of the PolicyManager and drives it.
 *   - a `policy` determines whether something is ready to be done or not. It is a glorified function returning
 *     a boolean.
 *   - an `actor` (e.g. Check or Aggregator) is in charge of executing something when a policy is fulfilled. There
 *     can be several actors for a caller.
 *   - the `objects` are received by the caller. They are processed by the actors and their status (e.g. freshly received)
 *     is used by some policies.
 *   - a revision is a number associated to each object to determine when it was received and associated to
 *     each actor to determine when it was last time triggered.
 *
 * The following policies are available:
 *   - OnAny: triggers when an object is received that matches ANY object listed as a data source of the policy.
 *   - OnAnyNonZero: triggers only if all objects have been received at least once, then trigger the same way as onAny.
 *   - onAll: triggers when ALL objects listed as data source of the policy have been updated at least once.
 *   - onEachSeparately: synonym of 'onEach'.
 * If "all" is specified as list of object, or the list is empty, we always trigger.
 *
 * A typical caller code looks like this:
 * \code{.cpp}
 *  // when initializing
 * policyManager.addPolicy("actor1", "OnAny", {"object1"}, false, false);
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
  void updateActorRevision(const std::string& actorName, RevisionType revision);
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
   * @param objectNames
   * @param allObjects
   * @param policyHelper
   */
  void addPolicy(std::string actorName, std::string policyType, std::vector<std::string> objectNames, bool allObjects, bool policyHelper);
  /**
   * Checks whether the given actor is ready or not.
   * @param actorName
   * @return
   */
  bool isReady(const std::string& actorName);

 private:
  std::map<std::string /* Actor name */, Policy> mPoliciesByName;
  RevisionType mGlobalRevision = 1;
  std::map<std::string, RevisionType> mObjectsRevision; // object name -> revision
};

} // namespace o2::quality_control::checker

#endif // QC_CHECKER_POLICYMANAGER_H