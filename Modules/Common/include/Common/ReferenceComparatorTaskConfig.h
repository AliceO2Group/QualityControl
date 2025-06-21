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
/// \file   ReferenceComparatorTaskConfig.h
/// \author Andrea Ferrero
/// \brief  ReferenceComparatorTask configuration structure
///

#ifndef QUALITYCONTROL_REFERENCECOMPARATORTASKCONFIG_H
#define QUALITYCONTROL_REFERENCECOMPARATORTASKCONFIG_H

#include <vector>
#include <map>
#include <string>
#include "QualityControl/PostProcessingConfig.h"

namespace o2::quality_control_modules::common
{

/// \brief  ReferenceComparatorTask configuration structure
struct ReferenceComparatorTaskConfig : quality_control::postprocessing::PostProcessingConfig {
  ReferenceComparatorTaskConfig() = default;
  ReferenceComparatorTaskConfig(std::string name, const boost::property_tree::ptree& config);
  ~ReferenceComparatorTaskConfig() = default;

  struct DataGroup {
    std::string name;
    // QCDB path where the source objects are located
    std::string inputPath;
    // QCDB path where the reference objects are located
    std::string referencePath;
    // QCDB path where the output plots are uploaded
    std::string outputPath;
    // wether to normalize the reference plot with respect to the current one
    bool normalizeReference{ false };
    // wether to only draw the current/reference ratio, or the inidividual histograms as well
    bool drawRatioOnly{ false };
    // space reserved for the legend above the histograms, in fractions of the pad height
    double legendHeight{ 0.2 };
    // ROOT option to be used for drawing 1-D plots ("HIST" by default)
    std::string drawOption1D{ "HIST" };
    // ROOT option to be used for drawing 2-D plots ("COLZ" by default)
    std::string drawOption2D{ "COLZ" };
    // list of QCDB objects in this group
    std::vector<std::string> objects;
  };

  std::vector<DataGroup> dataGroups;
};

} // namespace o2::quality_control_modules::common

#endif // QUALITYCONTROL_REFERENCECOMPARATORTASKCONFIG_H
