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
/// \file   CcdbDatabase.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/CcdbDatabase.h"
#include <regex>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>
#include <curl/curl.h>
#include <TMessage.h>
#include <TObjString.h>
#include <algorithm>
#include <unordered_set>

using namespace std::chrono;

namespace o2 {
namespace quality_control {
namespace repository {

using namespace std;

void CcdbDatabase::connect(std::string host, std::string database, std::string username, std::string password)
{
  url = host;
}

void CcdbDatabase::connect(std::unique_ptr<ConfigurationInterface> &config)
{
  url = config->get<string>("qc/config/database/host").value();
}

void CcdbDatabase::store(o2::quality_control::core::MonitorObject *mo)
{
  // Serialize the object mo
  TMessage message(kMESS_OBJECT);
  message.Reset();
  message.WriteObjectAny(mo, mo->IsA());

  // Prepare URL and filename
  string fullUrl = url + "/" + mo->getTaskName() + "/" + mo->getName() + "/" + getTimestampString(getCurrentTimestamp())
                   + "/" + getTimestampString(
    getFutureTimestamp(60 * 60 * 24 * 365 * 10)); // todo set a proper timestamp for the end
  string tmpFileName = mo->getTaskName() + "_" + mo->getName() + ".root";

  // Curl preparation
  CURL *curl;
  CURLM *multi_handle;
  int still_running;
  struct curl_httppost *formpost = nullptr;
  struct curl_httppost *lastptr = nullptr;
  struct curl_slist *headerlist = nullptr;
  static const char buf[] = "Expect:";
  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "send",
               CURLFORM_BUFFER, tmpFileName.c_str(),
               CURLFORM_BUFFERPTR, message.Buffer(),
               CURLFORM_BUFFERLENGTH, message.Length(),
               CURLFORM_END);

  // Curl init
  curl = curl_easy_init();
  multi_handle = curl_multi_init();

  // Curl magic to send object
  // See https://curl.haxx.se/libcurl/c/multi-formadd.html for the details.
  // This piece does not comply to our coding guidelines as it is copy pasted from C example for CURL.
  headerlist = curl_slist_append(headerlist, buf);
  if (curl && multi_handle) {
    curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
//    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    curl_multi_add_handle(multi_handle, curl);
    curl_multi_perform(multi_handle, &still_running);

    do {
      int rc; /* select() return code */
      CURLMcode mc; /* curl_multi_fdset() return code */
      int maxfd = -1;
      long curl_timeo = -1;

      fd_set fdread;
      fd_set fdwrite;
      fd_set fdexcep;
      FD_ZERO(&fdread);
      FD_ZERO(&fdwrite);
      FD_ZERO(&fdexcep);

      /* set a suitable timeout to play around with */
      struct timeval timeout{1/*tv_sec*/, 0/*tv_usec*/};

      curl_multi_timeout(multi_handle, &curl_timeo);
      if (curl_timeo >= 0) {
        timeout.tv_sec = curl_timeo / 1000;
        if (timeout.tv_sec > 1) {
          timeout.tv_sec = 1;
        } else {
          timeout.tv_usec = (curl_timeo % 1000) * 1000;
        }
      }

      /* get file descriptors from the transfers */
      mc = curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);

      if (mc != CURLM_OK) {
        fprintf(stderr, "curl_multi_fdset() failed, code %d.\n", mc);
        break;
      }

      /* On success the value of maxfd is guaranteed to be >= -1. We call
         select(maxfd + 1, ...); specially in case of (maxfd == -1) there are
         no fds ready yet so we call select(0, ...) --or Sleep() on Windows--
         to sleep 100ms, which is the minimum suggested value in the
         curl_multi_fdset() doc. */

      if (maxfd == -1) {
        /* Portable sleep for platforms other than Windows. */
        struct timeval wait = {0, 100 * 1000}; /* 100ms */
        rc = select(0, nullptr, nullptr, nullptr, &wait);
      } else {
        /* Note that on some platforms 'timeout' may be modified by select().
           If you need access to the original value save a copy beforehand. */
        rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
      }

      switch (rc) {
        case -1:
          /* select error */
          break;
        case 0:
        default:
          /* timeout or readable/writable sockets */
          curl_multi_perform(multi_handle, &still_running);
          break;
      }
    }
    while (still_running);

    curl_multi_cleanup(multi_handle);

