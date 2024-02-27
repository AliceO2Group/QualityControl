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
/// \file    ReductorHelpers.h
/// \author  Piotr Konopka
///

#ifndef QUALITYCONTROL_REDUCTORHELPERS_H
#define QUALITYCONTROL_REDUCTORHELPERS_H

#include <string>

namespace o2::quality_control
{
namespace postprocessing
{
class Reductor;
struct Trigger;
} // namespace postprocessing
namespace core
{
class ConditionAccess;
}
namespace repository
{
class DatabaseInterface;
}
} // namespace o2::quality_control

namespace o2::quality_control::postprocessing::reductor_helpers
{

namespace implementation
{

/// \brief implementation details of updateReductor, hiding some header inclusions
bool updateReductorImpl(Reductor* r, const Trigger& t, const std::string& path, const std::string& name, const std::string& type,
                        repository::DatabaseInterface& qcdb, core::ConditionAccess& ccdbAccess);

} // namespace implementation

/// \brief Updates the provided Reductor with implementation-specific procedures
///
/// \tparam DataSourceT data source structure type to be accessed. path, name and type string members are required.
/// \param r reductor which is going to be type-checked
/// \param t trigger
/// \param ds data source
/// \param qcdb QCDB interface
/// \param ccdbAccess a class which has access to conditions
/// \return bool value indicating the success or failure in reducing an object
template <typename DataSourceT>
bool updateReductor(Reductor* r, const Trigger& t, const DataSourceT& ds, repository::DatabaseInterface& qcdb, core::ConditionAccess& ccdbAccess)
{
  const std::string& path = ds.path;
  const std::string& name = ds.name;
  const std::string& type = ds.type;

  return implementation::updateReductorImpl(r, t, path, name, type, qcdb, ccdbAccess);
}

} // namespace o2::quality_control::postprocessing::reductor_helpers
#endif // QUALITYCONTROL_REDUCTORHELPERS_H
