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
/// \file InformationServiceDump.cxx
///

#include "InformationServiceDump.h"

#include <fairmq/FairMQLogger.h>
#include <options/FairMQProgOptions.h>

using namespace std;

InformationServiceDump::InformationServiceDump() { OnData("info_service_input", &InformationServiceDump::HandleData); }

InformationServiceDump::~InformationServiceDump() {}

bool InformationServiceDump::HandleData(FairMQMessagePtr& msg, int /*index*/)
{
  string* receivedData = new std::string(static_cast<char*>(msg->GetData()), msg->GetSize());
  LOG(INFO) << "Received data : ";
  LOG(INFO) << "    " << *receivedData;

  string* text = new string(fConfig->GetValue<string>("request-task"));
  LOG(INFO) << "Preparing request for \"" << *text << "\"";
  FairMQMessagePtr request(
    NewMessage(
      const_cast<char*>(text->c_str()),                                          // data
      text->length(),                                                            // size
      [](void* /*data*/, void* object) { delete static_cast<string*>(object); }, // deletion callback
      text));                                                                    // object that manages the data
  LOG(INFO) << "Sending request ";
  if (Send(request, "send_request") > 0) {
    FairMQMessagePtr reply(NewMessage());
    if (Receive(reply, "send_request") >= 0) {
      LOG(INFO) << "Received reply from server: \"" << string(static_cast<char*>(reply->GetData()), reply->GetSize())
                << "\"";
    } else {
      LOG(ERROR) << "Problem receiving reply";
    }
  } else {
    LOG(ERROR) << "problem sending request";
  }

  return true; // keep running
}
