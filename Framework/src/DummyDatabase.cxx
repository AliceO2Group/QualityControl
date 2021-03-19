
// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
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

std::string DummyDatabase::retrieveMOJson(std::string, std::string, long)
{
  return std::string();
}

void DummyDatabase::storeQO(std::shared_ptr<const o2::quality_control::core::QualityObject>, long, long)
{
}

std::shared_ptr<QualityObject> DummyDatabase::retrieveQO(std::string, long)
{
  return std::shared_ptr<QualityObject>();
}

std::string DummyDatabase::retrieveQOJson(std::string, long)
{
  return std::string();
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

void* DummyDatabase::retrieveAny(const std::type_info&, const std::string&, const std::map<std::string, std::string>&, long, std::map<std::string, std::string>*, const std::string&, const std::string&, const std::string&)
{
  return nullptr;
}

} // namespace o2::quality_control::repository
