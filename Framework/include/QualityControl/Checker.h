///
/// \file   Checker.h
/// \author Barthelemy von Haller
///

#ifndef QC_CHECKER_CHECKER_H
#define QC_CHECKER_CHECKER_H

// std & boost
#include <chrono>
#include <memory>
#include <boost/serialization/array_wrapper.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
// FairRoot
#include <FairMQDevice.h>
// O2
#include <Configuration/ConfigurationInterface.h>
#include <Common/Timer.h>
#include <Monitoring/MonitoringFactory.h>
// QC
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/CheckInterface.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/CheckerConfig.h"
#include "QualityControl/MonitorObject.h"

namespace ba = boost::accumulators;

namespace o2 {
namespace quality_control {
namespace checker {

/// \brief The class in charge of running the checks on a MonitorObject.
///
/// A Checker is in charge of loading/instantiating the proper checks for a given MonitorObject, to configure them
/// and to run them on the MonitorObject in order to generate a quality.
///
/// TODO Evaluate whether we should have a dedicated device to store in the database.
///
/// \author Barthélémy von Haller
class Checker : public FairMQDevice
{
  public:
    /// Default constructor
    Checker(std::string checkerName, std::string configurationSource);
    /// Destructor
    ~Checker() override;
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
    void check(std::shared_ptr<MonitorObject>  mo);

    /**
     * \brief Store the MonitorObject in the database.
     *
     * @param mo The MonitorObject to be stored in the database.
     */
    void store(std::shared_ptr<MonitorObject>  mo);

    /**
     * \brief Send the MonitorObject on FairMQ to whoever is listening.
     */
    void send(std::shared_ptr<MonitorObject>  mo);

    void loadLibrary(const std::string libraryName);
    CheckInterface* instantiateCheck(std::string checkName, std::string className);
    static void CustomCleanupTMessage(void *data, void *object);
    void populateConfig( std::unique_ptr<o2::configuration::ConfigurationInterface>& config, std::string checkerName);

    o2::quality_control::core::QcInfoLogger &mLogger;
    std::unique_ptr<o2::quality_control::repository::DatabaseInterface> mDatabase;
    std::vector<std::string> mTasksAlreadyEncountered;
    CheckerConfig mCheckerConfig;

    std::vector<std::string> mLibrariesLoaded;
    std::map<std::string,CheckInterface*> mChecksLoaded;
    std::map<std::string,TClass*> mClassesLoaded;

    // monitoring
    std::shared_ptr<o2::monitoring::Monitoring> mCollector;
    std::chrono::system_clock::time_point startFirstObject;
    std::chrono::system_clock::time_point endLastObject;
    int mTotalNumberHistosReceived = 0;
    AliceO2::Common::Timer timer;
};

} /* namespace checker */
} /* namespace quality_control */
} /* namespace o2 */

#endif /* QC_CHECKER_CHECKER_H */
