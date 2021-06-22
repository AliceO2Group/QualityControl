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
/// \file   DataSourceSpec.cxx
/// \author Piotr Konopka
///

#include "QualityControl/DataSourceSpec.h"
#include <utility>

namespace o2::quality_control::core
{

// todo make DataSourceUtils

DataSourceSpec::DataSourceSpec(DataSourceType type, std::unordered_map<std::string, std::string> params)
  : type(type), typeSpecificParams(std::move(params))
{
  // todo: validation?
}

} // namespace o2::quality_control::core