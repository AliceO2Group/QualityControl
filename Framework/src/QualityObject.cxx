#include "QualityControl/QualityObject.h"


ClassImp(o2::quality_control::core::QualityObject)

namespace o2::quality_control::core {

  QualityObject::QualityObject(const std::string& checkerName, o2::framework::Inputs inputs)
  : mCheckName(checkerName),
    mInputs{inputs},
    mUserMetadata{},
    mQuality(Quality::Null)
{
};

QualityObject::QualityObject(const std::string& checkerName)
  :QualityObject(checkerName, {})
{}

void QualityObject::updateQuality(Quality quality){
  //TODO: Update timestamp
  mQuality = quality;
}
Quality QualityObject::getQuality(){
  return mQuality;
}

void QualityObject::addMetadata(std::string key, std::string value){
  mUserMetadata.insert(std::pair(key, value)); 
}

std::map<std::string, std::string> QualityObject::getMetadataMap(){
  return mUserMetadata;
}

}
