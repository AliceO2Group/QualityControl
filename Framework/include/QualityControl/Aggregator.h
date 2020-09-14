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
/// \file   Aggregator.h
/// \author Barthelemy von Haller
///

#ifndef QC_CHECKER_AGGREGATOR_H
#define QC_CHECKER_AGGREGATOR_H

#include <string>
#include "QualityControl/QualityObject.h"
#include "QualityControl/CheckConfig.h"
#include <boost/property_tree/ptree.hpp>

namespace o2::configuration
{
class ConfigurationInterface;
}

namespace o2::quality_control::checker
{

class AggregatorInterface;

/// \brief An aggregator as found in the configuration.
///
/// A Aggregator is in charge of loading/instantiating the single aggregator from module,
/// configure them and tell whenever the aggregator policy is fulfilled.
class Aggregator
{
 public:
  /// Constructor
  /**
   * \brief Aggregator constructor
   *
   * Create an aggregator using the provided configuration.
   *
   * @param aggregatorName Aggregator name that must exist in the configuration
   * @param configurationSource Path to configuration
   */
  Aggregator(std::string aggregatorName, boost::property_tree::ptree configuration);

  /**
   * \brief Initialize the aggregator
   */
  void init();

  o2::quality_control::core::QualityObjectsType aggregate(o2::quality_control::core::QualityObjectsType* qos);

  /**
   * \brief Change the revision.
   *
   * Update the revision number with the latest global revision.
   * Expected to be changed after invoke of `aggregator(moMap)` or revision number overflow.
   */
//  void updateRevision(unsigned int revision);

  /**
   * \brief Return true if the Monitor Objects were changed accordingly to the policy
   */
//  bool isReady(std::map<std::string, unsigned int>& revisionMap);

  const std::string& getName() const { return mAggregatorConfig.checkName; };
//  o2::framework::OutputSpec getOutputSpec() const { return mOutputSpec; };
//  o2::framework::Inputs getInputs() const { return mInputs; };

  //TODO: Unique Input string
//  static o2::header::DataDescription createAggregatorerDataDescription(const std::string taskName);

  // For testing purpose
//  void setAggregatorInterface(AggregatorInterface* aggregatorInterface) { mAggregatorInterface = aggregatorInterface; };

 private:
  void initConfig(std::string aggregatorName);
  void initPolicy(std::string policyType);

  CheckConfig mAggregatorConfig;
  AggregatorInterface* mAggregatorInterface = nullptr;

  // Policy
//  std::function<bool(std::map<std::string, unsigned int>&)> mPolicy;
//  unsigned int mMORevision = 0;
//  bool mPolicyHelper = false; // Depending on policy, the purpose might change
};

} // namespace o2::quality_control::checker

#endif // QC_CHECKER_AGGREGATOR_H
