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

// std
#include <string>
// QC
#include "QualityControl/QualityObject.h"
#include "QualityControl/CheckConfig.h"
// config
#include <boost/property_tree/ptree_fwd.hpp>

namespace o2::configuration
{
class ConfigurationInterface;
}

namespace o2::quality_control::checker
{

class AggregatorInterface;
enum AggregatorSourceType { check,
                            aggregator };
struct AggregatorSource {
  AggregatorSource(const std::string& t, const std::string& n);
  AggregatorSourceType type;
  std::string name;
};

/// \brief An aggregator as found in the configuration.
///
/// An instance of this class represents a single aggregator as defined in the config file.
/// It is in charge of loading/instantiating it from a module,
/// configure it and execute the aggregation call on the underlying module.
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
  Aggregator(const std::string& aggregatorName, const boost::property_tree::ptree& configuration);

  /**
   * \brief Initialize the aggregator
   */
  void init();

  o2::quality_control::core::QualityObjectsType aggregate(core::QualityObjectsMapType& qoMap);

  const std::string& getName() const;
  std::string getPolicyName() const;
  std::vector<std::string> getObjectsNames() const;
  bool getAllObjectsOption() const;
  std::vector<AggregatorSource> getSources();
  std::vector<AggregatorSource> getSources(AggregatorSourceType type);

 private:
  CheckConfig mAggregatorConfig; // we reuse checkConfig, just consider that Check = Aggregator
  AggregatorInterface* mAggregatorInterface = nullptr;
  std::vector<AggregatorSource> mSources;
};

} // namespace o2::quality_control::checker

#endif // QC_CHECKER_AGGREGATOR_H
