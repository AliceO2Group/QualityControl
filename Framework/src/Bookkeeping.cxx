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
/// \file   Bookkeeping.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/Bookkeeping.h"
#include "QualityControl/QcInfoLogger.h"
#include "BookkeepingApi/BkpProtoClientFactory.h"

using namespace o2::bkp::api::proto;

namespace o2::quality_control::core
{

void Bookkeeping::init(std::string url)
{
  if (url.empty()) {
    ILOG(Warning, Support) << "No URL provided for Bookkeeping. Nothing will be stored nor retrieved." << ENDM;
    return;
  }
  try {
    mClient = BkpProtoClientFactory::create(url);
  } catch (std::runtime_error& error) {
    ILOG(Warning, Support) << "Error connecting to Bookkeeping: " << error.what() << ENDM;
    return;
  }
  ILOG(Debug, Devel) << "Bookkeeping initialized" << ENDM;
  mInitialized = true;
}

void Bookkeeping::populateActivity(Activity& activity, size_t runNumber)
{
  if(!mInitialized) {
    return;
  }
  try {
    auto bkRun = mClient->run()->Get(runNumber);
    ILOG(Debug, Devel) << "Retrieved run info from Bookkeeping : " << bkRun->environmentid() << ", " << bkRun->runtype() << ENDM;
    activity.mId = bkRun->runnumber();
    activity.mType = bkRun->runtype();
    activity.mPeriodName = bkRun->lhcperiod();
    activity.mValidity.setMin(bkRun->timeo2start());
    activity.mValidity.setMax(bkRun->timeo2end());
//    activity.mBeamType = bkRun->pdpbeamtype();   // uncomment when we receive the proper beam type
    ILOG(Debug, Devel) << "activity created from run : " << activity << ENDM;
  } catch (std::runtime_error& error) {
    ILOG(Warning, Support) << "Error retrieving run info from Bookkeeping: " << error.what() << ENDM;
  }
}
} // namespace o2::quality_control::core
