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
/// \author Adam Wegrzynek
///

#ifndef QC_SERVICEDISCOVERY_H
#define QC_SERVICEDISCOVERY_H

#include <atomic>
#include <curl/curl.h>
#include <memory>
#include <string>
#include <thread>
#include <boost/asio/ip/host_name.hpp>
#include <random>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>
#include "QualityControl/QcInfoLogger.h"

namespace o2::quality_control::core
{

/// \brief Information service for QC
///
/// Register a endpoint to Consul which then performs health checks it
/// Allow to publish list of online objects
class ServiceDiscovery
{
 public:
  /// Sets up CURL and health check
  /// \param url 		Consul URL
  /// \param name		Service name
  /// \param id 		Unique instance ID
  /// \param healthEndpoint	Local endpoint that is then used for health checks
  ///				(default value it set to  <hostname>:7777)
  ServiceDiscovery(const std::string& url, const std::string& name, const std::string& id, const std::string& healthEndUrl = GetDefaultUrl());

  /// Stops the health thread and deregisteres from Consul health checks
  ~ServiceDiscovery();

  /// Registers list of online objects by sending HTTP PUT request to Consul server
  /// \param objects 		List of comma separated objects
  void _register(const std::string& objects);

  /// Deregisters service
  void deregister();

  // https://stackoverflow.com/questions/33358321/using-c-and-boost-or-not-to-check-if-a-specific-port-is-being-used
  static inline bool PortInUse(unsigned short port)
  {
    using namespace boost::asio;
    using ip::tcp;

    boost::asio::io_service svc;
    tcp::acceptor a(svc);

    boost::system::error_code ec;
    a.open(tcp::v4(), ec) || a.bind({ tcp::v4(), port }, ec);

    return ec == error::address_in_use;
  }

  static inline std::string GetDefaultUrl() ///< Provides default health check URL
  {
    return GetDefaultUrl(GetHealthPort());
  }

  /// Find a free port in the range [HealthPortRangeEnd;HealthPortRangeStart] if any available.
  static inline size_t GetHealthPort()
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

  static inline std::string GetDefaultUrl(size_t port) ///< Provides default health check URL
  {
    return boost::asio::ip::host_name() + ":" + std::to_string(port);
  }

  static constexpr size_t HealthPortRangeStart = 47800; ///< Health check port range start
  static constexpr size_t HealthPortRangeEnd = 47899;   ///< Health check port range end

 private:
  /// Custom deleter of CURL object
  static void deleteCurl(CURL* curl);

  /// CURL instance
  std::unique_ptr<CURL, decltype(&ServiceDiscovery::deleteCurl)> curlHandle;

  const std::string mConsulUrl;     ///< Consul URL
  const std::string mName;          ///< Instance (service) Name
  const std::string mId;            ///< Instance (service) ID
  std::string mHealthUrl;           ///< hostname and port of health check endpoint
  std::thread mHealthThread;        ///< Health check thread
  std::atomic<bool> mThreadRunning; ///< Health check thread running flag

  /// Initializes CURL
  CURL* initCurl();

  /// Sends PUT request
  void send(const std::string& path, std::string&& request);

  /// Health check thread loop
  void runHealthServer(unsigned int port);
};

} // namespace o2::quality_control::core
#endif // QC_SERVICEDISCOVERY_H
