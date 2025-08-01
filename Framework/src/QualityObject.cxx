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

#include <utility>
#include <Common/Exceptions.h>
#include <string>
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
  std::map<std::string, std::string> metadata,
  int runNumber)
  : mQuality{ std::move(quality) },
    mCheckName{ std::move(checkName) },
    mDetectorName{ std::move(detectorName) },
    mPolicyName{ std::move(policyName) },
    mInputs{ std::move(inputs) },
    mMonitorObjectsNames{ std::move(monitorObjectsNames) },
    mActivity(runNumber, "NONE", "", "", "qc", gInvalidValidityInterval)
{
  mQuality.addMetadata(std::move(metadata));
}

QualityObject::~QualityObject() = default;

constexpr auto anonChecker = "anonymousChecker";
QualityObject::QualityObject()
  : QualityObject(Quality(), anonChecker)
{
  mActivity.mProvenance = "qc";
}

const char* QualityObject::GetName() const
{
  std::string name = getName();
  return strdup(name.c_str());
}

std::string QualityObject::getName() const
{
  if (mPolicyName == "OnEachSeparately") {
    if (mMonitorObjectsNames.size() != 1) {
      BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("QualityObject::getName: "
                                                                "The vector of monitorObjectsNames must contain a single object"));
    }
    return mCheckName + "/" + mMonitorObjectsNames[0];
  }
  return mCheckName;
}

void QualityObject::updateQuality(Quality quality)
{
  // TODO: Update timestamp
  mQuality = std::move(quality);
}
Quality QualityObject::getQuality() const
{
  return mQuality;
}

void QualityObject::addMetadata(std::string key, std::string value)
{
  mQuality.addMetadata(std::move(key), std::move(value));
}

void QualityObject::addMetadata(std::map<std::string, std::string> pairs)
{
  mQuality.addMetadata(std::move(pairs));
}

const std::map<std::string, std::string>& QualityObject::getMetadataMap() const
{
  return mQuality.getMetadataMap();
}

void QualityObject::updateMetadata(std::string key, std::string value)
{
  mQuality.updateMetadata(std::move(key), std::move(value));
}

std::string QualityObject::getMetadata(std::string key) const
{
  return mQuality.getMetadata(std::move(key));
}

std::string QualityObject::getMetadata(std::string key, std::string defaultValue) const
{
  return mQuality.getMetadata(std::move(key), std::move(defaultValue));
}

std::optional<std::string> QualityObject::getMetadataOpt(const std::string& key) const
{
  return mQuality.getMetadataOpt(key);
}

std::string QualityObject::getPath() const
{
  std::string path;
  try {
    path = RepoPathUtils::getQoPath(this);
  } catch (FatalException& fe) {
    fe << errinfo_details("Only one MO should be assigned to one QO With the policy OnEachSeparately"); // update error info
    throw;
  }
  return path;
}

QualityObject& QualityObject::addFlag(const FlagType& flag, std::string comment)
{
  mQuality.addFlag(flag, std::move(comment));
  return *this;
}

const CommentedFlagTypes& QualityObject::getFlags() const
{
  return mQuality.getFlags();
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

const std::vector<std::string>& QualityObject::getMonitorObjectsNames() const
{
  return mMonitorObjectsNames;
}

std::ostream& operator<<(std::ostream& out, const QualityObject& q) // output
{
  out << "QualityObject: " << q.getName() << ":\n"
      << "   - checkName : " << q.getCheckName() << "\n"
      << "   - detectorName : " << q.getDetectorName() << "\n"
      //      << "   - runNumber : " << q.getRunNumber() << "\n"
      << "   - quality : " << q.getQuality() << "\n"
      << "   - monitorObjectsNames : ";
  for (auto item : q.getMonitorObjectsNames()) {
    out << item << ", ";
  }
  return out;
}

void QualityObject::updateActivity(int runNumber, const std::string& periodName, const std::string& passName, const std::string& provenance)
{
  mActivity.mId = runNumber;
  mActivity.mPeriodName = periodName;
  mActivity.mPassName = passName;
  mActivity.mProvenance = provenance;
}

const Activity& QualityObject::getActivity() const
{
  return mActivity;
}

Activity& QualityObject::getActivity()
{
  return mActivity;
}

void QualityObject::setActivity(const Activity& activity)
{
  mActivity = activity;
}

void QualityObject::setValidity(ValidityInterval validityInterval)
{
  mActivity.mValidity = validityInterval;
}

void QualityObject::updateValidity(validity_time_t value)
{
  mActivity.mValidity.update(value);
}

ValidityInterval QualityObject::getValidity() const
{
  return mActivity.mValidity;
}

} // namespace o2::quality_control::core
