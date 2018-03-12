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
#include <options/FairMQProgOptions.h> // device->fConfig

using namespace std;
typedef boost::tokenizer<boost::char_separator<char> > t_tokenizer;
using namespace o2::quality_control::core;

int timeOutIntervals = 5; // in seconds

InformationService::InformationService() : th(nullptr), mFakeDataIndex(0)
{
  OnData("tasks_input", &InformationService::handleTaskInputData);
  OnData("request_data", &InformationService::handleRequestData);
}

void InformationService::Init()
{
  string fakeDataFile = fConfig->GetValue<string>("fake-data-file");

  // todo put this in a method
  if(fConfig->GetValue<string>("fake-data-file") != "") {
    // if user specified a fake data file we load the file
    std::string line;
    std::ifstream myfile(fakeDataFile);
    if(!myfile) //Always test the file open.
    {
      std::cout<<"Error opening fake data file"<< std::endl;
      return;
    }
    while (std::getline(myfile, line))
    {
      mFakeData.push_back(line);
    }

    // if user specified a fake data file we start a timer
    mTimer = new boost::asio::deadline_timer(io, boost::posix_time::seconds(timeOutIntervals));
    mTimer->async_wait(boost::bind(&InformationService::checkTimedOut, this));
    th = new thread([&] { io.run(); });
  }
}

InformationService::~InformationService()
{
}

void InformationService::checkTimedOut()
{
  cout << "checkTimedOut" << endl;

  cout << "index : " << mFakeDataIndex << endl;
  string line = mFakeData[mFakeDataIndex % mFakeData.size()];
  cout << "line : " << line << endl;
  handleTaskInputData(line);
  mFakeDataIndex++;

  // restart timer
  mTimer->expires_at(mTimer->expires_at() + boost::posix_time::seconds(timeOutIntervals));
  mTimer->async_wait(boost::bind(&InformationService::checkTimedOut, this));
}

bool InformationService::handleRequestData(FairMQMessagePtr &request, int /*index*/)
{
  string requestParam = string(static_cast<char *>(request->GetData()), request->GetSize());
  LOG(INFO) << "Received request from client: \"" << requestParam << "\"";

  string *result = nullptr;
  if (requestParam == "all") {
    result = new string(produceJsonAll());
  } else {
    if (mCacheTasksData.count(requestParam) > 0) {
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

bool InformationService::handleTaskInputData(FairMQMessagePtr &msg, int /*index*/)
{
  string *receivedData = new std::string(static_cast<char *>(msg->GetData()), msg->GetSize());
  LOG(INFO) << "Received data, processing...";
  LOG(INFO) << "    " << *receivedData;

  handleTaskInputData(*receivedData);

  return true; // keep running
}

bool InformationService::handleTaskInputData(std::string receivedData)
{
  std::string taskName = getTaskName(&receivedData);
  LOG(DEBUG) << "task : " << taskName;

  // check if new data
  boost::hash<std::string> string_hash;
  size_t hash = string_hash(receivedData);
  cout << "hash : " << hash << endl;
  cout << "mCacheTasksObjectsHash.count(taskName) : " << mCacheTasksObjectsHash.count(taskName) << endl;
  if(mCacheTasksObjectsHash.count(taskName) > 0)
    cout << "mCacheTasksObjectsHash[taskName] : " << mCacheTasksObjectsHash[taskName] << endl;
  if (mCacheTasksObjectsHash.count(taskName) > 0 && hash == mCacheTasksObjectsHash[taskName]) {
    LOG(INFO) << "Data already known, we skip it" << endl;
    return true;
  }
  mCacheTasksObjectsHash[taskName] = hash;

  // parse
  vector<string> objects = getObjects(&receivedData);

  // store
  mCacheTasksData[taskName] = objects;

  // json
  string *json = new std::string(produceJson(taskName));

  // publish
  sendJson(json);
}

void InformationService::readFile(std::string filePath)
{
  // Read file line by line and call handleTaskInputData for each line.
  // We could also read one line each call. or put it in a long vector and we read one element at
  // regular intervals.
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
