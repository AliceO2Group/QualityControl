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
#include "BookkeepingApi/BkpClientFactory.h"
#include "BookkeepingApi/BkpClient.h"
#include <unistd.h>

using namespace o2::bkp::api;

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
    if (auto tokenEnv = std::getenv("QC_BKP_CLIENT_TOKEN"); tokenEnv != NULL) {
      mClient = BkpClientFactory::create(url, tokenEnv);
    } else {
      mClient = BkpClientFactory::create(url);
    }
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

std::string getHostName()
{
  char hostname[256];
  if (gethostname(hostname, sizeof(hostname)) == 0) {
    return { hostname };
  } else {
    return "";
  }
}

void Bookkeeping::registerProcess(int runNumber, const std::string& name, const std::string& detector, bkp::DplProcessType type, const std::string& args)
{
  if (!mInitialized) {
    return;
  }
  mClient->dplProcessExecution()->registerProcessExecution(runNumber, type, getHostName(), name, args, detector);
}
} // namespace o2::quality_control::core
