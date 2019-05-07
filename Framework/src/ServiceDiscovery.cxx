///
/// \author Adam Wegrzynek <adam.wegrzynek@cern.ch>
///

#include "ServiceDiscovery.h"
#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/asio/ip/host_name.hpp>
#include <boost/algorithm/string/split.hpp>       
#include <boost/algorithm/string.hpp> 

namespace o2::quality_control::core
{

ServiceDiscovery::ServiceDiscovery(const std::string& url, const std::string& id) :
  curlHandle(initCurl(), &ServiceDiscovery::deleteCurl), consulUrl(url), id(id)
{
}

ServiceDiscovery::~ServiceDiscovery()
{
  mThreadRunning = false;
  healthThread.join();
  deregister();
}

CURL* ServiceDiscovery::initCurl()
{
  CURLcode globalInitResult = curl_global_init(CURL_GLOBAL_ALL);
  if (globalInitResult != CURLE_OK) {
    throw std::runtime_error(std::string("cURL init") + curl_easy_strerror(globalInitResult));
  }
  CURL *curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
  curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L);
  curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L);
  FILE *devnull = fopen("/dev/null", "w+");
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, devnull);
  return curl;
}

static inline std::string GetDefaultUrl()
{
  return boost::asio::ip::host_name() + ":" + std::to_string(7777);
}

void ServiceDiscovery::_register(const std::string& objects, const std::string& healthUrl = GetDefaultUrl())
{
   std::vector<std::string> objectsVec;
   boost::split(objectsVec, objects, boost::is_any_of(","), boost::token_compress_on);
   boost::property_tree::ptree tag, tags;
   for (auto& object : objectsVec) {
     tag.put("", object);
     tags.push_back(std::make_pair("", tag));
   }

   boost::property_tree::ptree checks, check;
   check.put("Id", id);
   check.put("Name", "Health check " + id);
   check.put("Interval", "30s");
   check.put("TCP", healthUrl);
   checks.push_back(std::make_pair("", check));

   boost::property_tree::ptree pt;
   pt.put("Name", id);
   pt.put("ID", id);
   pt.add_child("Tags", tags);   
   pt.add_child("Checks", checks);  
   
   std::stringstream ss;
   boost::property_tree::json_parser::write_json(ss, pt);
   
   healthThread =  std::thread([=] { runHealthServer(std::stoi(healthUrl.substr(healthUrl.find(":") + 1))); });
   
   send("/v1/agent/service/register", ss.str());
}

void ServiceDiscovery::deregister()
{
  send("/v1/agent/service/deregister/" + id, "");
}

void ServiceDiscovery::runHealthServer(unsigned int port)
{
  using boost::asio::ip::tcp;
  mThreadRunning = true;
  try {
    boost::asio::io_service io_service;
    tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), port));
    for (;;) {
      if (!mThreadRunning) break;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      tcp::socket socket(io_service);
      acceptor.accept(socket);
    }
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }
}

void ServiceDiscovery::deleteCurl(CURL * curl)
{
  curl_easy_cleanup(curl);
  curl_global_cleanup();
}

void ServiceDiscovery::send(const std::string& path, std::string&& post)
{
  std::string uri = consulUrl + path;
  CURLcode response;
  long responseCode;
  CURL *curl = curlHandle.get();
  struct curl_slist* headers = curl_slist_append(headers, "Content-Type: application/json");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_URL, uri.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post.c_str());
  response = curl_easy_perform(curl);
  curl_slist_free_all(headers);
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
  if (response != CURLE_OK) {
    std::cerr << curl_easy_strerror(response) << std::endl;
  }
  if (responseCode < 200 || responseCode > 206) {
    std::cerr << "Response code : " << responseCode << std::endl;
  }
}
}
