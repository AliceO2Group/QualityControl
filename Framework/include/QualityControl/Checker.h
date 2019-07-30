// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   Checker.h
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#ifndef QC_CHECKER_CHECKER_H
#define QC_CHECKER_CHECKER_H

// std & boost
#include <chrono>
#include <memory>
#include <string>
#include <map>
#include <vector>
// O2
#include <Common/Timer.h>
#include <Framework/Task.h>
#include <Headers/DataHeader.h>
// QC
#include "QualityControl/CheckInterface.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QcInfoLogger.h"

namespace o2::framework {
struct InputSpec;
struct OutputSpec;
class DataAllocator;
}

namespace o2::monitoring {
class Monitoring;
}

class TClass;

namespace o2::quality_control::checker
{

/// \brief The class in charge of running the checks on a MonitorObject.
///
/// A Checker is in charge of loading/instantiating the proper checks for a given MonitorObject, to configure them
/// and to run them on the MonitorObject in order to generate a quality. At the moment, a checker also stores the MO
/// and its quality in the repository.
///
/// TODO Evaluate whether we should have a dedicated device to store in the database.
///
/// \author Barthélémy von Haller
class Checker : public framework::Task
{
 public:
  /// Constructor
  Checker(std::string checkerName, std::string taskName, std::string configurationSource);

  /// Destructor
  ~Checker() override;

  /// \brief Checker init callback
  void init(framework::InitContext& ctx) override;

  /// \brief Checker process callback
  void run(framework::ProcessingContext& ctx) override;

  framework::InputSpec getInputSpec() { return mInputSpec; };

  framework::OutputSpec getOutputSpec() { return mOutputSpec; };

  /// \brief Unified DataDescription naming scheme for all checkers
  static o2::header::DataDescription createCheckerDataDescription(const std::string taskName);

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
  void send(std::unique_ptr<TObjArray>& mo, framework::DataAllocator& allocator);

  /**
   * \brief Load a library.
   * Load a library if it is not already in the cache.
   * \param libraryName The name of the library to load.
   */
  void loadLibrary(const std::string libraryName);

  /**
   * Get the check specified by its name and class.
   * If it has never been asked for before it is instantiated and cached. There can be several copies
   * of the same check but with different names in order to have them configured differently.
   * @todo Pass the name of the task that will use it. It will help with getting the correct configuration.
   * @param checkName
   * @param className
   * @return the check object
   */
  CheckInterface* getCheck(std::string checkName, std::string className);

  // General state
  std::string mCheckerName;
  std::string mConfigurationSource;
  o2::quality_control::core::QcInfoLogger& mLogger;
  std::shared_ptr<o2::quality_control::repository::DatabaseInterface> mDatabase;

  // DPL
  o2::framework::InputSpec mInputSpec;
  o2::framework::OutputSpec mOutputSpec;

  // Checks cache
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

} // namespace o2::quality_control::checker

#endif // QC_CHECKER_CHECKER_H
