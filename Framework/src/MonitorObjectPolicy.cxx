#include "QualityControl/MonitorObjectPolicy.h"
#include "QualityControl/QcInfoLogger.h"

namespace o2::quality_control::monitor
{

using namespace o2::quality_control::core;

MonitorObjectPolicy::MonitorObjectPolicy(std::string type, std::vector<std::string> moNames): 
    mRevisionMap(),
    mLastRevision(0), 
    mRevision(0),
    mSize(moNames.size())
{
  QcInfoLogger::GetInstance() << "Policy type: " << type << AliceO2::InfoLogger::InfoLogger::endm;
  if (type == "all" && moNames.size() > 1){
    QcInfoLogger::GetInstance() << "Policy type initiate: ALL" << AliceO2::InfoLogger::InfoLogger::endm;
    mPolicy = [=](){
      if (mSize != mRevisionMap.size()){
        return false;
      }
      for (auto &rev : mRevisionMap) {
        if (mLastRevision > rev.second) {
          return false;
        }
      }
      return true;
    };
  } else if (type == "anyNonZero" && moNames.size() > 1) {
    QcInfoLogger::GetInstance() << "Policy type initiate: ANYNONZERO" << AliceO2::InfoLogger::InfoLogger::endm;
    mPolicy = [=](){
      for (auto &rev : mRevisionMap) {
        if (rev.second <= 0) {
          return false;
        }
      }
      return mRevision > mLastRevision; //return true
    };
  } else /* any (default) */ {
    QcInfoLogger::GetInstance() << "Policy type initiate: ANY (default)" << AliceO2::InfoLogger::InfoLogger::endm;
    mPolicy = [=](){
      return (mSize == 1 || mSize == mRevisionMap.size()) && mRevision > mLastRevision; //return true
    };
  }
}

void MonitorObjectPolicy::update(std::string moName){
  if (mSize > 1){
    if (mRevisionMap.count(moName)){
      ++mRevision;
      mRevisionMap[moName] = mRevision;
      //TODO: Potencial bug if revision number > int(max)
    } else {
      ++mRevision;
      mRevisionMap[moName] = mRevision;
    }
  } else {
    ++mRevision;
  }
}

bool MonitorObjectPolicy::isReady() {
  bool ready = mPolicy(); 
  mLastRevision = mRevision;
  return ready;
}

}
