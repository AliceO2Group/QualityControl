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

#include <iostream>
#include <boost/program_options.hpp>
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/stringUtils.h"
#include "QualityControl/ptreeUtils.h"
#include <Configuration/ConfigurationFactory.h>
#include <boost/property_tree/json_parser.hpp>

using namespace std;
namespace bpo = boost::program_options;
using namespace o2::quality_control::core;
using boost::property_tree::ptree;
using namespace o2::configuration;

/**
 * A small utility merge several configs into one.
 */

int main(int argc, const char* argv[])
{
  bpo::options_description desc{ "Options" };
  desc.add_options()("help,h", "Help screen")("configs", bpo::value<std::string>()->required(), "comma separated list of configs")("out,o",bpo::value<std::string>()->default_value(""), "file to store the merged config");

  bpo::variables_map vm;
  store(parse_command_line(argc, argv, desc), vm);

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 0;
  }
  notify(vm);
  const auto configs = vm["configs"].as<string>();
  const auto out = vm["out"].as<string>();
  std::vector<std::string> allConfigurationSources;
  allConfigurationSources = o2::quality_control::core::splitString(configs, ',');

  ILOG_INST.filterDiscardDebug(true);
  ILOG_INST.filterDiscardLevel(11);

  ptree merged;
  for(auto config : allConfigurationSources) {
    auto newTree = ConfigurationFactory::getConfiguration(config)->getRecursive();
    boost::property_tree::write_json(std::cout, newTree);
    mergeInto(newTree, merged);
  }

  if(out.empty()) {
    boost::property_tree::write_json(std::cout, merged);
  } else {
    ofstream outFile;
    outFile.open (out);
    boost::property_tree::write_json(outFile, merged);
    outFile.close();
  }
}