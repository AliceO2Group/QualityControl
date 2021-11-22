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
/// \file    runMetadataUpdater.cxx
/// \author  Barthelemy von Haller
///
/// \brief Easily update the metadata of an object in the QCDB or add new metadata if it does not exist yet.
///
/// Example: o2-qc-metadata-updater --url ccdb-test.cern.ch:8080 --path Test/pid61065/Test --pair something,else --id 8b9728fe-486b-11ec-afda-2001171b226b --pair key1,value1
///
/// Note: commas can be escaped if they must be part of the key: "my,key" --> "my\\,key". Note that it needs double escaping.
///       commas don't have to escaped in the value.

#include <CCDB/CcdbApi.h>
#include <CCDB/CCDBTimeStampUtils.h>
#include <iostream>
#include <vector>
#include <boost/program_options.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>

namespace bpo = boost::program_options;
using namespace std;

int main(int argc, const char* argv[])
{
  try {
    bpo::options_description desc{ "Options" };
    desc.add_options()("help,h", "Help screen")("url,u", bpo::value<std::string>()->required(), "URL to the QCDB")("path,p", bpo::value<std::string>()->required(), "Path to the object to update")("timestamp,t", bpo::value<long>()->default_value(o2::ccdb::getCurrentTimestamp()), "Timestamp to select the object")("id", bpo::value<std::string>()->default_value(""), "Id of the object to select")("pair", bpo::value<vector<string>>()->required(), "Key-value pair to update the metadata (e.g. --pair \"1,oil\", can be added multiple times)");

    bpo::variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);
    notify(vm);

    if (vm.count("help")) {
      std::cout << desc << std::endl;
      return 0;
    }

    const auto url = vm["url"].as<string>();
    const auto path = vm["path"].as<string>();
    const auto timestamp = vm["timestamp"].as<long>();
    const auto id = vm["id"].as<string>();
    const auto pairs = vm["pair"].as<vector<string>>();

    // prepare the key value map, take into account escaped commas
    map<string, string> metadata;
    for (auto p : pairs) {
      size_t hit = -1; // on purpose ... don't worry
      do { // make sure we ignore the escaped commas
        hit = p.find(',', hit+1);
      } while(hit != string::npos && hit > 0 && p.at(hit-1) == '\\');
      if (hit == string::npos) {
        continue;
      }
      metadata[p.substr(0,hit)] = p.substr(hit+1);
    }
    if (metadata.empty()) {
      cout << "No proper pairs found, aborting." << endl;
      return -1;
    }

    std::cout << "PARAMETERS" << std::endl;
    std::cout << "url................" << url << std::endl;
    std::cout << "path,.............." << path << std::endl;
    std::cout << "timestamp.........." << timestamp << std::endl;
    std::cout << "id................." << id << std::endl;
    std::cout << "pairs" << std::endl;
    for (auto m : metadata) {
      std::cout << "   |........" << m.first << " -> " << m.second << endl;
    }
    std::cout << std::endl;

    o2::ccdb::CcdbApi api;
    api.init(url);
    api.updateMetadata(path, metadata, timestamp, id);

    return 0;
  } catch (const bpo::error& ex) {
    std::cerr << "Exception caught: " << ex.what() << std::endl;
    return 1;
  }

  return 0;
}

// -0.378347x + 2782.30