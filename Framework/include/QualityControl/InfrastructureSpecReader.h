// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#ifndef QUALITYCONTROL_INFRASTRUCTURESPECREADER_H
#define QUALITYCONTROL_INFRASTRUCTURESPECREADER_H

///
/// \file   InfrastructureSpecReader.h
/// \author Piotr Konopka
///

#include "QualityControl/InfrastructureSpec.h"
#include "QualityControl/TaskSpec.h"
#include "QualityControl/CommonSpec.h"
#include "QualityControl/DataSourceSpec.h"
#include <boost/property_tree/ptree_fwd.hpp>

namespace o2::quality_control::core
{

// If have to increase the performance of reading,
// we can probably improve it by writing a proper parser like for WorkflowSerializationHelpers in O2
// Also, move operators could be implemented.

class InfrastructureSpecReader
{
 public:
  /// \brief Reads the full QC configuration file.
  // todo remove configurationSource when it is possible
  static InfrastructureSpec readInfrastructureSpec(const boost::property_tree::ptree& wholeTree, const std::string& configurationSource);

  // readers for separate parts
  static CommonSpec readCommonSpec(const boost::property_tree::ptree& commonTree, const std::string& configurationSource);
  static TaskSpec readTaskSpec(std::string taskName, const boost::property_tree::ptree& taskTree, const boost::property_tree::ptree& wholeTree);
  static DataSourceSpec readDataSourceSpec(const boost::property_tree::ptree& dataSourceTree, const boost::property_tree::ptree& wholeTree);

  static std::string validateDetectorName(std::string name);
};

} // namespace o2::quality_control::core

#endif //QUALITYCONTROL_INFRASTRUCTURESPECREADER_H
