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
/// \file   TRFCollectionTaskConfig.cxx
/// \author Piotr Konopka
///

#include "Common/TRFCollectionTaskConfig.h"
#include <boost/property_tree/ptree.hpp>

namespace o2::quality_control_modules::common
{

TRFCollectionTaskConfig::TRFCollectionTaskConfig(std::string name, const boost::property_tree::ptree& config)
  : PostProcessingConfig(name, config), name(name)
{
  detector = config.get<std::string>("qc.postprocessing." + name + ".detectorName");
  for (const auto& qoPath : config.get_child("qc.postprocessing." + name + ".QOs")) {
    qualityObjects.push_back(qoPath.second.data());
  }
}

} // namespace o2::quality_control_modules::common
