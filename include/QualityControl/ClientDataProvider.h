///
/// \file   ClientDataProvider.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_CLIENT_CLIENT_DATA_PROVIDER_H_
#define QUALITYCONTROL_CLIENT_CLIENT_DATA_PROVIDER_H_

#include "QualityControl/MonitorObject.h"
#include "QualityControl/DatabaseInterface.h"

using namespace AliceO2::QualityControl;

namespace AliceO2 {
namespace QualityControl {
namespace Client {

/// \brief Class to access all information a client can need.
/// It is a Facade for the various specialized and specific data providers (eg. DB or IS)
///
/// \author Barthélémy von Haller
class ClientDataProvider
{
  public:
    /// Default constructor
    ClientDataProvider();
    /// Destructor
    virtual ~ClientDataProvider();

    // API
    TObject* getObject(std::string taskName, std::string objectName);
    std::vector<std::string> getListOfActiveTasks();
    std::string getTaskStatus(std::string taskName);
    std::vector<std::string> getPublicationList(std::string taskName);

  private:
    // Facaded systems
    Repository::DatabaseInterface *database;

};

} /* namespace Client */
} /* namespace QualityControl */
} /* namespace AliceO2 */

#endif /* QUALITYCONTROL_CLIENT_CLIENT_DATA_PROVIDER_H_ */
