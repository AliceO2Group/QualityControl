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
/// \file   CommonInterface.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/UserCodeInterface.h"

using namespace o2::ccdb;

namespace o2::quality_control::core
{

void UserCodeInterface::setCustomParameters(const CustomParameters& parameters)
{
  mCustomParameters = parameters;
  configure();
}

void UserCodeInterface::setCcdbUrl(const std::string& url)
{
  o2::ccdb::BasicCCDBManager::instance().setURL(url);
}

const std::string& UserCodeInterface::getName() const { return mName; }

void UserCodeInterface::setName(const std::string& name) { mName = name; }

} // namespace o2::quality_control::core