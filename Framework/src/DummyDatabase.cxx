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

namespace o2::quality_control::repository
{

void DummyDatabase::connect(std::string, std::string, std::string, std::string)
{
}

void DummyDatabase::connect(const std::unordered_map<std::string, std::string>&)
{
}

void DummyDatabase::store(std::shared_ptr<o2::quality_control::core::MonitorObject>)
{
}

core::MonitorObject* DummyDatabase::retrieve(std::string, std::string, long)
{
  return nullptr;
}

std::string DummyDatabase::retrieveJson(std::string, std::string)
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

} // namespace o2::quality_control::repository
