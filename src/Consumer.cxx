///
/// \file   Consumer.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/Consumer.h"
// std
#include <iostream>
#include <csignal>
// O2
#include "Common/signalUtilities.h"
#include "QualityControl/Version.h"
// boost
#include <boost/program_options.hpp>
#include <boost/exception/diagnostic_information.hpp>

using namespace std;
using namespace AliceO2::QualityControl;
namespace po = boost::program_options;

namespace AliceO2 {
namespace QualityControl {
namespace Client {

Consumer::Consumer()
    : mNumberCycles(0), mNumberObjects(0), mNumberTasks(0)
{
}

Consumer::~Consumer()
{
}

void Consumer::consume()
{
  // Get list of all objects being published
  vector<string> listTasks = mDataProvider.getListOfActiveTasks();
  mNumberCycles++;
  // Get all objects, inefficiently (1 by 1) and trash them
  for (auto const &task : listTasks) {
    mNumberTasks++;
    vector<string> publicationList = mDataProvider.getPublicationList(task);
    for (auto const &object : publicationList) {
      mNumberObjects++;
      TObject *obj = mDataProvider.getObject(task, object);
      delete obj;
    }
  }
}

void Consumer::print()
{
  cout << "\r" << "cycles: " << mNumberCycles;
  cout << " ; objects: " << mNumberObjects;
  cout.flush();
}

} // namespace Client
} // namespace QualityControl
} // namespace AliceO2

int main(int argc, char *argv[])
{
  // Arguments parsing
  po::variables_map vm;
  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "Produce help message.")("version,v", "Show program name/version banner and exit.")(
      "rev", "Print the Git revision number.")("configuration,c", po::value<string>(),
      "Configuration source, e.g. \"file:example.ini\" (required).")("cycle_duration,t", po::value<int>(),
      "Duration of cycles in seconds. Use 0 to keep looping (default).");
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
  string configurationSource = "";
  if (vm.count("configuration")) {
    configurationSource = vm["configuration"].as<string>();
  } /*else {
   // we don't use the "required" option of the po::value because we want to output the help and avoid
   // horrible template error to be displayed to the user.
   std::cout << "\"configuration\" is required!" << "\n";
   std::cout << desc << std::endl;
   return EXIT_FAILURE;
   }*/
  int maxNumberCycles = INT_MAX;
  int cycleDuration = 0;
  if (vm.count("cycle_duration")) {
    maxNumberCycles = vm["cycle_duration"].as<int>();
  }

  // install handlers
  signal(SIGSEGV, handler_sigsev); // for seg faults
  signal(SIGINT, handler_interruption); // for interruptions
  signal(SIGTERM, handler_interruption); // for termination requests

  try {

    AliceO2::QualityControl::Client::Consumer consumer;
    int cycle = 0;
    while (keepRunning) {
      sleep(cycleDuration);   // duration of the monitor cycle
      consumer.consume();
      if(cycle%20 == 0) {
        consumer.print();
      }
      cycle++;
    }
    consumer.print();

  } catch (std::string const& e) {
    cerr << e << endl;
  } catch (std::exception & exc) {
    cerr << exc.what() << endl;
  } catch (boost::exception & exc) {
    string diagnostic = boost::current_exception_diagnostic_information();
    std::cerr << "Unexpected exception, diagnostic information follows:\n" << diagnostic << endl;
    if (diagnostic == "No diagnostic information available.") {
      throw;
    }
  }

  return EXIT_SUCCESS;
}
