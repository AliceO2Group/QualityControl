
#ifndef QC_CHECKER_CHECK_H
#define QC_CHECKER_CHECK_H

// std
#include <functional>
#include <map>
#include <vector>
#include <memory>
// O2
#include <Framework/DataProcessorSpec.h>
// QC
#include "QualityControl/Quality.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/CheckInterface.h"
#include "QualityControl/QcInfoLogger.h"

namespace o2::quality_control::checker
{

class Check {
  public:
    Check(std::string checkName, std::string configurationSource);

    /**
     * Expected to run in the init phase of the FairDevice
     */
    void init();
    
    o2::quality_control::core::Quality check(std::map<std::string, std::shared_ptr<o2::quality_control::core::MonitorObject>>& moMap);

    // Policy 
    /**
     * Change the revision.
     * Expected to be changed after invoke of `check(moMap)` or revision number overflow.
     */
    void updateRevision(unsigned int revision);
    
    /**
     * Return true if the Monitor Objects were changed accordingly to the policy
     */
    bool isReady(std::map<std::string, unsigned int>& revisionMap);

    o2::framework::OutputSpec getOutputSpec() { return mOutputSpec; }
    //TODO: Unique Input string
    static o2::header::DataDescription createCheckerDataDescription(const std::string taskName);
  private:
    void initConfig();
    void initPolicy();
    void loadLibrary();

    std::string mName;
    std::string mConfigurationSource;

    o2::quality_control::core::QcInfoLogger& mLogger;
    o2::quality_control::core::Quality mLastQuality;
    o2::framework::Inputs mInputs;

    // Depends - might be a (1) global checker output (more performant) or (2) per checkname output (more flexible)
    // (1) - what should be the output name? (also device name problem) - random is not deterministic in config
    o2::framework::OutputSpec mOutputSpec;
    std::vector<std::string> mMonitorObjectNames;
    bool mAllMOs = false;

    // Check module
    std::string mModuleName;
    std::string mClassName;
    CheckInterface* mCheckInterface = nullptr;

    // Policy
    std::string mPolicyType;
    std::function<bool(std::map<std::string, unsigned int>&)> mPolicy;
    unsigned int mMORevision = 0;
    bool mPolicyHelper = false; // Depending on policy, the purpose might change
};

}

#endif