    /* always cleanup */
    curl_easy_cleanup(curl);

    /* then cleanup the formpost chain */
    curl_formfree(formpost);

    /* free slist */
    curl_slist_free_all(headerlist);
  }
}

/**
 * Struct to store the data we will receive from the CCDB with CURL.
 */
struct MemoryStruct
{
    char *memory;
    unsigned int size;
};

/**
 * Callback used by CURL to store the data received from the CCDB.
 * See https://curl.haxx.se/libcurl/c/getinmemory.html
 * @param contents
 * @param size
 * @param nmemb
 * @param userp a MemoryStruct where data is stored.
 * @return the size of the data we received and stored at userp.
 */
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  auto *mem = (struct MemoryStruct *) userp;

  mem->memory = (char *) realloc(mem->memory, mem->size + realsize + 1);
  if (mem->memory == nullptr) {
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

core::MonitorObject *CcdbDatabase::retrieve(std::string taskName, std::string objectName)
{
  // Note : based on https://curl.haxx.se/libcurl/c/getinmemory.html
  // Thus it does not comply to our coding guidelines as it is a copy paste.

  // Prepare CURL
  string fullUrl = url + "/" + taskName + "/" + objectName + "/" + getTimestampString(getCurrentTimestamp());
  CURL *curl_handle;
  CURLcode res;
  struct MemoryStruct chunk{(char *) malloc(1)/*memory*/, 0/*size*/};
  o2::quality_control::core::MonitorObject *mo = nullptr;
  curl_global_init(CURL_GLOBAL_ALL);

  /* init the curl session */
  curl_handle = curl_easy_init();

  /* specify URL to get */
  curl_easy_setopt(curl_handle, CURLOPT_URL, fullUrl.c_str());

  /* send all data to this function  */
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

  /* we pass our 'chunk' struct to the callback function */
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *) &chunk);

  /* some servers don't like requests that are made without a user-agent
     field, so we provide one */
  curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

  /* if redirected , we tell libcurl to follow redirection */
  curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);

  /* get it! */
  res = curl_easy_perform(curl_handle);

  /* check for errors */
  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n",
            curl_easy_strerror(res));
  } else {
    /*
     * Now, our chunk.memory points to a memory block that is chunk.size
     * bytes big and contains the remote file.
     */

//    printf("%lu bytes retrieved\n", (long) chunk.size);

    long response_code;
    res = curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
    if ((res == CURLE_OK) && (response_code != 404)) {

//    TDatime updatetime(statement->GetYear(2), statement->GetMonth(2), statement->GetDay(2), statement->GetHour(2),
//                       statement->GetMinute(2), statement->GetSecond(2));
//    int run = statement->IsNull(3) ? -1 : statement->GetInt(3);
//    int fill = statement->IsNull(4) ? -1 : statement->GetInt(4);

      TMessage mess(kMESS_OBJECT);
      mess.SetBuffer(chunk.memory, chunk.size, kFALSE);
      mess.SetReadMode();
      mess.Reset();
      mo = (o2::quality_control::core::MonitorObject *) (mess.ReadObjectAny(mess.GetClass()));
    } else {
      cerr << "invalid URL : " << fullUrl << endl;
    }

    // Print data
//    cout << "size : " << chunk.size << endl;
//    cout << "data : " << endl;
//    char* mem = (char*)chunk.memory;
//    for (int i = 0 ; i < chunk.size/4 ; i++)  {
//      cout << mem;
//      mem += 4;
//    }
  }

  /* cleanup curl stuff */
  curl_easy_cleanup(curl_handle);

  free(chunk.memory);

  /* we're done with libcurl, so clean it up */
  curl_global_cleanup();

  return mo;
}

void CcdbDatabase::disconnect()
{
  // NOOP for CCDB
}

void CcdbDatabase::prepareTaskDataContainer(std::string taskName)
{
  // NOOP for CCDB
}

size_t CurlWrite_CallbackFunc_StdString(void *contents, size_t size, size_t nmemb, std::string *s)
{
  size_t newLength = size * nmemb;
  size_t oldLength = s->size();
  try {
    s->resize(oldLength + newLength);
  }
  catch (std::bad_alloc &e) {
    cerr << "memory error when getting data from CCDB" << endl;
    return 0;
  }

  std::copy((char *) contents, (char *) contents + newLength, s->begin() + oldLength);
  return size * nmemb;
}

