//
// Created by bvonhall on 17/10/17.
//

#include <TMessage.h>
#include <TObjString.h>
#include "QualityControl/CcdbDatabase.h"
#include <regex>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

namespace o2 {
namespace quality_control {
namespace repository {

using namespace std;

CcdbDatabase::CcdbDatabase()
{
}

void CcdbDatabase::connect(std::string host, std::string database, std::string username, std::string password)
{
  url = host;
}

void CcdbDatabase::store(o2::quality_control::core::MonitorObject *mo)
{
  TMessage message(kMESS_OBJECT);
  message.Reset();
  message.WriteObjectAny(mo, mo->IsA());
  string fullUrl = url + "/" + mo->getTaskName() + "/" + mo->getName() + "/1/100000";
  CURL *curl;

  CURLM *multi_handle;
  int still_running;

  struct curl_httppost *formpost = nullptr;
  struct curl_httppost *lastptr = nullptr;
  struct curl_slist *headerlist = nullptr;
  static const char buf[] = "Expect:";

  /* data from buffer in memory */
  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "send",
               CURLFORM_BUFFER, "test.txt",
               CURLFORM_BUFFERPTR, message.Buffer(),
               CURLFORM_BUFFERLENGTH, message.Length(),
               CURLFORM_END);

  curl = curl_easy_init();
  multi_handle = curl_multi_init();

  /* initialize custom header list (stating that Expect: 100-continue is not
     wanted */
  headerlist = curl_slist_append(headerlist, buf);
  if (curl && multi_handle) {

    /* what URL that receives this POST */
    curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
//    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

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
//          printf("perform!\n");
          curl_multi_perform(multi_handle, &still_running);
//          printf("running: %d!\n", still_running);
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

core::MonitorObject *CcdbDatabase::retrieve(std::string taskName, std::string objectName)
{
// first get the redirection
  string location = getObjectPath(taskName, objectName);//"/daqTask/IDs/1/5ace77e0-b3e6-11e7-be2d-7f0000015566";
  // then get the object
  return downloadObject(location);
}

struct MemoryStruct
{
    char *memory;
    unsigned int size;
};

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
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

core::MonitorObject *CcdbDatabase::downloadObject(std::string location)
{
  CURL *curl_handle;
  CURLcode res;

  struct MemoryStruct chunk{(char *) malloc(1)/*memory*/, 0/*size*/};
  o2::quality_control::core::MonitorObject *mo = nullptr;

  curl_global_init(CURL_GLOBAL_ALL);

  /* init the curl session */
  curl_handle = curl_easy_init();

  /* specify URL to get */
  curl_easy_setopt(curl_handle, CURLOPT_URL, location.c_str());

  /* send all data to this function  */
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

  /* we pass our 'chunk' struct to the callback function */
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *) &chunk);

  /* some servers don't like requests that are made without a user-agent
     field, so we provide one */
  curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

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
     *
     * Do something nice with it!
     */

//    printf("%lu bytes retrieved\n", (long) chunk.size);

//    TDatime updatetime(statement->GetYear(2), statement->GetMonth(2), statement->GetDay(2), statement->GetHour(2),
//                       statement->GetMinute(2), statement->GetSecond(2));
//    int run = statement->IsNull(3) ? -1 : statement->GetInt(3);
//    int fill = statement->IsNull(4) ? -1 : statement->GetInt(4);

    TMessage mess(kMESS_OBJECT);
    mess.SetBuffer(chunk.memory, chunk.size, kFALSE);
    mess.SetReadMode();
    mess.Reset();
    mo = (o2::quality_control::core::MonitorObject *) (mess.ReadObjectAny(mess.GetClass()));
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

}

void CcdbDatabase::prepareTaskDataContainer(std::string taskName)
{

}

std::vector<std::string> CcdbDatabase::getListOfTasksWithPublications()
{
  std::vector<string> result{"daqTask"}; // TODO we need the "ls" feature in CCDB to list the top level nodes.
  return result;
}

std::vector<std::string> CcdbDatabase::getPublishedObjectNames(std::string taskName)
{
  std::vector<string> result;
  // we use the "index" string to know what objects are published.
  // get the information from the CCDB itself.
  core::MonitorObject *mo = retrieve(taskName, core::MonitorObject::SYSTEM_OBJECT_PUBLICATION_LIST);
  auto *indexString = dynamic_cast<TObjString *>(mo->getObject());
  if (indexString) {
    string s = indexString->GetString().Data();
    boost::algorithm::split(result, s, boost::algorithm::is_any_of(","),boost::algorithm::token_compress_on);
    // sanitize : remove system objects object and empty objects
    result.erase(std::remove(result.begin(), result.end(), ""));
    result.erase(std::remove(result.begin(), result.end(), core::MonitorObject::SYSTEM_OBJECT_PUBLICATION_LIST));
  } else {
    cerr << "ok we should do something here. The 'index' object must be a TObjString" << endl;
  }

  return result;
}

std::string CcdbDatabase::getObjectPath(std::string taskName, std::string objectName)
{
  CURL *curl;
  CURLcode res;
  char *location;
  long response_code;
  string result;

  string fullUrl = url + "/" + taskName + "/" + objectName + "/1";

  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());

    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curl);
    /* Check for errors */
    if (res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
      // TODO do something
    } else {
      res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
      if ((res == CURLE_OK) && ((response_code / 100) != 3)) {
        /* a redirect implies a 3xx response code */
        fprintf(stderr, "Not a redirect.\n");
        // tODO do something, this is wrong
      } else {
        res = curl_easy_getinfo(curl, CURLINFO_REDIRECT_URL, &location);

        if ((res == CURLE_OK) && location) {
          /* This is the new absolute URL that you could redirect to, even if
           * the Location: response header may have been a relative URL. */
//          printf("Redirected to: %s\n", location);
          result = location;
        }
      }
    }

    /* always cleanup */
    curl_easy_cleanup(curl);
  }
  return result;
}

}
}
}