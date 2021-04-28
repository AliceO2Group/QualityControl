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
/// \file   Quality.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/Quality.h"
#include <ostream>
#include <iostream>
#include <Common/Exceptions.h>

namespace o2::quality_control::core
{

// could be changed if needed but I don't see why we would need more than 10 levels
const unsigned int Quality::NullLevel = 10;

const Quality Quality::Good(1, "Good");
const Quality Quality::Medium(2, "Medium");
const Quality Quality::Bad(3, "Bad");
const Quality Quality::Null(NullLevel, "Null"); // we consider it the worst of the worst

Quality::Quality(unsigned int level, std::string name) : mLevel(level), mName(name), mUserMetadata{} {}

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

void Quality::addMetadata(std::string key, std::string value)
{
  mUserMetadata.insert(std::pair(key, value));
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

void Quality::updateMetadata(std::string key, std::string value)
{
  if (mUserMetadata.count(key) > 0) {
    mUserMetadata[key] = value;
  }
}

void Quality::overwriteMetadata(std::map<std::string, std::string> pairs)
{
  mUserMetadata.clear();
  addMetadata(pairs);
}

std::string Quality::getMetadata(std::string key) const
{
  if (mUserMetadata.count(key) == 0) {
    std::cerr << "Could not get the metadata with key \"" << key << "\"" << std::endl;
    BOOST_THROW_EXCEPTION(AliceO2::Common::ObjectNotFoundError() << AliceO2::Common::errinfo_object_name(key));
  }
  return mUserMetadata.at(key);
}

std::string Quality::getMetadata(std::string key, std::string defaultValue) const
{
  return mUserMetadata.count(key) > 0 ? mUserMetadata.at(key) : defaultValue;
}

Quality& Quality::addReason(FlagReason reason, std::string comment)
{
  mReasons.emplace_back(std::move(reason), std::move(comment));
  return *this;
}
const CommentedFlagReasons& Quality::getReasons() const
{
  return mReasons;
}
} // namespace o2::quality_control::core
