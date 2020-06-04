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

ClassImp(o2::quality_control::core::Quality)

  // clang-format off
namespace o2::quality_control::core
{
  // clang-format on

  // could be changed if needed but I don't see why we would need more than 10 levels
  const unsigned int Quality::NullLevel = 10;

  const Quality Quality::Good(1, "Good");
  const Quality Quality::Medium(2, "Medium");
  const Quality Quality::Bad(3, "Bad");
  const Quality Quality::Null(NullLevel, "Null"); // we consider it the worst of the worst

  Quality::Quality(unsigned int level, std::string name) : mLevel(level), mName(name), mUserMetadata{} {}
  Quality::Quality(const Quality& q) : mLevel(q.mLevel), mName(q.mName), mUserMetadata{} {}

  unsigned int Quality::getLevel() const { return mLevel; }

  const std::string& Quality::getName() const { return mName; }

  std::ostream& operator<<(std::ostream& out, const Quality& q) // output
  {
    out << "Quality: " << q.getName() << " (level " << q.getLevel() << ")\n";
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

} // namespace o2::quality_control::core
