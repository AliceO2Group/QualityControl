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
/// \file   runCcdbBenchmark.cxx
/// \author Barthelemy von Haller
///

#include "runFairMQDevice.h"
#include "CcdbBenchmark.h"

namespace bpo = boost::program_options;

void addCustomOptions(bpo::options_description& options)
{
  options.add_options()
    ("number-objects", bpo::value<uint64_t>()->default_value(1), "Number of objects to try to send to the CCDB every second (default : 1)")
    ("size-objects", bpo::value<uint64_t>()->default_value(1), "Size of the objects to send (in kB, 1, 10, 100, 1000, default : 1)")
    ("max-iterations", bpo::value<uint64_t>()->default_value(3), "Maximum number of iterations of Run/ConditionalRun/OnData (0 - infinite, default : 3)")
    ("ccdb-url", bpo::value<std::string>()->default_value("ccdb-test.cern.ch:8080"), "Database url (default : ccdb-test.cern.ch:8080")
    ("task-name", bpo::value<std::string>()->default_value("benchmarkTask"), "Name of the task (default : benchmarkTask)")
    ("object-name", bpo::value<std::string>()->default_value("benchmark"), "Name of the object (default : benchmark)")
    ("delete", bpo::value<int>()->default_value(0), "Deletion mode (deletes all the versions of the object, 1:true, 0:false)")
    ("monitoring-url", bpo::value<std::string>()->default_value("infologger://"), "The URL to the monitoring system (default : \"infologger://\")");
}

FairMQDevicePtr getDevice(const FairMQProgOptions& /*config*/)
{
  return new o2::quality_control::core::CcdbBenchmark();
}