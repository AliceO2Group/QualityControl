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

#include "QualityControl/Bookkeeping.h"
#include <iostream>
#include <boost/program_options.hpp>
#include "BookkeepingApi/BkpProtoClientFactory.h"
#include "QualityControl/Activity.h"
#include <Common/Timer.h>
#include "QualityControl/QcInfoLogger.h"

using namespace std;
namespace bpo = boost::program_options;
using namespace o2::bkp::api;
using namespace o2::bkp::api::proto;
using namespace o2::quality_control::core;

/**
 * A small utility to stress test the bookkeeping api.
 */

int main(int argc, const char* argv[])
{
  bpo::options_description desc{ "Options" };
  desc.add_options()("help,h", "Help screen")("url,u", bpo::value<std::string>()->required(), "URL to the Bookkeeping")("run,r", bpo::value<int>())("max,m", bpo::value<int>()->default_value(10000), "Max number of executions, default: 10000")("printCycles,p", bpo::value<int>()->default_value(1000), "We print every X cycles, default: 1000")("printActivity", bpo::value<bool>()->default_value(false), "just to check that we get something in the activity.")("delay,d", bpo::value<int>()->default_value(0), "Minimum delay between calls in ms, default 0");

  bpo::variables_map vm;
  store(parse_command_line(argc, argv, desc), vm);

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 0;
  }
  notify(vm);

  const auto url = vm["url"].as<string>();
  cout << "url : " << url << endl;
  const auto run = vm["run"].as<int>();
  cout << "run : " << run << endl;
  const auto max = vm["max"].as<int>();
  cout << "max : " << max << endl;
  const auto printCycles = vm["printCycles"].as<int>();
  cout << "printCycles : " << printCycles << endl;
  const auto printActivity = vm["printActivity"].as<bool>();
  cout << "printActivity : " << printActivity << endl;
  const auto minDelay = vm["delay"].as<int>();
  cout << "minDelay : " << minDelay << endl;

  ILOG_INST.filterDiscardDebug(true);
  ILOG_INST.filterDiscardLevel(11);

  Bookkeeping::getInstance().init(url);

  AliceO2::Common::Timer timer;
  AliceO2::Common::Timer triggerTimer;
  triggerTimer.reset();
  Activity activity;
  double totalDuration = 0;
  double cycleDuration = 0;
  int numberOfExecutionsInCycle = 0;
  int totalNumberOfExecutions = 0;

  while (totalNumberOfExecutions < max) {
    if (triggerTimer.isTimeout()) {
      numberOfExecutionsInCycle++;
      totalNumberOfExecutions++;
      triggerTimer.reset(minDelay * 1000);
      timer.reset();
      Bookkeeping::getInstance().populateActivity(activity, run);
      auto duration = timer.getTime();
      if (printActivity) {
        cout << activity << endl;
      }
      totalDuration += duration;
      cycleDuration += duration;
      if (totalNumberOfExecutions % printCycles == 0) {
        cout << "average duration last " << printCycles << " calls in [ms]: " << cycleDuration / numberOfExecutionsInCycle * 1000 << endl;
        numberOfExecutionsInCycle = 0;
        cycleDuration = 0;
      }
    }
  }
  cout << "average duration overall in ms : " << totalDuration / totalNumberOfExecutions * 1000 << endl;
}