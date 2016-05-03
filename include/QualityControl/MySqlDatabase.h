///
/// \file   MySqlDatabase.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_REPOSITORY_MYSQL_DATABASE_H_
#define QUALITYCONTROL_REPOSITORY_MYSQL_DATABASE_H_

#include "QualityControl/DatabaseInterface.h"

namespace AliceO2 {
namespace QualityControl {
namespace Repository {

class MySqlDatabase: public DatabaseInterface
{
  public:
    /// Default constructor
    MySqlDatabase();
    /// Destructor
    virtual ~MySqlDatabase();

    void Connect() override;
    void Store(AliceO2::QualityControl::Core::MonitorObject *mo) override;
    AliceO2::QualityControl::Core::MonitorObject* Retrieve(std::string name) override;
    void Disconnect() override;

};

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

#endif // QUALITYCONTROL_REPOSITORY_MYSQL_DATABASE_H_
