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
/// \author Adam Wegrzynek <adam.wegrzynek@cern.ch>
///

#include "QualityControl/ServiceDiscovery.h"
#include "QualityControl/QcInfoLogger.h"
#include <string>
#include <random>
#include <curl/curl.h>
#include <boost/asio/ip/host_name.hpp>
#include <boost/asio.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>

namespace o2::quality_control::core
{

ServiceDiscovery::ServiceDiscovery(const std::string& url, const std::string& name, const std::string& id, const std::string& healthEndUrl)
  : curlHandle(initCurl(), &ServiceDiscovery::deleteCurl), mConsulUrl(url), mName(name), mId(id), mHealthUrl(healthEndUrl)
{
  // parameter check
  if (mHealthUrl.find(':') == std::string::npos) {
    mHealthUrl = GetDefaultUrl();
  }
  if (_register("")) {
    mHealthThread = std::thread([=] { runHealthServer(std::stoi(mHealthUrl.substr(mHealthUrl.find(":") + 1))); });
  }
}

ServiceDiscovery::~ServiceDiscovery()
{
  mThreadRunning = false;
  if (mHealthThread.joinable()) {
    mHealthThread.join();
  }
  deregister();
}

CURL* ServiceDiscovery::initCurl()
{
  CURLcode globalInitResult = curl_global_init(CURL_GLOBAL_ALL);
  if (globalInitResult != CURLE_OK) {
    throw std::runtime_error(std::string("cURL init") + curl_easy_strerror(globalInitResult));
  }
  CURL* curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2);
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
  curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L);
  curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L);
  FILE* devnull = fopen("/dev/null", "w+");
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, devnull);
  return curl;
}

bool ServiceDiscovery::_register(const std::string& objects)
{
  boost::property_tree::ptree pt;
  if (!objects.empty()) {
    std::vector<std::string> objectsVec;
    boost::split(objectsVec, objects, boost::is_any_of(","), boost::token_compress_on);
    boost::property_tree::ptree tag, tags;
    for (auto& object : objectsVec) {
      tag.put("", object);
      tags.push_back(std::make_pair("", tag));
    }
    pt.add_child("Tags", tags);
  }

  boost::property_tree::ptree checks, check;
  check.put("Name", "Health check " + mId);
  check.put("Interval", "5s");
  check.put("DeregisterCriticalServiceAfter", "1m");
  check.put("TCP", mHealthUrl);
  checks.push_back(std::make_pair("", check));

  pt.put("Name", mName);
  pt.put("ID", mId);
  pt.add_child("Checks", checks);

  std::stringstream ss;
  boost::property_tree::json_parser::write_json(ss, pt);

  ILOG(Info, Devel) << "Registration to ServiceDiscovery: " << objects << ENDM;
  return send("/v1/agent/service/register", ss.str());
}

void ServiceDiscovery::deregister()
{
  send("/v1/agent/service/deregister/" + mId, "");
  ILOG(Info, Devel) << "Deregistration from ServiceDiscovery" << ENDM;
}

void ServiceDiscovery::runHealthServer(unsigned int port)
{
  using boost::asio::ip::tcp;
  mThreadRunning = true;

  // InfoLogger is not thread safe, we create a new instance for this thread.
  AliceO2::InfoLogger::InfoLogger threadInfoLogger;
  infoContext context;
  context.setField(infoContext::FieldName::Facility, "ServiceDiscovery");
  context.setField(infoContext::FieldName::System, "QC");
  threadInfoLogger.setContext(context);

  try {
    boost::asio::io_service io_service;
    tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), port));
    boost::asio::deadline_timer timer(io_service);
    while (mThreadRunning) {
      io_service.reset();
      timer.expires_from_now(boost::posix_time::seconds(1));
      tcp::socket socket(io_service);
      acceptor.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
      });
      timer.async_wait([&](boost::system::error_code ec) {
        io_service.stop();
      });
      io_service.run();
    }
  } catch (std::exception& e) {
    mThreadRunning = false;
    threadInfoLogger << AliceO2::InfoLogger::InfoLogger::Severity::Warning << "ServiceDiscovery::runHealthServer - " << e.what() << ENDM;
  }
}

void ServiceDiscovery::deleteCurl(CURL* curl)
{
  curl_easy_cleanup(curl);
  curl_global_cleanup();
}

bool ServiceDiscovery::send(const std::string& path, std::string&& post)
{
  std::string uri = mConsulUrl + path;
  CURLcode response;
  long responseCode;
  CURL* curl = curlHandle.get();
  curl_easy_setopt(curl, CURLOPT_URL, uri.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post.c_str());
  response = curl_easy_perform(curl);
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
  static AliceO2::InfoLogger::InfoLogger::AutoMuteToken msgLimit(LogWarningDevel, 1, 600); // send it only every 10 minutes
  if (response != CURLE_OK) {
    std::string s = std::string("ServiceDiscovery::send(...) ") + curl_easy_strerror(response) + "\n   URI: " + uri;
    ILOG_INST.log(msgLimit, "%s", s.c_str());
    return false;
  }
  if (responseCode < 200 || responseCode > 206) {
    std::string s = std::string("ServiceDiscovery::send(...) Response code: ") + std::to_string(responseCode);
    ILOG_INST.log(msgLimit, "%s", s.c_str());
    return false;
  }
  return true;
}

// https://stackoverflow.com/questions/33358321/using-c-and-boost-or-not-to-check-if-a-specific-port-is-being-used
bool ServiceDiscovery::PortInUse(unsigned short port)
{
  using ip::tcp;

  boost::asio::io_service svc;
  tcp::acceptor a(svc);

  boost::system::error_code ec;
  a.open(tcp::v4(), ec) || a.bind({ tcp::v4(), port }, ec);

  return ec == boost::asio::error::address_in_use;
}

size_t ServiceDiscovery::GetHealthPort()
{
  // inspired by https://stackoverflow.com/questions/7560114/random-number-c-in-some-range/7560151
  size_t port;
  std::random_device rd;  // obtain a random number from hardware
  std::mt19937 gen(rd()); // seed the generator
  size_t rangeLength = HealthPortRangeEnd - HealthPortRangeStart + 1;
  std::uniform_int_distribution<> distr(0, rangeLength - 1); // define the inclusive range

  size_t index = distr(gen); // get a random index in the range
  port = HealthPortRangeStart + index;
  size_t cycle = 1;                                // count how many ports we tried
  while (cycle < rangeLength && PortInUse(port)) { // if the port is in use and we did not go through the whole range
    index = (index + 1) % rangeLength;             // pick the next index
    port = HealthPortRangeStart + index;
    cycle++;
  }
  if (cycle == rangeLength) {
    ILOG(Error, Support) << "Could not find a free port for the ServiceDiscovery" << ENDM;
    // we keep the last port but all calls will fail.
  } else {
    ILOG(Debug, Devel) << "ServiceDiscovery selected port: " << port << ENDM;
  }
  return port;
}

std::string ServiceDiscovery::GetDefaultUrl(size_t port) ///< Provides default health check URL
{
  return boost::asio::ip::host_name() + ":" + std::to_string(port);
}

} // namespace o2::quality_control::core
