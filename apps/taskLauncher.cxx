///
/// \file   TaskLauncher.cxx
/// \author Barthelemy von Haller
///

#include "Core/Version.h"
#include "Core/Publisher.h"
#include <boost/program_options.hpp>
#include <iostream>
#include "Core/TaskControl.h"
#include <signal.h>
#include <execinfo.h>
#include <TApplication.h>
#include "Core/QcInfoLogger.h"

namespace po = boost::program_options;

using namespace std;
using namespace AliceO2::QualityControl::Core;

/// \brief Print all the entries of the stack on stderr
/// \author Barthelemy von Haller
void printStack()
{
  void *array[10];
  size_t size;
  size = backtrace(array, 10);
  backtrace_symbols_fd(array, size, 2);
}

/// \brief Callback for SigSev
/// \author Barthelemy von Haller
void handler_sigsev(int sig)
{
  fprintf(stderr, "Error: signal %d:\n", sig);
  cerr << "Error: signal " << sig << "\n";
  printStack();
  exit(1);
}

bool keepRunning = true; /// Indicates whether we should continue the execution loop or not.

/// \brief Callback for SigINT
/// Long description
/// \author Barthelemy von Haller
void handler_int(int sig)
{
  if (keepRunning) {
    cout << "Catched signal " << sig <<
    "\n  Exit the process at the end of this cycle. \n  Press again Ctrl-C to force immediate exit" << endl;
    keepRunning = false;
  } else {
    cout << "Second interruption : immediate exit" << endl;
    exit(0);
  }
}

int main(int argc, char *argv[])
{
  // This is needed for ROOT
  TApplication app("a", 0, 0);

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
  signal(SIGINT, handler_int); // for interruptions
  signal(SIGTERM, handler_int); // for termination requests

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

/*namespace AliceO2 {
namespace QualityControl {
namespace Core {

TaskLauncher::TaskLauncher()
{
}

TaskLauncher::~TaskLauncher()
{
}

void TaskLauncher::go()
{
  // Simulate the different state transitions
  taskControl.initialize();
  taskControl.configure();
  taskControl.start();
  while (1) {
    taskControl.execute();
  }
  taskControl.stop();
  // From here on we don't get control back until it is stopped or we receive a SIG.
}

} // namespace Publisher
} // namespace QualityControl
} // namespace AliceO2
*/