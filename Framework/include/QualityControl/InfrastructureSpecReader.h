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
#include "QualityControl/CheckSpec.h"
#include "QualityControl/PostProcessingTaskSpec.h"
#include "QualityControl/LateTaskSpec.h"
#include "QualityControl/RecoRequestSpecs.h"
#include <boost/property_tree/ptree_fwd.hpp>

namespace o2::quality_control::core
{

// If we have to increase the performance of reading,
// we can probably improve it by writing a proper parser like for WorkflowSerializationHelpers in O2
// Also, move operators could be implemented.

namespace InfrastructureSpecReader
{
/// \brief Reads the full QC configuration structure.
InfrastructureSpec readInfrastructureSpec(const boost::property_tree::ptree& wholeTree, WorkflowType workflowType);

template <typename T>
T readSpecEntry(const std::string& entryID, const boost::property_tree::ptree& entryTree, const boost::property_tree::ptree& wholeTree);

// readers for separate parts
template <>
DataSourceSpec readSpecEntry<DataSourceSpec>(const std::string& entryID, const boost::property_tree::ptree& entryTree, const boost::property_tree::ptree& wholeTree);
template <>
TaskSpec readSpecEntry<TaskSpec>(const std::string& entryID, const boost::property_tree::ptree& entryTree, const boost::property_tree::ptree& wholeTree);
template <>
checker::CheckSpec readSpecEntry<checker::CheckSpec>(const std::string& entryID, const boost::property_tree::ptree& entryTree, const boost::property_tree::ptree& wholeTree);
template <>
checker::AggregatorSpec readSpecEntry<checker::AggregatorSpec>(const std::string& entryID, const boost::property_tree::ptree& entryTree, const boost::property_tree::ptree& wholeTree);
template <>
postprocessing::PostProcessingTaskSpec readSpecEntry<postprocessing::PostProcessingTaskSpec>(const std::string& entryID, const boost::property_tree::ptree& entryTree, const boost::property_tree::ptree& wholeTree);
template <>
LateTaskSpec readSpecEntry<LateTaskSpec>(const std::string& entryID, const boost::property_tree::ptree& entryTree, const boost::property_tree::ptree& wholeTree);
template <>
ExternalTaskSpec readSpecEntry<ExternalTaskSpec>(const std::string& entryID, const boost::property_tree::ptree& entryTree, const boost::property_tree::ptree& wholeTree);
template <>
GRPGeomRequestSpec readSpecEntry<GRPGeomRequestSpec>(const std::string& entryID, const boost::property_tree::ptree& entryTree, const boost::property_tree::ptree& wholeTree);
template <>
GlobalTrackingDataRequestSpec readSpecEntry<GlobalTrackingDataRequestSpec>(const std::string& entryID, const boost::property_tree::ptree& entryTree, const boost::property_tree::ptree& wholeTree);
template <>
CommonSpec readSpecEntry<CommonSpec>(const std::string& entryID, const boost::property_tree::ptree& entryTree, const boost::property_tree::ptree& wholeTree);

// todo: section names should be enum.
template <typename T>
std::vector<T> readSectionSpec(const boost::property_tree::ptree& wholeTree, const std::string& section)
{
  const auto& qcTree = wholeTree.get_child("qc");
  std::vector<T> sectionSpec;
  if (qcTree.find(section) != qcTree.not_found()) {
    const auto& sectionTree = qcTree.get_child(section);
    sectionSpec.reserve(sectionTree.size());
    for (const auto& [entryID, entryTree] : sectionTree) {
      sectionSpec.push_back(readSpecEntry<T>(entryID, entryTree, wholeTree));
    }
  }
  return sectionSpec;
}

std::string validateDetectorName(std::string name);

} // namespace InfrastructureSpecReader
} // namespace o2::quality_control::core

#endif //QUALITYCONTROL_INFRASTRUCTURESPECREADER_H
