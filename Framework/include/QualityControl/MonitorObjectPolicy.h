#ifndef QC_CORE_MONITOROBJECTPOLICY_H
#define QC_CORE_MONITOROBJECTPOLICY_H

#include <vector>
#include <algorithm>
#include <string>
#include <functional>

namespace o2::quality_control::monitor
{

class MonitorObjectPolicy {
  public:
    MonitorObjectPolicy(std::string type, std::vector<std::string> moNames);
    void update(std::string moName); 
    bool isReady();
  private:
    int size;
    int mLastRevision;
    int mRevision;
    std::vector<int> mRevisionList;
    std::vector<std::string> mMoNames;
    std::function<bool()> mPolicy;
};
}

#endif
