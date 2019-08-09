// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   DatabaseFactory.cxx
/// \author Barthelemy von Haller
///

// ROOT
#include <QualityControl/CcdbDatabase.h>
// O2
#include <Common/Exceptions.h>
// QC
#include "QualityControl/DatabaseFactory.h"
#include "QualityControl/QcInfoLogger.h"
#ifdef _WITH_MYSQL
#include "QualityControl/MySqlDatabase.h"
#endif

using namespace std;
using namespace AliceO2::Common;
using namespace o2::quality_control::core;

namespace o2::quality_control::repository
{

std::unique_ptr<DatabaseInterface> DatabaseFactory::create(std::string name)
{
  if (name == "MySql") {
    QcInfoLogger::GetInstance() << "MySQL backend selected" << QcInfoLogger::endm;
#ifdef _WITH_MYSQL
    return std::make_unique<MySqlDatabase>();
#else
    BOOST_THROW_EXCEPTION(FatalException()
                          << errinfo_details("MySQL was not available during the compilation of the QC"));
#endif
  } else if (name == "CCDB") {
    // TODO check if CCDB installed
    QcInfoLogger::GetInstance() << "CCDB backend selected" << QcInfoLogger::endm;
    return std::make_unique<CcdbDatabase>();
  } else {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("No database named " + name));
  }
  return nullptr;
}

} // namespace o2::quality_control::repository
