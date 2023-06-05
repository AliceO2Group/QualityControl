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
#include "QualityControl/Activity.h"
#include "BookkeepingApi/BkpProtoClientFactory.h"
#include "BookkeepingApi/BkpProtoClient.h"

using namespace o2::bkp::api::proto;

namespace o2::quality_control::core
{

void Bookkeeping::init(const std::string& url)
{
  if (mInitialized) {
    if (mUrl == url) {
      ILOG(Debug, Devel) << "Bookkeeping already initialized with the same URL, ignoring." << ENDM;
      return;
    } else {
      ILOG(Warning, Support) << "Initializing the Bookkeeping although it has already been initialized with a different URL (" << url << " vs " << mUrl << ENDM;
    }
  }

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

  if (mClient == nullptr) { // make sure we did not get an empty pointer
    ILOG(Warning, Support) << "Error - we got an empty pointer to Bookkeeping" << ENDM;
    return;
  }

  ILOG(Debug, Devel) << "Bookkeeping initialized" << ENDM;
  mInitialized = true;
}

void Bookkeeping::populateActivity(Activity& activity, size_t runNumber)
{
  if (!mInitialized) {
    return;
  }
  try {
    auto bkRun = mClient->run()->Get(runNumber, { bookkeeping::RUN_RELATIONS_LHC_FILL });
    ILOG(Debug, Devel) << "Retrieved run info from Bookkeeping : " << bkRun->run().environmentid() << ", " << bkRun->run().runtype() << ENDM;
    activity.mId = bkRun->run().runnumber();
    activity.mType = bkRun->run().runtype();
    activity.mPeriodName = bkRun->run().lhcperiod();
    activity.mValidity.setMin(bkRun->run().timeo2start());
    activity.mValidity.setMax(bkRun->run().timeo2end());
    activity.mBeamType = bkRun->lhcfill().beamtype();
    ILOG(Debug, Devel) << "activity created from run : " << activity << ENDM;
  } catch (std::runtime_error& error) {
    ILOG(Warning, Support) << "Error retrieving run info from Bookkeeping: " << error.what() << ENDM;
  }
}
} // namespace o2::quality_control::core
