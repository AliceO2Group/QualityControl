#include "QualityControl/MonitorObjectPolicy.h"

namespace o2::quality_control::monitor
{

MonitorObjectPolicy::MonitorObjectPolicy(std::string type, std::vector<std::string> moNames): 
    mMoNames{moNames}, 
    mRevisionList(moNames.size(), 0),
    mLastRevision(0), 
    mRevision(0),
    size(moNames.size())
{
  if (type == "all"){
    mPolicy = [=](){
      for (auto &rev : mRevisionList) {
        if (mLastRevision > rev) {
          return false;
        }
      }
      return true;
    };
  } else if (type == "anyNonZero") {
    mPolicy = [=](){
      for (auto &rev : mRevisionList) {
        if (rev <= 0) {
          return false;
        }
      }
      return mRevision > mLastRevision; //return true
    };
  } else /* any (default) */ {
    mPolicy = [=](){
      return mRevision > mLastRevision; //return true
    };
  }
}

void MonitorObjectPolicy::update(std::string moName){
  if (size == 1){

  } else {
    auto pos = find(mMoNames.begin(), mMoNames.end(), moName) - mMoNames.begin();
    ++mRevision;
    mRevisionList[pos] = mRevision;
    //TODO: Potencial bug if revision number > int(max)
  }
}

bool MonitorObjectPolicy::isReady() {
  bool ready = mPolicy(); 
  mLastRevision = mRevision;
  return ready;
}

}
