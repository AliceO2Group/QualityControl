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
/// \file InformationService.cxx
///

#include "InformationService.h"

#include "FairMQLogger.h"

#include <string>
#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace std;
namespace pt = boost::property_tree;

InformationService::InformationService()
{
  OnData("tasks_input", &InformationService::HandleData);
}

bool InformationService::HandleData(FairMQMessagePtr &msg, int /*index*/)
{
  string *receivedData = new std::string(static_cast<char *>(msg->GetData()), msg->GetSize());
  LOG(INFO) << "Received data, processing...";
  LOG(INFO) << "    " << *receivedData;

  // parse
  std::string taskName = receivedData->substr(0, receivedData->find(":"));
  std::string objectsString = receivedData->substr(receivedData->find(":") + 1, receivedData->length());
  boost::char_separator<char> sep(",");
  typedef boost::tokenizer<boost::char_separator<char> > t_tokenizer;
  t_tokenizer tok(objectsString, sep);
  vector<string> objects;
  LOG(DEBUG) << "task : " << taskName;
  LOG(DEBUG) << "objects : " << objectsString;

  // store
  for (t_tokenizer::iterator beg = tok.begin(); beg != tok.end(); ++beg) {
    objects.push_back(*beg);
  }
  mCache[taskName] = objects;

  // json
  pt::ptree task_node;
  task_node.put("name", taskName);
  pt::ptree objects_node;
  for (auto &object : mCache[taskName]) {
    pt::ptree object_node;
    object_node.put("id", object);
    objects_node.push_back(std::make_pair("", object_node));
  }
  task_node.add_child("objects", objects_node);
  std::stringstream ss;
  pt::json_parser::write_json(ss, task_node);

  // publish
  string* json = new std::string(ss.str());
  FairMQMessagePtr msg2(NewMessage(const_cast<char *>(json->c_str()),
                                   json->length(),
                                   [](void * /*data*/, void *object) { delete static_cast<string *>(object); },
                                   json));
  int ret = Send(msg2, "updates_output");
  if(ret < 0)
  {
    LOG(error) << "Error sending update" << endl;
  }

  return true; // keep running
}

InformationService::~InformationService()
{
}