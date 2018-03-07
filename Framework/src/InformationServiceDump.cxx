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
/// \author bvonhall
/// \file InformationServiceDump.cxx
///

#include "InformationServiceDump.h"

#include "FairMQLogger.h"

#include <string>
#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace std;
namespace pt = boost::property_tree;

InformationServiceDump::InformationServiceDump()
{
  OnData("info_service_input", &InformationServiceDump::HandleData);
}

bool InformationServiceDump::HandleData(FairMQMessagePtr &msg, int /*index*/)
{
  string *receivedData = new std::string(static_cast<char *>(msg->GetData()), msg->GetSize());
  LOG(INFO) << "Received data : ";
  LOG(INFO) << "    " << *receivedData;

  return true; // keep running
}

InformationServiceDump::~InformationServiceDump()
{
}