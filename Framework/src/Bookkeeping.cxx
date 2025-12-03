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
#include <filesystem>
#include <fstream>
#include <thread>

using namespace o2::bkp::api;

namespace o2::quality_control::core
{

std::string readClientToken()
{
  // first we try to find the token in the environment variable
  if (auto tokenEnv = std::getenv("QC_BKP_CLIENT_TOKEN"); tokenEnv != NULL && std::strlen(tokenEnv) > 0) {
    ILOG(Info, Ops) << "Using token from environment variable QC_BKP_CLIENT_TOKEN" << ENDM;
    return tokenEnv;
  }

  // if the environment variable is not set, we try to read it from a file
  const std::string tokenFileName = "qc_bkp_client_token.txt";
  std::filesystem::path tokenPath = std::filesystem::current_path() / tokenFileName;

  std::error_code ec;
  if (std::filesystem::exists(tokenPath, ec) && !ec.value()) {
    std::string token;
    std::ifstream tokenFile(tokenPath);
    // from now on, we throw if something goes wrong, because the user is clearly trying to use a token file
    if (!tokenFile.is_open()) {
      throw std::runtime_error("BKP token file '" + tokenFileName + "' was provided but cannot be opened, check permissions");
    }
    std::getline(tokenFile, token);
    if (token.empty()) {
      throw std::runtime_error("BKP token file '" + tokenFileName + "' was provided but it is empty, please provide a valid token");
    }
    ILOG(Debug, Devel) << "Using token from file qc_bkp_client_token.txt" << ENDM;
    return token;
  }

  ILOG(Debug, Devel) << "Could not find an env var QC_BKP_CLIENT_TOKEN nor a qc_bkp_client_token.txt file, using BKP client without an authentication token" << ENDM;
  return "";
}

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

  const auto token = readClientToken();

  try {
    if (!token.empty()) {
      mClient = BkpClientFactory::create(url, token);
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

  std::thread([this, runNumber, type, name, args, detector]() {
    try{
      this->mClient->dplProcessExecution()->registerProcessExecution(runNumber, type, getHostName(), name, args, detector);
    } catch (std::runtime_error& error) { // catch here because we are in a thread
      ILOG(Warning, Devel) << "Failed registration to the BookKeeping: " << error.what() << ENDM;
    }
  }).detach();
}

std::vector<int> Bookkeeping::sendFlagsForSynchronous(uint32_t runNumber, const std::string& detectorName, const std::vector<QcFlag>& qcFlags)
{
  if (!mInitialized) {
    return {};
  }
  return mClient->qcFlag()->createForSynchronous(runNumber, detectorName, qcFlags);
}

std::vector<int> Bookkeeping::sendFlagsForDataPass(uint32_t runNumber, const std::string& passName, const std::string& detectorName, const std::vector<QcFlag>& qcFlags)
{
  if (!mInitialized) {
    return {};
  }
  return mClient->qcFlag()->createForDataPass(runNumber, passName, detectorName, qcFlags);
}

std::vector<int> Bookkeeping::sendFlagsForSimulationPass(uint32_t runNumber, const std::string& productionName, const std::string& detectorName, const std::vector<QcFlag>& qcFlags)
{
  if (!mInitialized) {
    return {};
  }
  return mClient->qcFlag()->createForSimulationPass(runNumber, productionName, detectorName, qcFlags);
}

} // namespace o2::quality_control::core
