///
/// \file   DatabaseFactory.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_CORE_DatabaseFactory_H_
#define QUALITYCONTROL_LIBS_CORE_DatabaseFactory_H_

#include <iostream>
// O2
#include "Common/Exceptions.h"
// QC
#include "QualityControl/DatabaseInterface.h"

namespace AliceO2 {
namespace QualityControl {
namespace Repository {

class DatabaseFactory
{
  public:

    /// \brief Create a new instance of a DatabaseInterface.
    /// The DatabaseInterface actual class is decided based on the parameters passed.
    /// The ownership is returned as well.
    /// TODO use unique_ptr
    /// \param name MySql
    /// \author Barthelemy von Haller
    static DatabaseInterface* create(std::string name);
};

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

#endif // QUALITYCONTROL_LIBS_CORE_DatabaseFactory_H_
