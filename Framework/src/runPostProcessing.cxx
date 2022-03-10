// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file    runPostProcessing.cxx
/// \author  Piotr Konopka
///
/// \brief This is a standalone executable to run postprocessing

#include "QualityControl/PostProcessingRunner.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/runnerUtils.h"

#include <Common/Timer.h>
#include <boost/program_options.hpp>
#include <Configuration/ConfigurationFactory.h>

using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;
using namespace AliceO2::Common;
using namespace o2::configuration;
namespace bpo = boost::program_options;

int main(int argc, const char* argv[])
{
  try {
    bpo::options_description desc{ "Options" };
    desc.add_options()                                                                                       //
      ("help,h", "Help screen")                                                                              //
      ("config", bpo::value<std::string>(), "Absolute path to a configuration file, preceded with backend.") //
      ("name", bpo::value<std::string>(), "Name of a post processing task to run")                           //
      ("override-values", bpo::value<std::string>(),                                                         //
       "QC configuration file key/value pairs which should be overwritten. "                                 //
       "The format is \"full.path.to.key=value[;full.path.to.key=value]\".")                                 //
      ("timestamps,t", bpo::value<std::vector<uint64_t>>()->composing(),
       "Space-separated timestamps (ms since epoch) which should be given to the post processing task."
       " Effectively, it ignores triggers declared in the configuration file and replaces them with"
       " TriggerType::Manual with given timestamps. The first value is used for initalization trigger, the last for"
       " finalization, so at least two are required.");

    bpo::positional_options_description positionalArgs;
    positionalArgs.add("timestamps", -1);

    bpo::variables_map vm;
    store(bpo::command_line_parser(argc, argv).options(desc).positional(positionalArgs).run(), vm);
    notify(vm);

    QcInfoLogger::setFacility("runPostProcessing");

    if (vm.count("help")) {
      ILOG(Info, Support) << desc << ENDM;
      return 0;
    } else if (vm.count("name") == 0 && vm.count("config") == 0) {
      ILOG(Error, Support) << "No name and/or config parameters provided" << ENDM;
      return 1;
    }

    PostProcessingRunner runner(vm["name"].as<std::string>());

    auto config = ConfigurationFactory::getConfiguration(vm["config"].as<std::string>());
    auto configTree = config->getRecursive();
    if (vm.count("override-values")) {
      auto keyValuesToOverride = parseOverrideValues(vm["override-values"].as<std::string>());
      overrideValues(configTree, keyValuesToOverride);
    }

    int periodUs = static_cast<int>(1000000 * configTree.get<double>("qc.config.postprocessing.periodSeconds", 10.0));

    runner.init(configTree);

    if (vm.count("timestamps")) {
      // running the PP task on a set of timestamps
      runner.runOverTimestamps(vm["timestamps"].as<std::vector<uint64_t>>());
    } else {
      // running the PP task with an event loop
      runner.start();

      Timer timer;
      timer.reset(periodUs);
      while (runner.run()) {
        while (timer.getRemainingTime() < 0) {
          timer.increment();
        }
        usleep(1000000.0 * timer.getRemainingTime());
      }
    }
    runner.stop();
    return 0;
  } catch (const bpo::error& ex) {
    ILOG(Error, Ops) << "Exception caught: " << ex.what() << ENDM;
    return 1;
  } catch (const boost::exception& ex) {
    ILOG(Error, Ops) << "Exception caught: " << boost::current_exception_diagnostic_information(true) << ENDM;
    return 1;
  }

  return 0;
}