std::string CcdbDatabase::getListing(std::string subpath, std::string accept)
{
  CURL *curl;
  CURLcode res;
  string fullUrl = url + "/browse/" + subpath;
  std::string tempString;

  curl_global_init(CURL_GLOBAL_DEFAULT);

  curl = curl_easy_init();
  if (curl != nullptr) {

    curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &tempString);
//    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, accept.c_str());

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, (string("Accept: ") + accept).c_str());
    headers = curl_slist_append(headers, (string("Content-Type: ") + accept).c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Perform the request, res will get the return code
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
  }

  return tempString;
}

/// trim from start (in place)
/// https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
static inline void ltrim(std::string &s)
{
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
    return !std::isspace(ch);
  }));
}

/// trim from end (in place)
/// https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
static inline void rtrim(std::string &s)
{
  s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
    return !std::isspace(ch);
  }).base(), s.end());
}

std::vector<std::string> CcdbDatabase::getListOfTasksWithPublications()
{
  std::vector<string> result;

  // Get the listing from CCDB
  string listing = getListing();

  // Split the string we received, by line. Also trim it and remove empty lines.
  std::stringstream ss(listing);
  std::string line;
  while (std::getline(ss, line, '\n')) {
    ltrim(line);
    rtrim(line);
    if (line.length() > 0 && line != "Subfolders:") {
      result.push_back(line);
    }
  }

  return result;
}

std::vector<std::string> CcdbDatabase::getPublishedObjectNames(std::string taskName)
{
  std::vector<string> result;
  // we use the "index" string to know what objects are published.
  // get the information from the CCDB itself.
  core::MonitorObject *mo = retrieve(taskName, core::MonitorObject::SYSTEM_OBJECT_PUBLICATION_LIST);
  if (!mo) {
    cerr << "could not retrieve the objects lists for task " << taskName << endl;
    return result;
  }
  auto *indexString = dynamic_cast<TObjString *>(mo->getObject());
  if (!indexString) {
    cerr << "could not cast the objects lists for task " << taskName << " to a TObjString" << endl;
    return result;
  }
  string s = indexString->GetString().Data();
  boost::algorithm::split(result, s, boost::algorithm::is_any_of(","), boost::algorithm::token_compress_on);
  // sanitize : remove system objects object and empty objects
  result.erase(std::remove(result.begin(), result.end(), ""));
  result.erase(std::remove(result.begin(), result.end(), core::MonitorObject::SYSTEM_OBJECT_PUBLICATION_LIST));

  return result;
}

long CcdbDatabase::getFutureTimestamp(int secondsInFuture)
{
  std::chrono::seconds sec(secondsInFuture);
  auto future = std::chrono::system_clock::now() + sec;
  auto future_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(future);
  auto epoch = future_ms.time_since_epoch();
  auto value = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);
  return value.count();
}

long CcdbDatabase::getCurrentTimestamp()
{
  auto now = std::chrono::system_clock::now();
  auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
  auto epoch = now_ms.time_since_epoch();
  auto value = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);
  return value.count();
}

std::string CcdbDatabase::getTimestampString(long timestamp)
{
  stringstream ss;
  ss << timestamp;
  return ss.str();
}

void CcdbDatabase::deleteObjectVersion(std::string taskName, std::string objectName, string timestamp)
{
  CURL *curl;
  CURLcode res;
  stringstream fullUrl;
  fullUrl << url << "/" << taskName << "/" << objectName << "/" << timestamp;

  curl_global_init(CURL_GLOBAL_DEFAULT);

  curl = curl_easy_init();
  if (curl != nullptr) {
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(curl, CURLOPT_URL, fullUrl.str().c_str());

    // Perform the request, res will get the return code
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }
    curl_easy_cleanup(curl);
  }
}

void CcdbDatabase::deleteObject(std::string taskName, std::string objectName)
{
  cout << "truncating data for " << taskName << "/" << objectName << endl;

  CURL *curl;
  CURLcode res;
  stringstream fullUrl;
  fullUrl << url << "/truncate/" << taskName << "/" << objectName;

  curl_global_init(CURL_GLOBAL_DEFAULT);

  curl = curl_easy_init();
  if (curl != nullptr) {
    curl_easy_setopt(curl, CURLOPT_URL, fullUrl.str().c_str());

    // Perform the request, res will get the return code
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }
    curl_easy_cleanup(curl);
  }
}

}
}
}
