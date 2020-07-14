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
#include "QualityControl/RepoPathUtils.h"
#include "QualityControl/QualityObject.h"

using namespace AliceO2::Common;

namespace o2::quality_control::core
{

QualityObject::QualityObject(
  Quality quality,
  std::string checkName,
  std::string detectorName,
  std::string policyName,
  std::vector<std::string> inputs,
  std::vector<std::string> monitorObjectsNames,
  std::map<std::string, std::string> metadata)
  : mQuality{ quality },
    mCheckName{ std::move(checkName) },
    mDetectorName{ std::move(detectorName) },
    mPolicyName{ std::move(policyName) },
    mInputs{ std::move(inputs) },
    mMonitorObjectsNames{ std::move(monitorObjectsNames) }
{
  mQuality.overwriteMetadata(metadata);
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
    mQuality.addMetadata(key, value);
  }

  void QualityObject::addMetadata(std::map<std::string, std::string> pairs)
  {
    mQuality.addMetadata(pairs);
  }

  const std::map<std::string, std::string>& QualityObject::getMetadataMap() const
  {
    return mQuality.getMetadataMap();
  }

  void QualityObject::updateMetadata(std::string key, std::string value)
  {
    mQuality.updateMetadata(key, value);
  }

  const std::string QualityObject::getMetadata(std::string key)
  {
    return mQuality.getMetadata(key);
  }

  std::string QualityObject::getPath() const
  {
    std::string path;
    try {
      path = RepoPathUtils::getQoPath(this);
      //      path = RepoPathUtils::getQoPath(getDetectorName(), getCheckName(), mPolicyName, mMonitorObjectsNames);
    } catch (FatalException& fe) {
      fe << errinfo_details("Only one MO should be assigned to one QO With the policy OnEachSeparatety"); // update error info
      throw;
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

  const std::string& QualityObject::getPolicyName() const
  {
    return mPolicyName;
  }

  const std::vector<std::string> QualityObject::getMonitorObjectsNames() const
  {
    return mMonitorObjectsNames;
  }

} // namespace o2::quality_control::core
