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
/// @file     Bookkeeping.h
/// @author   Barthelemy von Haller

#ifndef QC_CORE_BOOKKEEPING_H
#define QC_CORE_BOOKKEEPING_H

#include <string>
#include "BookkeepingApi/BkpClient.h"

namespace o2::quality_control::core
{
class Activity;

// \brief A singleton class to handle the bookkeeping service interactions.
//
// All calls in QC code to Bookkeeping service should go through this class.
class Bookkeeping
{
 public:
  static Bookkeeping& getInstance()
  {
    static Bookkeeping instance;
    return instance;
  }

  // disable non-static
  Bookkeeping& operator=(const Bookkeeping&) = delete;
  Bookkeeping(const Bookkeeping&) = delete;

  void init(const std::string& url);
  void registerProcess(int runNumber, const std::string& name, const std::string& detector, bkp::DplProcessType type, const std::string& args);

  // send QC flags to the bookkeeping service
  std::vector<int> sendFlagsForSynchronous(uint32_t runNumber, const std::string& detectorName, const std::vector<QcFlag>& qcFlags);
  std::vector<int> sendFlagsForDataPass(uint32_t runNumber, const std::string& passName, const std::string& detectorName, const std::vector<QcFlag>& qcFlags);
  std::vector<int> sendFlagsForSimulationPass(uint32_t runNumber, const std::string& productionName, const std::string& detectorName, const std::vector<QcFlag>& qcFlags);

 private:
  Bookkeeping() = default;

  bool mInitialized = false;
  std::string mUrl;
  std::unique_ptr<bkp::api::BkpClient> mClient;
};

} // namespace o2::quality_control::core

#endif
