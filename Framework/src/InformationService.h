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


#ifndef PROJECT_INFORMATIONSERVICE_H
#define PROJECT_INFORMATIONSERVICE_H

#include <InfoLogger/InfoLogger.hxx>
#include "FairMQDevice.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

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

class InformationService : public FairMQDevice
{
  public:
    InformationService();
    virtual ~InformationService();

  protected:
    bool HandleData(FairMQMessagePtr&, int);
    bool requestData(FairMQMessagePtr&, int);

  private:
    std::vector<std::string> getObjects(std::string *receivedData);
    std::string getTaskName(std::string *receivedData);
    std::string produceJson(std::string taskName);
    std::string produceJsonAll();
    void sendJson(std::string *json);
    pt::ptree buildTaskNode(std::string taskName);

  private:
    std::map<std::string,std::vector<std::string>> mCacheTasksData;
};

#endif //PROJECT_INFORMATIONSERVICE_H
