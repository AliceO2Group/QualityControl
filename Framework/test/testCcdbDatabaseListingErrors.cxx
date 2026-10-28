// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   testCcdbDatabaseListingErrors.cxx
/// \author Barthelemy von Haller
///
/// Reproduces issues with CCDB

#include "QualityControl/CcdbDatabase.h"

#define BOOST_TEST_MODULE CcdbDatabase listing errors test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <boost/property_tree/ptree.hpp>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <string>
#include <thread>
#include <vector>

using namespace o2::quality_control::repository;

namespace
{

constexpr const char* kServiceUnavailableHtml =
  "<!doctype html><html lang=\"en\"><head><title>HTTP Status 503 – Service Unavailable</title></head>"
  "<body><h1>HTTP Status 503 – Service Unavailable</h1>"
  "<p><b>Message</b> Too many concurrent requests to /browse/ or /latest/ , cannot serve more</p>"
  "<h3>Apache Tomcat/9.0.97</h3></body></html>";

// Junie generated
class Html503Server
{
 public:
  Html503Server()
  {
    mListenFd = ::socket(AF_INET, SOCK_STREAM, 0);

    int yes = 1;
    ::setsockopt(mListenFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(0);

    socklen_t len = sizeof(addr);
    mPort = ntohs(addr.sin_port);

    mRunning = true;
    mThread = std::thread([this] { run(); });
  }

  ~Html503Server()
  {
    mRunning = false;
    if (mListenFd >= 0) {
      ::shutdown(mListenFd, SHUT_RDWR);
      ::close(mListenFd);
      mListenFd = -1;
    }
    if (mThread.joinable()) {
      mThread.join();
    }
  }

  std::string url() const { return "http://127.0.0.1:" + std::to_string(mPort); }

 private:
  void run()
  {
    while (mRunning) {
      sockaddr_in client{};
      socklen_t clientLen = sizeof(client);
      int clientFd = ::accept(mListenFd, reinterpret_cast<sockaddr*>(&client), &clientLen);
      if (clientFd < 0) {
        continue;
      }
      char buf[1024];
      ::recv(clientFd, buf, sizeof(buf), 0);

      const std::string body = kServiceUnavailableHtml;
      std::string response =
        "HTTP/1.1 503 Service Unavailable\r\n"
        "Content-Type: text/html;charset=utf-8\r\n"
        "Content-Length: " +
        std::to_string(body.size()) +
        "\r\n"
        "Connection: close\r\n"
        "\r\n" +
        body;
      ::send(clientFd, response.data(), response.size(), 0);
      ::close(clientFd);
    }
  }

  int mListenFd = -1;
  int mPort = 0;
  std::atomic<bool> mRunning{ false };
  std::thread mThread;
};

} // namespace

BOOST_AUTO_TEST_CASE(getPublishedObjectNames_handles_html_503)
{
  Html503Server server;
  CcdbDatabase db;
  db.connect(server.url(), "", "", "");

  std::vector<std::string> names;
  BOOST_CHECK_NO_THROW(names = db.getPublishedObjectNames("qc/TST/MO/SomeTask"));
  BOOST_TEST(names.empty());
}

BOOST_AUTO_TEST_CASE(getListingAsPtree_handles_html_503)
{
  Html503Server server;
  CcdbDatabase db;
  db.connect(server.url(), "", "", "");

  boost::property_tree::ptree listing;
  BOOST_CHECK_NO_THROW(listing = db.getListingAsPtree("qc/TST/MO/SomeTask"));
  BOOST_TEST(listing.count("objects") == 0);
}
