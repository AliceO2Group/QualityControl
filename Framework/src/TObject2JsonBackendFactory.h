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
/// \file   TObject2JsonBackendFactory.h
/// \author Adam Wegrzynek
/// \author Vladimir Kosmala
///

#ifndef QUALITYCONTROL_TOBJECT2JSON_BACKEND_FACTORY_H
#define QUALITYCONTROL_TOBJECT2JSON_BACKEND_FACTORY_H

#include "TObject2JsonServer.h"
#include "TObject2JsonBackend.h"

namespace o2 {
namespace quality_control {
namespace tobject_to_json {

/// Creates a backend instance
class TObject2JsonBackendFactory
{
  public:
    /// Disables copy constructor
    TObject2JsonBackendFactory & operator=(const TObject2JsonBackendFactory&) = delete;
    TObject2JsonBackendFactory(const TObject2JsonBackendFactory&) = delete;

    /// Creates an instance of backend depending on the URL passed
    static std::unique_ptr<Backend> get(std::string url);

  private:
    /// Private constructor disallows to create instance of Factory
    TObject2JsonBackendFactory() = default;
};

} // namespace tobject_to_json {
} // namespace quality_control
} // namespace o2

#endif // QUALITYCONTROL_TOBJECT2JSON_BACKEND_FACTORY_H
