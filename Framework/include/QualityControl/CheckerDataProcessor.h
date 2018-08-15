///
/// \file   CheckerDataProcessor.h
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_CORE_CHECKERDATAPROCESSOR_H
#define QUALITYCONTROL_CORE_CHECKERDATAPROCESSOR_H

// std & boost
#include <chrono>
#include <memory>
// O2
#include <Common/Timer.h>
#include <Configuration/ConfigurationInterface.h>
#include <Framework/Task.h>
#include <Monitoring/MonitoringFactory.h>
// QC
#include "QualityControl/CheckInterface.h"
#include "QualityControl/CheckerConfig.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QcInfoLogger.h"

namespace o2 {
namespace quality_control {
namespace checker {

/// \brief The class in charge of running the checks on a MonitorObject.
///
/// A Checker is in charge of loading/instantiating the proper checks for a given MonitorObject, to configure them
/// and to run them on the MonitorObject in order to generate a quality.
///
/// TODO Evalue whether we should have a dedicated device to store in the database.
///
/// \author Barthélémy von Haller
class CheckerDataProcessor : public framework::Task {
public:
  /// Constructor
  CheckerDataProcessor(std::string checkerName, std::string taskName, std::string configurationSource);
  /// Destructor
  ~CheckerDataProcessor() override;

  /// \brief Checker init callback
  void init(framework::InitContext& ctx) override;
  /// \brief Checker process callback
  void run(framework::ProcessingContext& ctx) override;

  framework::InputSpec getInputSpec() { return mInputSpec; };
  framework::OutputSpec getOutputSpec() { return mOutputSpec; };

  static o2::header::DataDescription checkerDataDescription(const std::string taskName);

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
  void check(std::shared_ptr<MonitorObject> mo);

  /**
   * \brief Store the MonitorObject in the database.
   *
   * @param mo The MonitorObject to be stored in the database.
   */
  void store(std::shared_ptr<MonitorObject> mo);

  /**
   * \brief Send the MonitorObject on FairMQ to whoever is listening.
   */
  void send(std::shared_ptr<MonitorObject> mo, framework::DataAllocator& allocator);

  void loadLibrary(const std::string libraryName);
  CheckInterface* instantiateCheck(std::string checkName, std::string className);
  void populateConfig(std::unique_ptr<o2::configuration::ConfigurationInterface>& config);

  std::string mCheckerName;
  std::string mConfigurationSource;
  o2::framework::InputSpec mInputSpec;
  o2::framework::OutputSpec mOutputSpec;

  o2::quality_control::core::QcInfoLogger& mLogger;
  std::shared_ptr<o2::quality_control::repository::DatabaseInterface> mDatabase;

  std::vector<std::string> mLibrariesLoaded;
  std::map<std::string, CheckInterface*> mChecksLoaded;
  std::map<std::string, TClass*> mClassesLoaded;

  // monitoring
  std::shared_ptr<o2::monitoring::Monitoring> mCollector;
  std::chrono::system_clock::time_point startFirstObject;
  std::chrono::system_clock::time_point endLastObject;
  int mTotalNumberHistosReceived;
  AliceO2::Common::Timer timer;
};

} /* namespace Checker */
} /* namespace QualityControl */
} /* namespace o2 */

#endif // QUALITYCONTROL_CORE_CHECKERDATAPROCESSOR_H
