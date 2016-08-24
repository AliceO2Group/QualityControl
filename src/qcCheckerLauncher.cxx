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
// O2
#include "Common/signalUtilities.h"
#include "QualityControl/Checker.h"
#include "QualityControl/Version.h"

using namespace std;
using namespace AliceO2::QualityControl::Core;
using namespace AliceO2::QualityControl::Checker;
namespace po = boost::program_options;

int main(int argc, char *argv[])
{
  string checkerName, configurationSource;

  // This is needed for ROOT
  TApplication app("a", 0, 0);
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
    ConfigFile config;
    config.load(configurationSource);
    int numberCheckers = config.getValue<int>("checkers.numberCheckers");
    int numberTasks = config.getValue<int>("checkers.numberTasks");
    int id = config.getValue<int>(checkerName + ".id");
    string addresses = config.getValue<string>("checkers.tasksAddresses");
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
      checker.createChannel("sub", "connect", address, "data-in");
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
    checker.InteractiveStateLoop();
  }
  catch (const boost::exception &e) {
    cerr << diagnostic_information(e) << endl;
  }
  catch (const std::exception &e) {
    cerr << "Error: " << e.what() << endl;
  }
  catch (const std::string &e) {
    cerr << "Error: " << e << endl;
  }

  return EXIT_SUCCESS;
}
