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
class ServiceDiscovery
{
  public:
    /// Sets up CURL
    ServiceDiscovery(const std::string& url, const std::string& id, const std::string& healthEndpoint);

    /// Stops the health thread and deregisteres from Consul health checks
    ~ServiceDiscovery();

    /// Registeres service and health check by sending HTTP PUT request to Consul server
    /// Provides list of published tasks as Consul tags
    void _register(const std::string& objects);

    /// Deregisters service
    void deregister();
  private:
    /// Custom deleter of CURL object
    static void deleteCurl(CURL * curl);

    /// CURL instance
    std::unique_ptr<CURL, decltype(&ServiceDiscovery::deleteCurl)> curlHandle;

    const std::string mConsulUrl; ///< Consul URL
    const std::string mId; ///< Instance (service) ID
    const std::string mHealthEndpoint; ///< hostname and port of health check endpoint
    std::thread mHealthThread; ///< Health check thread
    std::atomic<bool> mThreadRunning; ///< Health check thread running flag
    CURL* initCurl(); ///< Initializes CURL

    /// Sends PUT request
    void send(const std::string &path, std::string&& request);

    /// Health check thread loop
    void runHealthServer(unsigned int port);
};

}
#endif // QC_SERVICEDISCOVERY_H
