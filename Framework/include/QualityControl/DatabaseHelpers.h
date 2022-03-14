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
/// \file   DatabaseHelpers.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_DATABASEHELPERS_H
#define QUALITYCONTROL_DATABASEHELPERS_H

#include <map>
#include <string>
#include <boost/property_tree/ptree_fwd.hpp>
#include "QualityControl/Activity.h"

namespace o2::quality_control::repository::database_helpers
{

std::map<std::string, std::string> asDatabaseMetadata(const core::Activity&, bool putDefault = true);
core::Activity asActivity(const std::map<std::string, std::string>& metadata, const std::string& provenance = "qc");
core::Activity asActivity(const boost::property_tree::ptree&, const std::string& provenance = "qc");

} // namespace o2::quality_control::repository::database_helpers

#endif // QUALITYCONTROL_DATABASEHELPERS_H
