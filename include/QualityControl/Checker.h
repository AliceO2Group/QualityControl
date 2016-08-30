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

    void createChannel(std::string type, std::string method, std::string address, std::string channelName);

  protected:
    virtual void Run() override;

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
    void check(AliceO2::QualityControl::Core::MonitorObject *mo);

    /**
     * \brief Store the MonitorObject in the database.
     *
     * @param mo The MonitorObject to be stored in the database.
     */
    void store(AliceO2::QualityControl::Core::MonitorObject *mo);

    /**
     * \brief Send the MonitorObject on FairMQ to whoever is listening.
     */
    void send(AliceO2::QualityControl::Core::MonitorObject *mo);

    void loadLibrary(const std::string libraryName) const;
    CheckInterface* instantiateCheck(std::string checkName, std::string className) const;
    static void CustomCleanupTMessage(void *data, void *object);
    void populateConfig(ConfigFile& configFile, std::string checkerName);

    AliceO2::QualityControl::Core::QcInfoLogger &mLogger;
    AliceO2::QualityControl::Repository::DatabaseInterface *mDatabase;
    std::vector<std::string> mTasksAlreadyEncountered;
    CheckerConfig mCheckerConfig;

    // monitoring
    std::shared_ptr<AliceO2::Monitoring::Core::Collector> mCollector;
    std::unique_ptr<Monitoring::Core::ProcessMonitor> mMonitor;

    // metrics
    ba::accumulator_set<double, ba::features<ba::tag::mean, ba::tag::variance>> pcpus;
    ba::accumulator_set<double, ba::features<ba::tag::mean, ba::tag::variance>> pmems;
    ba::accumulator_set<double, ba::features<ba::tag::mean, ba::tag::variance>> mAccProcessTime;
    int mTotalNumberHistosReceived = 0;

};

} /* namespace Checker */
} /* namespace QualityControl */
} /* namespace AliceO2 */

#endif /* QUALITYCONTROL_LIBS_CHECKER_CHECKER_H_ */
