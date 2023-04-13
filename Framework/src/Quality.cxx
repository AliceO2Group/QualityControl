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
/// \file   Quality.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/Quality.h"
#include <ostream>
#include <iostream>
#include <utility>
#include <Common/Exceptions.h>
#include <boost/algorithm/string.hpp>

namespace o2::quality_control::core
{

// could be changed if needed, but I don't see why we would need more than 10 levels
const unsigned int Quality::NullLevel = 10;

const Quality Quality::Good(1, "Good");
const Quality Quality::Medium(2, "Medium");
const Quality Quality::Bad(3, "Bad");
const Quality Quality::Null(NullLevel, "Null"); // we consider it the worst of the worst

Quality::Quality(unsigned int level, std::string name) : mLevel(level), mName(std::move(name)), mUserMetadata{} {}

void Quality::set(const Quality& q)
{
  mLevel = q.mLevel;
  mName = q.mName;
}

unsigned int Quality::getLevel() const { return mLevel; }

const std::string& Quality::getName() const { return mName; }

std::ostream& operator<<(std::ostream& out, const Quality& q) // output
{
  out << "Quality: " << q.getName() << " (level " << q.getLevel() << ")";
  return out;
}

void Quality::addMetadata(const std::string& key, const std::string& value)
{
  mUserMetadata.emplace(key, value);
}

void Quality::addMetadata(std::map<std::string, std::string> pairs)
{
  // we do not use "merge" because it would ignore the items whose key already exist in mUserMetadata.
  mUserMetadata.insert(pairs.begin(), pairs.end());
}

const std::map<std::string, std::string>& Quality::getMetadataMap() const
{
  return mUserMetadata;
}

void Quality::updateMetadata(const std::string& key, std::string value)
{
  if (mUserMetadata.count(key) > 0) {
    mUserMetadata[key] = std::move(value);
  }
}

void Quality::overwriteMetadata(std::map<std::string, std::string> pairs)
{
  mUserMetadata.clear();
  addMetadata(std::move(pairs));
}

std::string Quality::getMetadata(const std::string& key) const
{
  if (mUserMetadata.count(key) == 0) {
    std::cerr << "Could not get the metadata with key \"" << key << "\"" << std::endl;
    BOOST_THROW_EXCEPTION(AliceO2::Common::ObjectNotFoundError() << AliceO2::Common::errinfo_object_name(key));
  }
  return mUserMetadata.at(key);
}

std::string Quality::getMetadata(const std::string& key, const std::string& defaultValue) const
{
  return mUserMetadata.count(key) > 0 ? mUserMetadata.at(key) : defaultValue;
}

Quality& Quality::addReason(const FlagReason& reason, std::string comment)
{
  mReasons.emplace_back(std::move(reason), std::move(comment));
  return *this;
}
const CommentedFlagReasons& Quality::getReasons() const
{
  return mReasons;
}

Quality Quality::fromString(const std::string& str)
{
  if (str == Quality::Good.getName()) {
    return Quality::Good;
  } else if (str == Quality::Medium.getName()) {
    return Quality::Medium;
  } else if (str == Quality::Bad.getName()) {
    return Quality::Bad;
  } else {
    return Quality::Null;
  }
}

} // namespace o2::quality_control::core
