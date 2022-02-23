
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
/// \file   DummyDatabase.cxx
/// \author Piotr Konopka
///

#include "QualityControl/DummyDatabase.h"

using namespace o2::quality_control::core;
namespace o2::quality_control::repository
{

void DummyDatabase::connect(std::string, std::string, std::string, std::string)
{
}

void DummyDatabase::connect(const std::unordered_map<std::string, std::string>&)
{
}

void DummyDatabase::storeAny(const void*, std::type_info const&, std::string const&, std::map<std::string, std::string> const&,
                             std::string const&, std::string const&, long, long)
{
}

void DummyDatabase::storeMO(std::shared_ptr<const o2::quality_control::core::MonitorObject>, long, long)
{
}

std::shared_ptr<MonitorObject> DummyDatabase::retrieveMO(std::string, std::string, long)
{
  return std::shared_ptr<MonitorObject>();
}

void DummyDatabase::storeQO(std::shared_ptr<const o2::quality_control::core::QualityObject>, long, long)
{
}

void DummyDatabase::storeTRFC(std::shared_ptr<const o2::quality_control::TimeRangeFlagCollection> trfc)
{
}

std::shared_ptr<QualityObject> DummyDatabase::retrieveQO(std::string, long)
{
  return std::shared_ptr<QualityObject>();
}

void DummyDatabase::disconnect()
{
}

void DummyDatabase::prepareTaskDataContainer(std::string)
{
}

std::vector<std::string> DummyDatabase::getPublishedObjectNames(std::string)
{
  return std::vector<std::string>();
}

void DummyDatabase::truncate(std::string, std::string)
{
}

TObject* DummyDatabase::retrieveTObject(std::string, const std::map<std::string, std::string>&, long, std::map<std::string, std::string>*)
{
  return nullptr;
}

std::string DummyDatabase::retrieveJson(std::string, long, const std::map<std::string, std::string>&)
{
  return std::string();
}

void* DummyDatabase::retrieveAny(const std::type_info&, const std::string&, const std::map<std::string, std::string>&, long, std::map<std::string, std::string>*, const std::string&, const std::string&)
{
  return nullptr;
}
std::shared_ptr<o2::quality_control::TimeRangeFlagCollection> DummyDatabase::retrieveTRFC(const std::string& name, const std::string& detector, int runNumber, const std::string& passName, const std::string& periodName, const std::string& provenance, long timestamp)
{
  return nullptr;
}

} // namespace o2::quality_control::repository
