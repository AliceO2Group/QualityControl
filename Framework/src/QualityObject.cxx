// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#include <utility>
#include <Common/Exceptions.h>

#include "QualityControl/QualityObject.h"

using namespace AliceO2::Common;

ClassImp(o2::quality_control::core::QualityObject)

  namespace o2::quality_control::core
{

  QualityObject::QualityObject(
    Quality quality,
    std::string checkName,
    std::string detectorName,
    std::string policy,
    std::vector<std::string> inputs,
    std::vector<std::string> monitorObjects,
    std::map<std::string, std::string> metadata) //
    : mQuality{ quality },
      mCheckName{ std::move(checkName) },
      mDetectorName{ std::move(detectorName) },
      mPolicy{ std::move(policy) },
      mInputs{ std::move(inputs) },
      mMonitorObjects{ std::move(monitorObjects) },
      mUserMetadata{ std::move(metadata) }
  {
  }

  QualityObject::~QualityObject() = default;

  const std::string anonChecker = "anonymousChecker";
  QualityObject::QualityObject()
    : QualityObject(Quality(), anonChecker)
  {
  }

  const char* QualityObject::GetName() const
  {
    return mCheckName.c_str();
  }

  void QualityObject::updateQuality(Quality quality)
  {
    //TODO: Update timestamp
    mQuality = quality;
  }
  Quality QualityObject::getQuality() const
  {
    return mQuality;
  }

  void QualityObject::addMetadata(std::string key, std::string value)
  {
    mUserMetadata.insert(std::pair(key, value));
  }

  void QualityObject::addMetadata(std::map<std::string, std::string> pairs)
  {
    // we do not use "merge" because it would ignore the items whose key already exist in mUserMetadata.
    mUserMetadata.insert(pairs.begin(), pairs.end());
  }

  const std::map<std::string, std::string>& QualityObject::getMetadataMap() const
  {
    return mUserMetadata;
  }

  void QualityObject::updateMetadata(std::string key, std::string value)
  {
    if (mUserMetadata.count(key) > 0) {
      mUserMetadata[key] = value;
    }
  }

  std::string QualityObject::getPath() const
  {
    std::string path = "qc/checks/" + getDetectorName() + "/" + getName();
    if (mPolicy == "OnEachSeparately") {
      if (mMonitorObjects.size() == 1) {
        path += "/" + mMonitorObjects[0];
      } else {
        BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Only one MO should be assigned to one QO With the policy OnEachSeparatety"));
      }
    }
    return path;
  }

  const std::string& QualityObject::getDetectorName() const
  {
    return mDetectorName;
  }

  void QualityObject::setDetectorName(const std::string& detectorName)
  {
    QualityObject::mDetectorName = detectorName;
  }

  void QualityObject::setQuality(const Quality& quality)
  {
    updateQuality(quality);
  }
  const std::string& QualityObject::getCheckName() const
  {
    return mCheckName;
  }
} // namespace o2::quality_control::core
