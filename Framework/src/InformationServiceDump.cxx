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

#include <boost/date_time/posix_time/posix_time.hpp>

using namespace std;
namespace pt = boost::property_tree;

int requestIntervals = 5; // in seconds

void InformationServiceDump::sendRequest()
{
  string* text = new string("asdf");
  cout << "preparing request " << endl;
  FairMQMessagePtr request(NewMessage(const_cast<char*>(text->c_str()), // data
                                      text->length(), // size
                                      [](void* /*data*/, void* object) { delete static_cast<string*>(object); }, // deletion callback
                                      text)); // object that manages the data
  cout << "Sending request" << endl;
  if (Send(request, "send_request") <= 0)
  {
    cout << "problem sending request" << endl;
  }

  // restart timer
  mTimer.expires_at(mTimer.expires_at() + boost::posix_time::seconds(requestIntervals));
  mTimer.async_wait(boost::bind(&InformationServiceDump::sendRequest, this));
}

bool InformationServiceDump::ConditionalRun()
{
  string* text = new string("asdf");
  cout << "preparing request " << endl;
  FairMQMessagePtr request(NewMessage(const_cast<char*>(text->c_str()), // data
                                      text->length(), // size
                                      [](void* /*data*/, void* object) { delete static_cast<string*>(object); }, // deletion callback
                                      text)); // object that manages the data
  cout << "Sending request" << endl;
  if (Send(request, "send_request") <= 0)
  {
    cout << "problem sending request" << endl;
  }
  this_thread::sleep_for(chrono::seconds(5));

  return true;
}

InformationServiceDump::InformationServiceDump(boost::asio::io_service& io)
  : mTimer(io, boost::posix_time::seconds(requestIntervals))
{
  OnData("info_service_input", &InformationServiceDump::HandleData);

//  mTimer.async_wait(boost::bind(&InformationServiceDump::sendRequest, this));
}

bool InformationServiceDump::HandleData(FairMQMessagePtr &msg, int /*index*/)
{
  string *receivedData = new std::string(static_cast<char *>(msg->GetData()), msg->GetSize());
  LOG(INFO) << "Received data : ";
  LOG(INFO) << "    " << *receivedData;

  string* text = new string("asdf");
  cout << "preparing request " << endl;
  FairMQMessagePtr request(NewMessage(const_cast<char*>(text->c_str()), // data
                                      text->length(), // size
                                      [](void* /*data*/, void* object) { delete static_cast<string*>(object); }, // deletion callback
                                      text)); // object that manages the data
  cout << "Sending request" << endl;
  if (Send(request, "send_request") > 0) {
    FairMQMessagePtr reply(NewMessage());
    if (Receive(reply, "send_request") >= 0)
    {
      LOG(INFO) << "Received reply from server: \"" << string(static_cast<char*>(reply->GetData()), reply->GetSize()) << "\"";
    } else {
      cout << "Problem receiving reply" << endl;
    }
  } else {
    cout << "problem sending request" << endl;
  }

  return true; // keep running
}

InformationServiceDump::~InformationServiceDump()
{
}