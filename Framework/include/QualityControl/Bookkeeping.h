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
#include "BookkeepingApi/BkpProtoClient.h"

namespace o2::quality_control::core
{
class Activity;

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
  void populateActivity(Activity& activity, size_t runNumber);
  void registerProcess(int runNumber, const std::string& name, const std::string& detector, bookkeeping::DplProcessType type, const std::string& args);

 private:
  Bookkeeping() = default;

  bool mInitialized = false;
  std::string mUrl;
  std::unique_ptr<bkp::api::proto::BkpProtoClient> mClient;
};

} // namespace o2::quality_control::core

#endif
