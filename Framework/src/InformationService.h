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
/// \file InformationService.h
///


#ifndef QC_INFORMATIONSERVICE_H
#define QC_INFORMATIONSERVICE_H

#include <InfoLogger/InfoLogger.hxx>
#include "FairMQDevice.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/asio.hpp>

namespace pt = boost::property_tree;

/// \brief Collect the list of objects published by all the tasks and make it available to clients.
///
/// The InformationService receives the list of objects published by each task.
/// It keeps a list of all tasks and objects and send it upon request to clients. It also publishes updates when
/// new information comes from the tasks.
///
/// See InformationService.json to know the port where updates are published.
/// See InformationService.json to know the port where to request information for all tasks (param "all") or
/// for a specific task (param "<name_of_task>").
/// See runInformationService.cxx for the steering code.
///
/// Example usage :
///      qcInfoService -c /absolute/path/to/InformationService.json -n information_service \\
///                    --id information_service --mq-config /absolute/path/to/InformationService.json
///
/// Format of the string coming from the tasks :
///      `task_id:obj0,obj1,obj2`
/// Format of the JSON output for one task or all tasks :
///      See README
///
/// \todo Handle tasks dying and their removal from the cache and the publication of an update (heartbeat ?).
/// \todo Handle tasks sending information that they are disappearing.

class InformationService : public FairMQDevice
{
  public:
    InformationService();
    virtual ~InformationService();

  protected:
    /// Callback for data coming from qcTasks
    bool handleTaskInputData(FairMQMessagePtr&, int);
    /// Callback for the requests coming from clients
    bool handleRequestData(FairMQMessagePtr&, int);
    void Init();

  private:
    /// Extract the list of objects from the string received from the tasks
    std::vector<std::string> getObjects(std::string *receivedData);
    /// Extract the task name from the string received from the tasks
    std::string getTaskName(std::string *receivedData);
    /// Produce the JSON string for the specified task
    std::string produceJson(std::string taskName);
    /// Produce the JSON string for all tasks and objects
    std::string produceJsonAll();
    /// Send the JSON string to all clients (subscribers)
    void sendJson(std::string *json);
    pt::ptree buildTaskNode(std::string taskName);
    void checkTimedOut();
    /// Compute and send the JSON using the inputString from a task
    bool handleTaskInputData(std::string inputString);
    /// Reads a file containing data in format as received from the tasks.
    /// Store the items and use them at regular intervals to simulate tasks inputs.
    /// Calling again this method will delete the former fake data cache.
    void readFakeDataFile(std::string filePath);

  private:
    std::map<std::string,std::vector<std::string>> mCacheTasksData; /// the list of objects names for each task
    std::map<std::string /*task name*/, size_t /*hash of the objects list*/> mCacheTasksObjectsHash; /// used to check whether we already have received this list of objects
    boost::asio::deadline_timer *mTimer; /// the asynchronous timer to check if agents have timed out
    std::vector<std::string> mFakeData; /// container for the fake data (if any). Each line is in a string and used in turn.
    int mFakeDataIndex;
    // variables for the timer
    boost::asio::io_service io;
    std::thread *th;

};

#endif //QC_INFORMATIONSERVICE_H
