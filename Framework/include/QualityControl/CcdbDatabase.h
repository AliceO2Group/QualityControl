//
// Created by bvonhall on 17/10/17.
//

#ifndef PROJECT_CCDBDATABASE_H
#define PROJECT_CCDBDATABASE_H

#include <curl/curl.h>
#include "QualityControl/DatabaseInterface.h"

namespace o2 {
namespace quality_control {
namespace repository {

/*
 * Notes
 * - having 1 file per object per version server-side might lead to a tremendous number of files.
 * - how to add a new filter ? such as expert/shifter flag
 * - we really need a C++ interface hiding the curl complexity.
 * - what are those time intervals ? what does it mean for us ?
 * - how to know the real time at which the object was stored ?
 * - we rather have a task_name/X/Y/Z/object_name/.../time where X/Y/Z are actually part of object_name but happen to have slashes, to build a hierarchy of objects
 * - we need to have a way to query for all objects in a certain path, e.g. in "task_name/X/Y" or in "task_name"
 * - when retrieving an object, despite what the usage menu says, the time can't be omitted.
 * - initial tests show that it seems pretty slow.
 * - We need getListOfTasksWithPublications() and getPublishedObjectNames()
 */

class CcdbDatabase : public DatabaseInterface
{
  public:
    CcdbDatabase();
    void connect(std::string host, std::string database, std::string username, std::string password) override;
    void store(o2::quality_control::core::MonitorObject *mo) override;
    core::MonitorObject *retrieve(std::string taskName, std::string objectName) override;
    void disconnect() override;
    void prepareTaskDataContainer(std::string taskName) override;
    std::vector<std::string> getListOfTasksWithPublications() override;
    std::vector<std::string> getPublishedObjectNames(std::string taskName) override;

  private:
    core::MonitorObject *downloadObject( std::string location);
    std::string getObjectPath(std::string taskName, std::string objectName);

    std::string url;
};

}
}
}

#endif //PROJECT_CCDBDATABASE_H
