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
/// \author Adam Wegrzynek <adam.wegrzynek@cern.ch>
///

#include "QualityControl/ServiceDiscovery.h"
#include "QualityControl/QcInfoLogger.h"
#include <string>
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

namespace o2::quality_control::core
{

ServiceDiscovery::ServiceDiscovery(const std::string& url, const std::string& name, const std::string& id, const std::string& healthEndpoint)
  : curlHandle(initCurl(), &ServiceDiscovery::deleteCurl), mConsulUrl(url), mName(name), mId(id), mHealthEndpoint(healthEndpoint)
{
  // parameter check
  if (mHealthEndpoint.find(':') == std::string::npos) {
    mHealthEndpoint = GetDefaultUrl();
  }

  mHealthThread = std::thread([=] { runHealthServer(std::stoi(mHealthEndpoint.substr(mHealthEndpoint.find(":") + 1))); });
  _register("");
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

void ServiceDiscovery::_register(const std::string& objects)
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
  check.put("TCP", mHealthEndpoint);
  checks.push_back(std::make_pair("", check));

  pt.put("Name", mName);
  pt.put("ID", mId);
  pt.add_child("Checks", checks);

  std::stringstream ss;
  boost::property_tree::json_parser::write_json(ss, pt);

  send("/v1/agent/service/register", ss.str());
  ILOG(Info) << "Registration to ServiceDiscovery: " << objects << ENDM;
}

void ServiceDiscovery::deregister()
{
  send("/v1/agent/service/deregister/" + mId, "");
  ILOG(Info) << "Deregistration from ServiceDiscovery" << ENDM;
}

void ServiceDiscovery::runHealthServer(unsigned int port)
{
  using boost::asio::ip::tcp;
  mThreadRunning = true;
  try {
    boost::asio::io_service io_service;
    tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), port));
    boost::asio::deadline_timer timer(io_service);
    while (mThreadRunning) {
      io_service.reset();
      timer.expires_from_now(boost::posix_time::seconds(1));
      timer.async_wait([&](boost::system::error_code ec) {
        if (!ec)
          acceptor.cancel();
      });
      tcp::socket socket(io_service);
      acceptor.async_accept(socket, [&](boost::system::error_code ec) {
        if (!ec)
          timer.cancel();
      });
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  } catch (std::exception& e) {
    mThreadRunning = false;
    std::cerr << "ServiceDiscovery::runHealthServer - " << e.what() << std::endl;
  }
}

void ServiceDiscovery::deleteCurl(CURL* curl)
{
  curl_easy_cleanup(curl);
  curl_global_cleanup();
}

void ServiceDiscovery::send(const std::string& path, std::string&& post)
{
  std::string uri = mConsulUrl + path;
  CURLcode response;
  long responseCode;
  CURL* curl = curlHandle.get();
  curl_easy_setopt(curl, CURLOPT_URL, uri.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post.c_str());
  response = curl_easy_perform(curl);
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
  if (response != CURLE_OK) {
    ILOG(Error) << "ServiceDiscovery::send(...) " << curl_easy_strerror(response) << ENDM;
    ILOG(Error) << "   URI: " << uri << ENDM;
  }
  if (responseCode < 200 || responseCode > 206) {
    ILOG(Error) << "ServiceDiscovery::send(...) Response code: " << responseCode << ENDM;
  }
}
} // namespace o2::quality_control::core
