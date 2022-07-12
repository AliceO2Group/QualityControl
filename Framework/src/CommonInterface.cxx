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

#include "QualityControl/CommonInterface.h"

using namespace o2::ccdb;

namespace o2::quality_control::core
{

void CommonInterface::setCustomParameters(const std::unordered_map<std::string, std::string>& parameters)
{
  mCustomParameters = parameters;
  configure();
}

void CommonInterface::loadCcdb()
{
  if (!mCcdbApi) {
    mCcdbApi = std::make_shared<CcdbApi>();
  }

  mCcdbApi->init(mCcdbUrl);
  if (!mCcdbApi->isHostReachable()) {
    ILOG(Warning, Support) << "CCDB at URL '" << mCcdbUrl << "' is not reachable." << ENDM;
  }
}

void CommonInterface::setCcdbUrl(const std::string& url)
{
  mCcdbUrl = url;
}

}