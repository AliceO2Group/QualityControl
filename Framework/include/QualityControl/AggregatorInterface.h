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
/// \file   AggregatorInterface.h
/// \author Barthelemy von Haller
///

#ifndef QC_CHECKER_AGGREGATORINTERFACE_H
#define QC_CHECKER_AGGREGATORINTERFACE_H

#include <unordered_map>

#include "QualityControl/QualityObject.h"
#include "QualityControl/Quality.h"

using namespace o2::quality_control::core;

namespace o2::quality_control::checker
{

/// \brief  Skeleton of a quality aggregator user algorithm.
///
/// \author Barthelemy von Haller
class AggregatorInterface
{
 public:
  /// Default constructor
  AggregatorInterface() = default;
  /// Destructor
  virtual ~AggregatorInterface() = default;

  /// \brief Configure the aggregator based on its name.
  ///
  /// The configuration of the object can't be done in the constructor because
  /// ROOT needs an argument-less constructor when streaming it. We use this method
  /// to configure the object. The name might be used to ask the configuration system
  /// for specific parameters.
  virtual void configure(std::string d) = 0;

  /// \brief Returns new qualities (usually fewer) based on the input qualities
  ///
  /// @param aoMap A map of the the QualityObjects to aggregate and their full names.
  /// @return The quality associated with these objects.
  virtual std::vector<Quality> aggregate(std::map<std::string, std::shared_ptr<const o2::quality_control::core::QualityObject>>& qoMap) = 0;

  void setCustomParameters(const std::unordered_map<std::string, std::string>& parameters)
  {
    mCustomParameters = parameters;
  }

 protected:
  std::unordered_map<std::string, std::string> mCustomParameters;

  ClassDef(AggregatorInterface, 1)
};

} // namespace o2::quality_control::checker

#endif /* QC_CHECKER_AGGREGATORINTERFACE_H */
