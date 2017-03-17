///
/// \file   Checker.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_CHECKER_CHECKER_H_
#define QUALITYCONTROL_LIBS_CHECKER_CHECKER_H_

#include "QualityControl/MonitorObject.h"
#include "Configuration/Configuration.h"
#include <FairMQDevice.h>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <memory>
#include <Configuration/ConfigurationInterface.h>
#include <Common/Timer.h>
#include <chrono>

#include "Common/Timer.h"

// QC
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/CheckInterface.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/CheckerConfig.h"
#include "Monitoring/Collector.h"
#include "Monitoring/ProcessMonitor.h"

namespace ba = boost::accumulators;

namespace AliceO2 {
namespace QualityControl {
namespace Checker {

/// \brief The class in charge of running the checks on a MonitorObject.
///
/// A Checker is in charge of loading/instantiating the proper checks for a given MonitorObject, to configure them
/// and to run them on the MonitorObject in order to generate a quality.
///
/// TODO Evalue whether we should have a dedicated device to store in the database.
///
/// \author Barthélémy von Haller
class Checker : public FairMQDevice
{
  public:
    /// Default constructor
    Checker(std::string checkerName, std::string configurationSource);
    /// Destructor
    virtual ~Checker();
    /**
     * \brief Create a new channel.
     * Create a new channel.
     * @param type
     * @param method
     * @param address
     * @param channelName
     * @param createCallback
     */
    void createChannel(std::string type, std::string method, std::string address, std::string channelName, bool createCallback=false);

  protected:
    bool HandleData(FairMQMessagePtr&, int);

  private:

    /**
     * \brief Evaluate the quality of a MonitorObject.
     *
     * The Check's associated with this MonitorObject are run and a global quality is built by
     * taking the worse quality encountered. The MonitorObject is modified by setting its quality
     * and by calling the "beautifying" methods of the Check's.
     *
     * @param mo The MonitorObject to evaluate and whose quality will be set according
     *        to the worse quality encountered while running the Check's.
     */
    void check(MonitorObject*  mo);

    /**
     * \brief Store the MonitorObject in the database.
     *
     * @param mo The MonitorObject to be stored in the database.
     */
    void store(MonitorObject*  mo);

    /**
     * \brief Send the MonitorObject on FairMQ to whoever is listening.
     */
    void send(MonitorObject*  mo);

    void loadLibrary(const std::string libraryName);
    CheckInterface* instantiateCheck(std::string checkName, std::string className);
    static void CustomCleanupTMessage(void *data, void *object);
    void populateConfig( std::unique_ptr<AliceO2::Configuration::ConfigurationInterface>& config, std::string checkerName);

    AliceO2::QualityControl::Core::QcInfoLogger &mLogger;
    AliceO2::QualityControl::Repository::DatabaseInterface *mDatabase;
    std::vector<std::string> mTasksAlreadyEncountered;
    CheckerConfig mCheckerConfig;

    std::vector<std::string> mLibrariesLoaded;
    std::map<std::string,CheckInterface*> mChecksLoaded;
    std::map<std::string,TClass*> mClassesLoaded;

    // monitoring
    std::shared_ptr<AliceO2::Monitoring::Collector> mCollector;
    std::chrono::system_clock::time_point startFirstObject;
    std::chrono::system_clock::time_point endLastObject;
    int mTotalNumberHistosReceived = 0;
    AliceO2::Common::Timer timer;
};

} /* namespace Checker */
} /* namespace QualityControl */
} /* namespace AliceO2 */

#endif /* QUALITYCONTROL_LIBS_CHECKER_CHECKER_H_ */
