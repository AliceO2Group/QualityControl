///
/// \file   qcCheckerLauncher.cxx
/// \author Barthelemy von Haller
///

// std
#include <iostream>
// boost
#include <boost/exception/diagnostic_information.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string/classification.hpp>
// ROOT
#include <TApplication.h>
#include <TROOT.h>
// FairRoot
#include <FairMQTransportFactoryZMQ.h>
#include <Configuration/ConfigurationInterface.h>
#include <Configuration/ConfigurationFactory.h>
// O2
#include "Common/signalUtilities.h"
#include "QualityControl/Checker.h"
#include "QualityControl/Version.h"

using namespace std;
using namespace AliceO2::QualityControl::Core;
using namespace AliceO2::Configuration;
using namespace AliceO2::QualityControl::Checker;
namespace po = boost::program_options;

int main(int argc, char *argv[])
{
  string checkerName, configurationSource;

  // This is needed for ROOT
  TApplication app("a", nullptr, nullptr);
  gROOT->SetBatch(true);

  // Argument parsing
  po::variables_map vm;
  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "Produce help message.")("version,v", "Show program name/version banner and exit.")(
    "rev", "Print the Git revision number.")("name,n", po::value<string>(), "Set the name of the check (required).")(
    "configuration,c", po::value<string>(), "Configuration source, e.g. \"file:example.ini\" (required).");
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
  if (vm.count("name")) {
    checkerName = vm["name"].as<string>();
  } else {
    // we don't use the "required" option of the po::value because we want to output the help and avoid
    // horrible template error to be displayed to the user.
    std::cout << "\"name\" is required!" << "\n";
    std::cout << desc << std::endl;
    return EXIT_FAILURE;
  }
  if (vm.count("configuration")) {
    configurationSource = vm["configuration"].as<string>();
  } else {
    // we don't use the "required" option of the po::value because we want to output the help and avoid
    // horrible template error to be displayed to the user.
    std::cout << "\"configuration\" is required!" << "\n";
    std::cout << desc << std::endl;
    return EXIT_FAILURE;
  }

  try {
    // Config parsing
    Checker checker(checkerName, configurationSource);
    unique_ptr<ConfigurationInterface> config = ConfigurationFactory::getConfiguration(configurationSource);;
    int numberCheckers = config->get<int>("checkers/numberCheckers").value();
    int numberTasks = config->get<int>("checkers/numberTasks").value();
    int id = config->get<int>(checkerName + "/id").value();
    string addresses = config->get<string>("checkers/tasksAddresses").value();
    vector<string> addressesVector;
    boost::algorithm::split(addressesVector, addresses, boost::is_any_of(","), boost::token_compress_on);
    vector<string> addressesForThisChecker;
    for (int i = 0; i < numberTasks; i++) {
      if (i % numberCheckers == id) {
        addressesForThisChecker.push_back(addressesVector[i]);
        cout << "We will get data from this address : " << addressesVector[i] << endl;
      }
    }
    for (auto address : addressesForThisChecker) {
      checker.createChannel("sub", "connect", address, "data-in", true);
    }

    // Get the proper transport factory
#ifdef NANOMSG
    FairMQTransportFactory* transportFactory = new FairMQTransportFactoryNN();
#else
    FairMQTransportFactory *transportFactory = new FairMQTransportFactoryZMQ();
#endif

    checker.CatchSignals();

    checker.SetTransport(transportFactory);

    checker.ChangeState("INIT_DEVICE");
    checker.WaitForEndOfState("INIT_DEVICE");

    checker.ChangeState("INIT_TASK");
    checker.WaitForEndOfState("INIT_TASK");

    checker.ChangeState("RUN");
    checker.WaitForEndOfState("RUN");
//    checker.InteractiveStateLoop();
  } catch (std::string const &e) {
    cerr << e << endl;
  } catch (...) {
    string diagnostic = boost::current_exception_diagnostic_information();
    std::cerr << "Unexpected exception, diagnostic information follows:\n" << diagnostic << endl;
    if (diagnostic == "No diagnostic information available.") {
      throw;
    }
  }

  return EXIT_SUCCESS;
}
