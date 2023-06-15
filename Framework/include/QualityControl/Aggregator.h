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
/// \file   Aggregator.h
/// \author Barthelemy von Haller
///

#ifndef QC_CHECKER_AGGREGATOR_H
#define QC_CHECKER_AGGREGATOR_H

// std
#include <string>
#include <vector>
// QC
#include "QualityControl/QualityObject.h"
#include "QualityControl/AggregatorConfig.h"
#include "QualityControl/AggregatorSource.h"
#include "QualityControl/UpdatePolicyType.h"

namespace o2::configuration
{
class ConfigurationInterface;
}

namespace o2::quality_control::core
{
struct CommonSpec;
struct Activity;
}

namespace o2::quality_control::checker
{

class AggregatorInterface;
struct AggregatorSpec;

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
   * @param configuration configuration structure of an aggregator
   */
  Aggregator(AggregatorConfig configuration);

  /**
   * \brief Initialize the aggregator
   */
  void init();

  o2::quality_control::core::QualityObjectsType aggregate(core::QualityObjectsMapType& qoMap, const core::Activity& defaultActivity = {});

  const std::string& getName() const;
  UpdatePolicyType getUpdatePolicyType() const;
  std::vector<std::string> getObjectsNames() const;
  bool getAllObjectsOption() const;
  std::vector<AggregatorSource> getSources() const;
  std::vector<AggregatorSource> getSources(core::DataSourceType type);
  const std::string& getDetector() const { return mAggregatorConfig.detectorName; };

  static AggregatorConfig extractConfig(const core::CommonSpec&, const AggregatorSpec&);

 private:
  /**
   * Filter out the list of QualityObjects and keep only the ones that have to be aggregated by this aggregator.
   * @param qoMap
   * @return
   */
  core::QualityObjectsMapType filter(core::QualityObjectsMapType& qoMap);

  AggregatorConfig mAggregatorConfig;
  AggregatorInterface* mAggregatorInterface = nullptr;
  std::vector<AggregatorSource> mSources;
};

} // namespace o2::quality_control::checker

#endif // QC_CHECKER_AGGREGATOR_H
