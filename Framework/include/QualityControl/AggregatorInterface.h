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
/// \file   AggregatorInterface.h
/// \author Barthelemy von Haller
///

#ifndef QC_CHECKER_AGGREGATORINTERFACE_H
#define QC_CHECKER_AGGREGATORINTERFACE_H

#include <unordered_map>
#include <map>

#include "QualityControl/QualityObject.h"
#include "QualityControl/Quality.h"

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
  ///
  /// \param name The name of the aggregator.
  virtual void configure(std::string name) = 0;

  /// \brief Returns new qualities (usually fewer) based on the input qualities
  ///
  /// @param qoMap A map of the the QualityObjects to aggregate and their full names.
  /// @return The new qualities, associated with a name.
  virtual std::map<std::string, o2::quality_control::core::Quality> aggregate(std::map<std::string, std::shared_ptr<const o2::quality_control::core::QualityObject>>& qoMap) = 0;

  /// \brief Set the custom parameters for this aggregator.
  /// Set the custom parameters for this aggregator. It is usually the ones defined in the configuration.
  /// \param parameters
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
