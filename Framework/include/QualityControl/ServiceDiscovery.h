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
  /// \param id 		Unique instance ID
  /// \param healthEndpoint	Local endpoint that is then used for health checks
  ///				(default value it set to  <hostname>:7777)
  ServiceDiscovery(const std::string& url, const std::string& id, const std::string& healthEndpoint = GetDefaultUrl());

  /// Stops the health thread and deregisteres from Consul health checks
  ~ServiceDiscovery();

  /// Registeres list of online objects by sending HTTP PUT request to Consul server
  /// \param objects 		List of comma separated objects
  void _register(const std::string& objects);

  /// Deregisteres service
  void deregister();

 private:
  /// Custom deleter of CURL object
  static void deleteCurl(CURL* curl);

  /// CURL instance
  std::unique_ptr<CURL, decltype(&ServiceDiscovery::deleteCurl)> curlHandle;

  const std::string mConsulUrl;     ///< Consul URL
  const std::string mId;            ///< Instance (service) ID
  std::string mHealthEndpoint;      ///< hostname and port of health check endpoint
  std::thread mHealthThread;        ///< Health check thread
  std::atomic<bool> mThreadRunning; ///< Health check thread running flag
  CURL* initCurl();                 ///< Initializes CURL

  /// Sends PUT request
  void send(const std::string& path, std::string&& request);

  /// Health check thread loop
  void runHealthServer(unsigned int port);

  static inline std::string GetDefaultUrl(); ///< Provides default health check URL
};

} // namespace o2::quality_control::core
#endif // QC_SERVICEDISCOVERY_H
