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
#include <memory>
#include <string>
#include <thread>

typedef void CURL;

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
  /// \param healthEndUrl	Local endpoint that is then used for health checks
  ServiceDiscovery(const std::string& url, const std::string& name, const std::string& id, const std::string& healthEndUrl = "");

  /// Stops the health thread and deregisteres from Consul health checks
  ~ServiceDiscovery();

  /// Registers list of online objects by sending HTTP PUT request to Consul server
  /// \param objects 		List of comma separated objects
  bool _register(const std::string& objects);

  /// Deregisters service
  void deregister();

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
  std::string mHealthUrl;           ///< hostname of health check endpoint
  size_t mHealthPort;               ///< Port of the health check endpoint
  std::thread mHealthThread;        ///< Health check thread
  std::atomic<bool> mThreadRunning; ///< Health check thread running flag

  /// Initializes CURL
  CURL* initCurl();

  /// Sends PUT request
  bool send(const std::string& path, std::string&& request);

  /// Health check thread loop + port computation
  void runHealthServer();
};

} // namespace o2::quality_control::core
#endif // QC_SERVICEDISCOVERY_H
