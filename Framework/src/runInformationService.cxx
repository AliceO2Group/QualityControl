// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
//

///
/// \author Barthelemy von Haller
/// \file runInformationService.cxx
///

#include "runFairMQDevice.h"
#include "InformationService.h"

namespace bpo = boost::program_options;

void addCustomOptions(bpo::options_description &options)
{
  options.add_options()
    ("fake-data-file", bpo::value<std::string>()->default_value(""),
     "File containing JSON to use as input (useful for tests if no tasks is running). It is used to reply to requests. "
       "It is reloaded every 10 seconds and if it changed it is published to the clients.");
}

FairMQDevicePtr getDevice(const FairMQProgOptions & /*config*/)
{
  InformationService *is = new InformationService();

  return is;
}
