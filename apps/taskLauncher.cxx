///
/// \file   taskLauncher.cxx
/// \author Barthelemy von Haller
///

// std
#include <iostream>
#include <signal.h>
// boost
#include <boost/program_options.hpp>
// ROOT
#include <TApplication.h>
#include <TROOT.h>
// QC
#include "Core/QcInfoLogger.h"
#include "signalUtilities.h"
#include "Core/TaskControl.h"
#include "Core/Version.h"

namespace po = boost::program_options;

using namespace std;
using namespace AliceO2::QualityControl::Core;

int main(int argc, char *argv[])
{
  // This is needed for ROOT
  TApplication app("a", 0, 0);
  gROOT->SetBatch(true);

  // Arguments parsing
  po::variables_map vm;
  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "Produce help message.")("version,v", "Show program name/version banner and exit.")(
    "rev", "Print the SVN revision number");
  po::store(parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  // help
  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return EXIT_SUCCESS;
  }
  // version
  if (vm.count("version")) {
    std::cout << "QualityControl version " << AliceO2::QualityControl::Core::Version::getString() << std::endl;
    return EXIT_SUCCESS;
  }
  // rev
  if (vm.count("rev")) {
    std::cout << "SVN revision : " << AliceO2::QualityControl::Core::Version::getRevision() << std::endl;
    return EXIT_SUCCESS;
  }

  // install handlers
  signal(SIGSEGV, handler_sigsev); // for seg faults
  signal(SIGINT, handler_interruption); // for interruptions
  signal(SIGTERM, handler_interruption); // for termination requests

  // Actual "work" here
  TaskControl taskControl("ExampleTask"); // TODO get the name of class and module from arguments
  taskControl.initialize();
  taskControl.configure();
  taskControl.start();
  while (keepRunning) {
    taskControl.execute();
  }
  taskControl.stop();

  return EXIT_SUCCESS;
}
