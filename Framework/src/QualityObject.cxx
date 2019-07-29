#include <Framework/DataSpecUtils.h>
#include "QualityControl/QualityObject.h"


ClassImp(o2::quality_control::core::QualityObject)

namespace o2::quality_control::core {

QualityObject::QualityObject(const std::string& checkerName, o2::framework::Inputs inputs)
  : mCheckName(checkerName),
    mInputs{},
    mUserMetadata{}
{
  setInputs(inputs);
  updateQuality(Quality());
}

QualityObject::QualityObject(const std::string& checkerName)
  :QualityObject(checkerName, {})
{}

QualityObject::~QualityObject(){}

const std::string anonChecker = "anonymouseChecker";
QualityObject::QualityObject()
  :QualityObject(anonChecker,{})
{}

const char* QualityObject::GetName() const {
  return mCheckName.c_str();
}

void QualityObject::updateQuality(Quality quality){
  //TODO: Update timestamp
  mQualityLevel = quality.getLevel();
  mQualityName = quality.getName();
}
Quality QualityObject::getQuality(){
  return Quality(mQualityLevel, mQualityName);
}

void QualityObject::setInputs(o2::framework::Inputs inputs){
  for (auto& input: inputs){
    mInputs.push_back(o2::framework::DataSpecUtils::describe(input));
  }
}

void QualityObject::addMetadata(std::string key, std::string value){
  mUserMetadata.insert(std::pair(key, value)); 
}

std::map<std::string, std::string> QualityObject::getMetadataMap(){
  return mUserMetadata;
}

}
