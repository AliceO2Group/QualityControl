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
/// \file   TObject2Json.cxx
/// \author Adam Wegrzynek
/// \author Vladimir Kosmala
///

// TObject2Json
#include "TObject2JsonServer.h"
#include <boost/program_options.hpp>
#include <iostream>

using o2::quality_control::tobject_to_json::TObject2JsonServer;

int main(int argc, char *argv[])
{
  boost::program_options::variables_map vm;
  boost::program_options::options_description desc("Allowed options");
  desc.add_options()
    ("backend", boost::program_options::value<std::string>()->required(), "Backend URL, eg.: mysql://<login>:<password>@<hostname>:<port>/<database>")
    ("zeromq-server", boost::program_options::value<std::string>()->required(), "ZeroMQ server endpoint, eg.: tcp://<host>:<port>")
    ("workers", boost::program_options::value<int>(), "Number of worker threads, eg.: 8")
  ;
  try {
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
    boost::program_options::notify(vm);
  } catch (...) {
    std::cout << desc << std::endl;
    return 1;
  }
  std::string backend = vm["backend"].as<std::string>();
  std::string zeromq = vm["zeromq-server"].as<std::string>();
  int workers = 8;
  if (vm.count("workers")) {
    workers = vm["workers"].as<int>();
  }

  TObject2JsonServer server;
  try {
    server.start(backend, zeromq, workers);
  } catch (const std::exception& error) {
    std::cout << error.what() << std::endl;
    return 2;
  }

  return 0;
}
