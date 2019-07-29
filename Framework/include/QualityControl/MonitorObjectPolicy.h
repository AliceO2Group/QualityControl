#ifndef QC_CORE_MONITOROBJECTPOLICY_H
#define QC_CORE_MONITOROBJECTPOLICY_H

#include <vector>
#include <algorithm>
#include <string>
#include <functional>
#include <map>

namespace o2::quality_control::monitor
{

class MonitorObjectPolicy {
  public:
    MonitorObjectPolicy(std::string type, std::vector<std::string> moNames);
    void update(std::string moName); 
    bool isReady();
  private:
    unsigned int mSize;
    unsigned int mLastRevision;
    unsigned int mRevision;
    std::map<std::string, unsigned int> mRevisionMap;
    std::function<bool()> mPolicy;
};
}

#endif
