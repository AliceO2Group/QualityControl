// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   CcdbApi.h
/// \author Barthelemy von Haller
///

#ifndef PROJECT_CCDBAPI_H
#define PROJECT_CCDBAPI_H

//#include <ctime>
//#include <chrono>
#include <string>
#include <memory>
#include <iostream>
#include <map>
#include <curl/curl.h>
#include <TObject.h>

namespace o2 {
namespace ccdb {

/*
 * Notes
 * - we need to have a way to query for all objects in a certain path, e.g. in "task_name/X/Y" or in "task_name"
 *    --> the new api should allow it. To be tested.
 */

/**
 * @todo use smart pointers ?
 * @todo handle errors and exceptions
 */
class CcdbApi //: public DatabaseInterface
{
  public:
    /// \brief Default constructor
    CcdbApi();
    /// \brief Default destructor
    virtual ~CcdbApi();

    /**
     * Initialize connection to CCDB
     * @param host The URL to the CCDB (e.g. "ccdb-test.cern.ch:8080")
     */
    void init(std::string host);

    /**
     * Stores an object in the CCDB
     * @param rootObject Shared pointer to the object to store.
     * @param path The path where the object is going to be stored.
     * @param metadata Key-values representing the metadata for this object.
     * @param startValidityTimestamp Start of validity. If omitted, current timestamp is used.
     * @param endValidityTimestamp End of validity. If omitted, current timestamp + 1 year is used.
     */
    void store(TObject* rootObject, std::string path, std::map<std::string, std::string> metadata,
               long startValidityTimestamp = -1, long endValidityTimestamp = -1);

    /**
     *
     * @param path
     * @param timestamp
     * @return
     */
    TObject* retrieve(std::string path, std::map<std::string, std::string> metadata,
      long timestamp = -1);

//    void disconnect();
//    void prepareTaskDataContainer(std::string taskName);
//    std::vector<std::string> getListOfTasksWithPublications();
//    std::vector<std::string> getPublishedObjectNames(std::string taskName);
//    void truncateObject(std::string taskName, std::string objectName);
//    /**
//     * Delete the matching version of this object.
//     * @todo Raise an exception if no such object exist.
//     * @param taskName
//     * @param objectName
//     * @param timestamp
//     */
//    void deleteObjectVersion(std::string taskName, std::string objectName, std::string timestamp);

  private:
    long getCurrentTimestamp();
    std::string getTimestampString(long timestamp);
    long getFutureTimestamp(int secondsInFuture);

    /**
     * Build the full url to store an object.
     * @param path The path where the object is going to be stored.
     * @param metadata Key-values representing the metadata for this object.
     * @param startValidityTimestamp Start of validity. If omitted or negative, the current timestamp is used.
     * @param endValidityTimestamp End of validity. If omitted or negative, current timestamp + 1 year is used.
     * @return The full url to store an object (url / startValidity / endValidity / [metadata &]* )
     */
    std::string getFullUrlForStorage(const std::string &path, const std::map<std::string, std::string> &metadata,
                                     long startValidityTimestamp = -1, long endValidityTimestamp = -1);

    /**
 * Build the full url to store an object.
 * @param path The path where the object is going to be stored.
 * @param metadata Key-values representing the metadata for this object.
 * @param timestamp When the object we retrieve must be valid. If omitted or negative, the current timestamp is used.
 * @return The full url to store an object (url / startValidity / endValidity / [metadata &]* )
 */
    std::string getFullUrlForRetrieval(const std::string &path, const std::map<std::string, std::string> &metadata,
                                     long timestamp = -1);

    /**
     * Return the listing of folder and/or objects in the subpath.
     * @param subpath The folder we want to list the children of.
     * @param accept The format of the returned string as an \"Accept\", i.e. text/plain, application/json, text/xml
     * @return The listing of folder and/or objects in the format requested and as returned by the http server.
     */
//    std::string getListing(std::string subpath = "", std::string accept = "text/plain");
    std::string mUrl;
    void curlInit();

};

}
}

#endif //PROJECT_CCDBAPI_H
