///
/// \file   qcTaskLauncher.cxx
/// \author Barthelemy von Haller
///

// std
#include <iostream>
#include <signal.h>
// boost
#include <boost/program_options.hpp>
#include <boost/exception/diagnostic_information.hpp>
// ROOT
#include <TApplication.h>
#include <TROOT.h>
// O2
#include "Common/signalUtilities.h"
#include "Common/Timer.h"
// QC
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/TaskControl.h"
#include "QualityControl/Version.h"

namespace po = boost::program_options;

using namespace std;
using namespace AliceO2::QualityControl::Core;

int main(int argc, char *argv[])
{
  // This is needed for ROOT
  TApplication app("a", nullptr, nullptr);
  gROOT->SetBatch(true);

  // Arguments parsing
  po::variables_map vm;
  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "Produce help message.")("version,v", "Show program name/version banner and exit.")(
    "rev", "Print the Git revision number.")("name,n", po::value<string>(), "Set the name of the task (required).")(
    "configuration,c", po::value<string>(), "Configuration source, e.g. \"file:example.ini\" (required).")("cycles,C",
                                                                                                           po::value<int>(),
                                                                                                           "Number of cycles to run.")
//    ("source,s", po::value<string>(), "Set the source for the data blocks (required).")
//    ("module,m", po::value<string>(), "Module containing the task to load (required).")
//    ("class,c", po::value<string>(), "Module's task class to instantiate (required).")
    ;
  po::store(parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return EXIT_SUCCESS;
  }
  if (vm.count("version")) {
    std::cout << "QualityControl version " << AliceO2::QualityControl::Core::Version::getString() << std::endl;
    return EXIT_SUCCESS;
  }
  if (vm.count("rev")) {
    std::cout << "SVN revision : " << AliceO2::QualityControl::Core::Version::getRevision() << std::endl;
    return EXIT_SUCCESS;
  }
  string taskName = "";
  if (vm.count("name")) {
    taskName = vm["name"].as<string>();
  } else {
    // we don't use the "required" option of the po::value because we want to output the help and avoid
    // horrible template error to be displayed to the user.
    std::cout << "\"name\" is required!" << "\n";
    std::cout << desc << std::endl;
    return EXIT_FAILURE;
  }
  string configurationSource = "";
  if (vm.count("configuration")) {
    configurationSource = vm["configuration"].as<string>();
  } else {
    // we don't use the "required" option of the po::value because we want to output the help and avoid
    // horrible template error to be displayed to the user.
    std::cout << "\"configuration\" is required!" << "\n";
    std::cout << desc << std::endl;
    return EXIT_FAILURE;
  }
  int maxNumberCycles = INT_MAX;
  if (vm.count("cycles")) {
    maxNumberCycles = vm["cycles"].as<int>();
    std::cout << "Max number cycles : " << maxNumberCycles << std::endl;
  }

  // install handlers
  signal(SIGSEGV, handler_sigsev); // for seg faults
  signal(SIGINT, handler_interruption); // for interruptions
  signal(SIGTERM, handler_interruption); // for termination requests

  TaskControl *taskControl = new TaskControl(taskName, configurationSource);
  try {
    // Actual "work" starts here
    taskControl->initialize();
    taskControl->configure();
    taskControl->start();

    std::shared_ptr<AliceO2::Monitoring::Collector> collector(new AliceO2::Monitoring::Collector(configurationSource));

    int cycle = 0;
    AliceO2::Common::Timer timer;
    timer.reset(10000000); // 10 s.
    int lastNumberObjects = 0;

    while (keepRunning && cycle < maxNumberCycles) {
      QcInfoLogger::GetInstance() << "cycle " << cycle << AliceO2::InfoLogger::InfoLogger::endm;
      taskControl->execute();
      cycle++;

      // if 10 s we publish stats
      if(timer.isTimeout()) {
        double current = timer.getTime();
        int objectsPublished = (taskControl->getTotalNumberObjectsPublished()-lastNumberObjects);
        lastNumberObjects = taskControl->getTotalNumberObjectsPublished();
        collector->send(objectsPublished/current, "QC_task_Rate_objects_published_per_10_seconds");
        timer.increment();
      }
    }

    taskControl->stop();

  } catch (std::string const& e) {
    cerr << e << endl;
  } catch (...) {
    string diagnostic = boost::current_exception_diagnostic_information();
    std::cerr << "Unexpected exception, diagnostic information follows:\n" << diagnostic << endl;
    if (diagnostic == "No diagnostic information available.") {
      throw;
    }
  }
  delete taskControl;

  return EXIT_SUCCESS;
}
