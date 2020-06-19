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
/// \file    runPostProcessing.cxx
/// \author  Piotr Konopka
///
/// \brief This is a standalone executable to run postprocessing

#include "QualityControl/PostProcessingRunner.h"
#include "QualityControl/QcInfoLogger.h"

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
      ("period", bpo::value<double>()->default_value(10.0), "Cycle period of checking triggers in seconds");

    bpo::variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);
    notify(vm);

    if (vm.count("help")) {
      ILOG(Info) << desc << ENDM;
      return 0;
    } else if (vm.count("name") == 0 && vm.count("config") == 0) {
      ILOG(Error) << "No name and/or config parameters provided" << ENDM;
      return 1;
    }

    int periodUs = static_cast<int>(1000000 * vm["period"].as<double>());
    PostProcessingRunner runner(vm["name"].as<std::string>());

    auto config = ConfigurationFactory::getConfiguration(vm["config"].as<std::string>());

    runner.init(config->getRecursive());
    runner.start();

    Timer timer;
    timer.reset(periodUs);

    while (runner.run()) {
      while (timer.getRemainingTime() < 0) {
        timer.increment();
      }
      usleep(1000000.0 * timer.getRemainingTime());
    }
    runner.stop();
    return 0;
  } catch (const bpo::error& ex) {
    ILOG(Error) << "Exception caught: " << ex.what() << ENDM;
    return 1;
  }

  return 0;
}