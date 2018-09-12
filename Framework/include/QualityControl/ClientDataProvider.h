///
/// \file   ClientDataProvider.h
/// \author Barthelemy von Haller
///

#ifndef QC_CLIENT_CLIENTDATAPROVIDER_H
#define QC_CLIENT_CLIENTDATAPROVIDER_H

#include "QualityControl/MonitorObject.h"
#include "QualityControl/DatabaseInterface.h"

using namespace o2::quality_control;

namespace o2 {
namespace quality_control {
namespace client
{

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
    std::unique_ptr<repository::DatabaseInterface> database;

};

} /* namespace client */
} /* namespace quality_control */
} /* namespace o2 */

#endif /* QC_CLIENT_CLIENTDATAPROVIDER_H */
