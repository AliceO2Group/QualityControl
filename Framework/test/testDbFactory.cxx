///
/// \file   Publisher_test.cpp
/// \author Barthelemy von Haller
///

#include "../include/QualityControl/DatabaseFactory.h"

#ifdef _WITH_MYSQL

#include "QualityControl/MySqlDatabase.h"

#endif
#define BOOST_TEST_MODULE Quality test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <cassert>
#include <iostream>
#include "curl/curl.h"

#include <stdio.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <QualityControl/CcdbDatabase.h>
#include <QualityControl/MonitorObject.h>
#include <TH1F.h>

using namespace std;
using namespace o2::quality_control::core;

namespace o2 {
namespace quality_control {
namespace repository {

bool do_nothing(AliceO2::Common::FatalException const &ex)
{ return true; }

BOOST_AUTO_TEST_CASE(db_factory_test)
{
#ifdef _WITH_MYSQL
  std::unique_ptr<DatabaseInterface> database = DatabaseFactory::create("MySql");
  BOOST_CHECK(database);
  BOOST_CHECK(dynamic_cast<MySqlDatabase*>(database.get()));
#endif

  std::unique_ptr<DatabaseInterface> database2 = nullptr;
  BOOST_CHECK_EXCEPTION(database2 = DatabaseFactory::create("asf"), AliceO2::Common::FatalException, do_nothing );
  BOOST_CHECK(!database2);

  std::unique_ptr<DatabaseInterface> database3 = DatabaseFactory::create("CCDB");
  BOOST_CHECK(database3);
  BOOST_CHECK(dynamic_cast<CcdbDatabase*>(database3.get()));
}

BOOST_AUTO_TEST_CASE(db_ccdb_listing)
{
  std::unique_ptr<DatabaseInterface> database3 = DatabaseFactory::create("CCDB");
  BOOST_CHECK(database3);
  auto *ccdb = dynamic_cast<CcdbDatabase*>(database3.get());
  BOOST_CHECK(ccdb);

  ccdb->connect("ccdb-test.cern.ch:8080", "", "", "");

  // prepare stuff in the db
  ccdb->truncate("functional_test", "object1");
  ccdb->truncate("functional_test", "object2");
  ccdb->truncate("functional_test", "path/to/object3");
  auto *h1 = new TH1F("object1", "object1", 100, 0, 99);
  auto *h2 = new TH1F("object2", "object2", 100, 0, 99);
  auto *h3 = new TH1F("object3", "object3", 100, 0, 99);
  shared_ptr<MonitorObject> mo1 = make_shared<MonitorObject>("object1", h1, "functional_test");
  shared_ptr<MonitorObject> mo2 = make_shared<MonitorObject>("object2", h2, "functional_test");
  shared_ptr<MonitorObject> mo3 = make_shared<MonitorObject>("path/to/object3", h3, "functional_test");
  ccdb->store(mo1);
  ccdb->store(mo2);
  ccdb->store(mo3);

  // test getting list of tasks
  std::vector<std::string> list = ccdb->getListOfTasksWithPublications();
//  for(const auto &item : list) {
//    cout << "task : " << item << endl;
//  }
  BOOST_CHECK(std::find(list.begin(), list.end(), "functional_test") != list.end());

  // test getting objects list from task
  auto objectNames = ccdb->getPublishedObjectNames("functional_test");
//  cout << "objects in task functional_test" << endl;
//  for (auto name : objectNames) {
//    cout << " - object : " << name << endl;
//  }
  BOOST_CHECK(std::find(objectNames.begin(), objectNames.end(), "object1") != objectNames.end());
  BOOST_CHECK(std::find(objectNames.begin(), objectNames.end(), "object2") != objectNames.end());
  BOOST_CHECK(std::find(objectNames.begin(), objectNames.end(), "path/to/object3") != objectNames.end());

  // test retrieve object
  MonitorObject* mo1_retrieved = ccdb->retrieve("functional_test", "object1");
  BOOST_CHECK(mo1_retrieved != nullptr);

}

/*
BOOST_AUTO_TEST_CASE(test_libcurl)
{
  CURL* easyhandle= curl_easy_init();

//  TMessage message(kMESS_OBJECT);
//  message.Reset();
//  message.WriteObjectAny(mo, mo->IsA());
  string url = "localhost:8080";
  char local_buffer[1024]="data to send";
  string fullUrl =  url + "/taskname/moname/1/100000";
  //"/quality=" + to_string(mo->getQuality().getLevel());
  cout << "fullUrl : " << fullUrl << endl;
  curl_easy_setopt(easyhandle, CURLOPT_URL,
                   fullUrl.c_str());

//  curl_easy_setopt(easyhandle, CURLOPT_READFUNCTION, read_function);
//   curl_easy_setopt(easyhandle, CURLOPT_READDATA, &filedata);
  curl_easy_setopt(easyhandle, CURLOPT_UPLOAD, 1L);
//  curl_easy_setopt(easyhandle, CURLOPT_INFILESIZE_LARGE, message.Length());
  // size of the POST data //
//  curl_easy_setopt(easyhandle, CURLOPT_POSTFIELDSIZE, message.Length());
  curl_easy_setopt(easyhandle, CURLOPT_POSTFIELDSIZE, 12L);
  // pass in a pointer to the data - libcurl will copy //
  curl_easy_setopt(easyhandle, CURLOPT_COPYPOSTFIELDS, local_buffer);
  // message.Buffer()); // we copy for sake of simplicity, would be better not to
  cout << "perform" << endl;
  CURLcode success = curl_easy_perform(easyhandle);
  cout << "success : " << success << endl;
  // Check for errors //
  if(success != CURLE_OK) {
    cerr << "curl_easy_perform() failed: " << curl_easy_strerror(success) << endl;
  } else {
    cout << "curl worked" << endl;
  }
}*/

/*
BOOST_AUTO_TEST_CASE(test_libcurlsimple)
{
  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, "localhost:8080");
    *//* example.com is redirected, so we tell libcurl to follow redirection *//*
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    *//* Perform the request, res will get the return code *//*
    res = curl_easy_perform(curl);
    *//* Check for errors *//*
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));

    *//* always cleanup *//*
    curl_easy_cleanup(curl);
  }
}*/
/*
BOOST_AUTO_TEST_CASE(test_libculrsimplepost)
{
  CURL *curl;
  CURLcode res;

  static const char *postthis = "moo mooo moo moo";
  string fullUrl =  "localhost:8080/taskname/moname/1/100000";

  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postthis);

    *//* if we don't provide POSTFIELDSIZE, libcurl will strlen() by
       itself *//*
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(postthis));

    *//* Perform the request, res will get the return code *//*
    res = curl_easy_perform(curl);
    *//* Check for errors *//*
    if(res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
    }

    *//* always cleanup *//*
    curl_easy_cleanup(curl);
  }
}*/
//
//BOOST_AUTO_TEST_CASE(test_upload)
//{
//  CURL *curl;
//  CURLcode res;
//  struct stat file_info;
//  double speed_upload, total_time;
//  FILE *fd;
//  string fullUrl =  "localhost:8080/taskname/moname/1/100000";
//
//  fd = fopen("/tmp/test.txt", "rb"); /* open file to upload */
//  if(!fd) {
//    BOOST_CHECK(fd);
//    return;
//  }
//
//  /* to get the file size */
//  if(fstat(fileno(fd), &file_info) != 0) {
//    BOOST_CHECK(false); /* can't continue */
//    return;
//  }
//
//  curl = curl_easy_init();
//  if(curl) {
//    /* upload to this place */
//    curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
////    curl_easy_setopt(curl, CURLOPT_URL, "file:///tmp/test2.txt");
//
//    /* tell it to "upload" to the URL */
//    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
//
//    /* set where to read from (on Windows you need to use READFUNCTION too) */
//    curl_easy_setopt(curl, CURLOPT_READDATA, fd);
//
//    /* and give the size of the upload (optional) */
//    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
//                     (curl_off_t)file_info.st_size);
//
//    /* enable verbose for easier tracing */
//    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
//
//    res = curl_easy_perform(curl);
//    /* Check for errors */
//    if(res != CURLE_OK) {
//      fprintf(stderr, "curl_easy_perform() failed: %s\n",
//              curl_easy_strerror(res));
//
//    }
//    else {
//      /* now extract transfer info */
//      curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD, &speed_upload);
//      curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total_time);
//
//      fprintf(stderr, "Speed: %.3f bytes/sec during %.3f seconds\n",
//              speed_upload, total_time);
//
//    }
//    /* always cleanup */
//    curl_easy_cleanup(curl);
//  }
//  fclose(fd);
//}

//
///* silly test data to POST */
//static const char data[]="Lorem ipsum dolor sit amet, consectetur adipiscing "
//  "elit. Sed vel urna neque. Ut quis leo metus. Quisque eleifend, ex at "
//  "laoreet rhoncus, odio ipsum semper metus, at tempus ante urna in mauris. "
//  "Suspendisse ornare tempor venenatis. Ut dui neque, pellentesque a varius "
//  "eget, mattis vitae ligula. Fusce ut pharetra est. Ut ullamcorper mi ac "
//  "sollicitudin semper. Praesent sit amet tellus varius, posuere nulla non, "
//  "rhoncus ipsum.";
//
//struct WriteThis {
//    const char *readptr;
//    size_t sizeleft;
//};
//
//static size_t read_callback(void *dest, size_t size, size_t nmemb, void *userp)
//{
//  struct WriteThis *wt = (struct WriteThis *)userp;
//  size_t buffer_size = size*nmemb;
//
//  if(wt->sizeleft) {
//    /* copy as much as possible from the source to the destination */
//    size_t copy_this_much = wt->sizeleft;
//    if(copy_this_much > buffer_size)
//      copy_this_much = buffer_size;
//    memcpy(dest, wt->readptr, copy_this_much);
//
//    wt->readptr += copy_this_much;
//    wt->sizeleft -= copy_this_much;
//    return copy_this_much; /* we copied this many bytes */
//  }
//
//  return 0; /* no more data left to deliver */
//}
//
//BOOST_AUTO_TEST_CASE(test_curl_case){
//  CURL *curl;
//  CURLcode res;
//
//  struct WriteThis wt;
//
//  wt.readptr = data;
//  wt.sizeleft = strlen(data);
//
//  /* get a curl handle */
//  curl = curl_easy_init();
//  if (curl) {
//    /* First set the URL that is about to receive our POST. */
//      string fullUrl =  "localhost:8080/taskname/moname/1/100000";
//    curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());//"https://example.com/index.cgi");
//
//    /* Now specify we want to POST data */
//    curl_easy_setopt(curl, CURLOPT_POST, 1L);
//
//    /* we want to use our own read function */
//    curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
//
//    /* pointer to pass to our read function */
//    curl_easy_setopt(curl, CURLOPT_READDATA, &wt);
//
//    /* get verbose debug output please */
//    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
//
//    /* Set the expected POST size. If you want to POST large amounts of data,
//       consider CURLOPT_POSTFIELDSIZE_LARGE */
//    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long) wt.sizeleft);
//
//    /* Perform the request, res will get the return code */
//    res = curl_easy_perform(curl);
//    /* Check for errors */
//    if (res != CURLE_OK)
//      fprintf(stderr, "curl_easy_perform() failed: %s\n",
//              curl_easy_strerror(res));
//
//    /* always cleanup */
//    curl_easy_cleanup(curl);
//  }
//  curl_global_cleanup();
//}
//
//static const char data[]="Lorem ipsum dolor sit amet, consectetur adipiscing "
//  "elit. Sed vel urna neque. Ut quis leo metus. Quisque eleifend, ex at "
//  "laoreet rhoncus, odio ipsum semper metus, at tempus ante urna in mauris. "
//  "Suspendisse ornare tempor venenatis. Ut dui neque, pellentesque a varius "
//  "eget, mattis vitae ligula. Fusce ut pharetra est. Ut ullamcorper mi ac "
//  "sollicitudin semper. Praesent sit amet tellus varius, posuere nulla non, "
//  "rhoncus ipsum.";
//
//BOOST_AUTO_TEST_CASE(test_curl_multiform)
//{
//
//  CURL *curl;
//
//  CURLM *multi_handle;
//  int still_running;
//
//  struct curl_httppost *formpost = NULL;
//  struct curl_httppost *lastptr = NULL;
//  struct curl_slist *headerlist = NULL;
//  static const char buf[] = "Expect:";
//  string fullUrl = "localhost:8080/taskname/moname/1/100000";
//
//  /* Fill in the file upload field. This makes libcurl load data from
//     the given file name when curl_easy_perform() is called. */
////  curl_formadd(&formpost,
////               &lastptr,
////               CURLFORM_COPYNAME, "sendfile",
////               CURLFORM_FILE, "/tmp/test.txt",
////               CURLFORM_END);
////
////  /* Fill in the filename field */
////  curl_formadd(&formpost,
////               &lastptr,
////               CURLFORM_COPYNAME, "filename",
////               CURLFORM_COPYCONTENTS, "test.txt",
////               CURLFORM_END);
////
////  /* Fill in the submit field too, even if this is rarely needed */
////  curl_formadd(&formpost,
////               &lastptr,
////               CURLFORM_COPYNAME, "submit",
////               CURLFORM_COPYCONTENTS, "send",
////               CURLFORM_END);
//
//  curl_formadd(&formpost,
//               &lastptr,
//               CURLFORM_COPYNAME, "send",
//               CURLFORM_BUFFER, "test.txt",
//               CURLFORM_BUFFERPTR, data,
//               CURLFORM_BUFFERLENGTH, strlen(data),
//               CURLFORM_END);
//
//  curl = curl_easy_init();
//  multi_handle = curl_multi_init();
//
//  /* initialize custom header list (stating that Expect: 100-continue is not
//     wanted */
//  headerlist = curl_slist_append(headerlist, buf);
//  if (curl && multi_handle) {
//
//    /* what URL that receives this POST */
//    curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
//    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
//
//    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
//    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
//
//    curl_multi_add_handle(multi_handle, curl);
//
//    curl_multi_perform(multi_handle, &still_running);
//
//    do {
//      struct timeval timeout;
//      int rc; /* select() return code */
//      CURLMcode mc; /* curl_multi_fdset() return code */
//
//      fd_set fdread;
//      fd_set fdwrite;
//      fd_set fdexcep;
//      int maxfd = -1;
//
//      long curl_timeo = -1;
//
//      FD_ZERO(&fdread);
//      FD_ZERO(&fdwrite);
//      FD_ZERO(&fdexcep);
//
//      /* set a suitable timeout to play around with */
//      timeout.tv_sec = 1;
//      timeout.tv_usec = 0;
//
//      curl_multi_timeout(multi_handle, &curl_timeo);
//      if (curl_timeo >= 0) {
//        timeout.tv_sec = curl_timeo / 1000;
//        if (timeout.tv_sec > 1) {
//          timeout.tv_sec = 1;
//        } else {
//          timeout.tv_usec = (curl_timeo % 1000) * 1000;
//        }
//      }
//
//      /* get file descriptors from the transfers */
//      mc = curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);
//
//      if (mc != CURLM_OK) {
//        fprintf(stderr, "curl_multi_fdset() failed, code %d.\n", mc);
//        break;
//      }
//
//      /* On success the value of maxfd is guaranteed to be >= -1. We call
//         select(maxfd + 1, ...); specially in case of (maxfd == -1) there are
//         no fds ready yet so we call select(0, ...) --or Sleep() on Windows--
//         to sleep 100ms, which is the minimum suggested value in the
//         curl_multi_fdset() doc. */
//
//      if (maxfd == -1) {
//        /* Portable sleep for platforms other than Windows. */
//        struct timeval wait = {0, 100 * 1000}; /* 100ms */
//        rc = select(0, NULL, NULL, NULL, &wait);
//      } else {
//        /* Note that on some platforms 'timeout' may be modified by select().
//           If you need access to the original value save a copy beforehand. */
//        rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
//      }
//
//      switch (rc) {
//        case -1:
//          /* select error */
//          break;
//        case 0:
//        default:
//          /* timeout or readable/writable sockets */
//          printf("perform!\n");
//          curl_multi_perform(multi_handle, &still_running);
//          printf("running: %d!\n", still_running);
//          break;
//      }
//    }
//    while (still_running);
//
//    curl_multi_cleanup(multi_handle);
//
//    /* always cleanup */
//    curl_easy_cleanup(curl);
//
//    /* then cleanup the formpost chain */
//    curl_formfree(formpost);
//
//    /* free slist */
//    curl_slist_free_all(headerlist);
//  }
//}
//
//struct MemoryStruct {
//  char *memory;
//  size_t size;
//};
//
//static size_t
//WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
//{
//  size_t realsize = size * nmemb;
//  struct MemoryStruct *mem = (struct MemoryStruct *)userp;
//
//  mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);
//  if(mem->memory == NULL) {
//    /* out of memory! */
//    printf("not enough memory (realloc returned NULL)\n");
//    return 0;
//  }
//
//  memcpy(&(mem->memory[mem->size]), contents, realsize);
//  mem->size += realsize;
//  mem->memory[mem->size] = 0;
//
//  return realsize;
//}
//
//BOOST_AUTO_TEST_CASE(curl_test_get)
//{
//  CURL *curl_handle;
//  CURLcode res;
//
//  struct MemoryStruct chunk;
//
//  chunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */
//  chunk.size = 0;    /* no data at this point */
//
//  curl_global_init(CURL_GLOBAL_ALL);
//
//  /* init the curl session */
//  curl_handle = curl_easy_init();
//
//  /* specify URL to get */
//  curl_easy_setopt(curl_handle, CURLOPT_URL, "http://www.example.com/");
//
//  /* send all data to this function  */
//  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
//
//  /* we pass our 'chunk' struct to the callback function */
//  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
//
//  /* some servers don't like requests that are made without a user-agent
//     field, so we provide one */
//  curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
//
//  /* get it! */
//  res = curl_easy_perform(curl_handle);
//
//  /* check for errors */
//  if(res != CURLE_OK) {
//    fprintf(stderr, "curl_easy_perform() failed: %s\n",
//            curl_easy_strerror(res));
//  }
//  else {
//    /*
//     * Now, our chunk.memory points to a memory block that is chunk.size
//     * bytes big and contains the remote file.
//     *
//     * Do something nice with it!
//     */
//
//    printf("%lu bytes retrieved\n", (long)chunk.size);
//  }
//
//  /* cleanup curl stuff */
//  curl_easy_cleanup(curl_handle);
//
//  free(chunk.memory);
//
//  /* we're done with libcurl, so clean it up */
//  curl_global_cleanup();
//}
//
//BOOST_AUTO_TEST_CASE(test_curl_get_reloc)
//{
//  CURL *curl;
//  CURLcode res;
//  char *location;
//  long response_code;
//
//  curl = curl_easy_init();
//  if(curl) {
//    curl_easy_setopt(curl, CURLOPT_URL, "localhost:8080/daqTask/IDs/1");
//
//    /* example.com is redirected, figure out the redirection! */
//
//    /* Perform the request, res will get the return code */
//    res = curl_easy_perform(curl);
//    /* Check for errors */
//    if(res != CURLE_OK)
//      fprintf(stderr, "curl_easy_perform() failed: %s\n",
//              curl_easy_strerror(res));
//    else {
//      res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
//      if((res == CURLE_OK) &&
//         ((response_code / 100) != 3)) {
//        /* a redirect implies a 3xx response code */
//        fprintf(stderr, "Not a redirect.\n");
//      }
//      else {
//        res = curl_easy_getinfo(curl, CURLINFO_REDIRECT_URL, &location);
//
//        if((res == CURLE_OK) && location) {
//          /* This is the new absolute URL that you could redirect to, even if
//           * the Location: response header may have been a relative URL. */
//          printf("Redirected to: %s\n", location);
//        }
//      }
//    }
//
//    /* always cleanup */
//    curl_easy_cleanup(curl);
//  }
//}




} /* namespace repository */
} /* namespace quality_control */
} /* namespace o2 */
