
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
/// \file   DatabaseFactory.h
/// \author Barthelemy von Haller
///

#ifndef QC_REPOSITORY_DATABASEFACTORY_H
#define QC_REPOSITORY_DATABASEFACTORY_H

#include <memory>
// QC
#include "QualityControl/DatabaseInterface.h"

namespace o2::quality_control::repository
{

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

} // namespace o2::quality_control::repository

#endif // QC_REPOSITORY_DATABASEFACTORY_H
