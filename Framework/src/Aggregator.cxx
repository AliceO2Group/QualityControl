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
/// \file   Aggregator.cxx
/// \author Barthelemy von Haller
///

#include <Configuration/ConfigurationInterface.h>
#include "QualityControl/Aggregator.h"

o2::quality_control::checker::Aggregator::Aggregator(std::string aggregatorName, std::shared_ptr<o2::configuration::ConfigurationInterface> configuration)
{
}
void o2::quality_control::checker::Aggregator::init()
{
}
o2::quality_control::core::QualityObjectsType o2::quality_control::checker::Aggregator::aggregate(o2::quality_control::core::QualityObjectsType* qos)
{
  return o2::quality_control::core::QualityObjectsType();
}
void o2::quality_control::checker::Aggregator::initConfig(std::string aggregatorName)
{
}
void o2::quality_control::checker::Aggregator::initPolicy(std::string policyType)
{
}
