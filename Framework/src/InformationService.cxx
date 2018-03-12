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

#include "QualityControl/QcInfoLogger.h"

using namespace std;
typedef boost::tokenizer<boost::char_separator<char> > t_tokenizer;
using namespace o2::quality_control::core;

InformationService::InformationService()
{
  OnData("tasks_input", &InformationService::HandleData);
  OnData("request_data", &InformationService::requestData);
}

bool InformationService::requestData(FairMQMessagePtr &request, int /*index*/)
{
  string requestParam = string(static_cast<char *>(request->GetData()), request->GetSize());
  LOG(INFO) << "Received request from client: \"" << requestParam << "\"";

  string *result = nullptr;
  if(requestParam == "all") {
    result = new string(produceJsonAll());
  } else {
    if(mCacheTasksData.count(requestParam) >0 ) {
      result = new string(produceJson(requestParam));
    } else {
      result = new string("{\"error\": \"no such task\"}");
    }
  }

  LOG(INFO) << "Sending reply to client.";
  FairMQMessagePtr reply(NewMessage(const_cast<char *>(result->c_str()), // data
                                    result->length(), // size
                                    [](void * /*data*/,
                                       void *object) { delete static_cast<string *>(object); }, // deletion callback
                                    result)); // object that manages the data
  if (Send(reply, "request_data") <= 0) {
    cout << "error sending reply" << endl;
  }
  return true; // keep running
}

bool InformationService::HandleData(FairMQMessagePtr &msg, int /*index*/)
{
  string *receivedData = new std::string(static_cast<char *>(msg->GetData()), msg->GetSize());
  LOG(INFO) << "Received data, processing...";
  LOG(INFO) << "    " << *receivedData;

  // parse
  std::string taskName = getTaskName(receivedData);
  vector<string> objects = getObjects(receivedData);
  LOG(DEBUG) << "task : " << taskName;

  // store
  mCacheTasksData[taskName] = objects;

  // json
  string *json = new std::string(produceJson(taskName));

  // publish
  sendJson(json);

  return true; // keep running
}

vector<string> InformationService::getObjects(string *receivedData)
{
  vector<string> objects;
  std::string objectsString = receivedData->substr(receivedData->find(":") + 1, receivedData->length());
  LOG(DEBUG) << "objects : " << objectsString;
  boost::char_separator<char> sep(",");
  t_tokenizer tok(objectsString, sep);
  for (t_tokenizer::iterator beg = tok.begin(); beg != tok.end(); ++beg) {
    objects.push_back(*beg);
  }
  return objects;
}

std::string InformationService::getTaskName(std::string *receivedData)
{
  return receivedData->substr(0, receivedData->find(":"));
}

pt::ptree InformationService::buildTaskNode(std::string taskName)
{
  pt::ptree task_node;
  task_node.put("name", taskName);
  pt::ptree objects_node;
  for (auto &object : mCacheTasksData[taskName]) {
    pt::ptree object_node;
    object_node.put("id", object);
    objects_node.push_back(std::make_pair("", object_node));
  }
  task_node.add_child("objects", objects_node);
  return task_node;
}

std::string InformationService::produceJson(std::string taskName)
{
  pt::ptree taskNode = buildTaskNode(taskName);

  std::stringstream ss;
  pt::json_parser::write_json(ss, taskNode);
  LOG(DEBUG) << "json : " << endl << ss.str();
//  QcInfoLogger::GetInstance() << infologger::Debug << "json : \n" << *json << infologger::endm;
  return ss.str();
}

std::string InformationService::produceJsonAll()
{
  string result;
  pt::ptree main_node;

  pt::ptree tasksListNode;
  for (const auto &taskTuple : mCacheTasksData) {
    pt::ptree taskNode = buildTaskNode(taskTuple.first);
    tasksListNode.push_back(std::make_pair("", taskNode));
  }
  main_node.add_child("tasks", tasksListNode);

  std::stringstream ss;
  pt::json_parser::write_json(ss, main_node);
  LOG(debug) << "json : " << endl << ss.str();
  return ss.str();
}

void InformationService::sendJson(std::string *json)
{
  FairMQMessagePtr msg2(NewMessage(const_cast<char *>(json->c_str()),
                                   json->length(),
                                   [](void * /*data*/, void *object) { delete static_cast<string *>(object); },
                                   json));
  int ret = Send(msg2, "updates_output");
  if (ret < 0) {
    LOG(error) << "Error sending update" << endl;
  }
}

InformationService::~InformationService()
{
}