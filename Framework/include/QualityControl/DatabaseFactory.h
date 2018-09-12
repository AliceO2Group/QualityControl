///
/// \file   DatabaseFactory.h
/// \author Barthelemy von Haller
///

#ifndef QC_REPOSITORY_DATABASEFACTORY_H
#define QC_REPOSITORY_DATABASEFACTORY_H

#include <memory>
// O2
#include "Common/Exceptions.h"
// QC
#include "QualityControl/DatabaseInterface.h"

namespace o2 {
namespace quality_control {
namespace repository {

/// \brief Factory to get a database accessor
class DatabaseFactory
{
  public:

    /// \brief Create a new instance of a DatabaseInterface.
    /// The DatabaseInterface actual class is decided based on the parameters passed.
    /// The ownership is returned as well.
    /// \param name Possible values : "MySql", "CCDB"
    /// \author Barthelemy von Haller
    static std::unique_ptr<DatabaseInterface> create(std::string name);
};

} // namespace repository
} // namespace quality_control
} // namespace o2

#endif // QC_REPOSITORY_DATABASEFACTORY_H
