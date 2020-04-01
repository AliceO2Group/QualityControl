#include "QualityControl/QualityObject.h"

ClassImp(o2::quality_control::core::QualityObject)

  namespace o2::quality_control::core
{

  QualityObject::QualityObject(const std::string& checkName, std::vector<std::string> inputs, const std::string& detectorName)
    : mDetectorName(detectorName),
      mCheckName(checkName),
      mInputs{},
      mUserMetadata{}
  {
    setInputs(inputs);
    updateQuality(Quality());
  }

  QualityObject::~QualityObject() = default;

  const std::string anonChecker = "anonymouseChecker";
  QualityObject::QualityObject()
    : QualityObject(anonChecker, {})
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
    if(mUserMetadata.count(key) > 0) {
      mUserMetadata[key] = value;
    }
  }

  std::string QualityObject::getPath() const
  {
    std::string path = "qc/checks/" + getDetectorName() + "/" + getName();
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
}
