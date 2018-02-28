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
/// \file   TObejct2Json.h
/// \author Adam Wegrzynek
///

#ifndef QUALITYCONTROL_TOBJECT2JSON_FACTORY_H
#define QUALITYCONTROL_TOBJECT2JSON_FACTORY_H

#include "TObject2Json.h"
#include "TObject2JsonBackend.h"

namespace o2 {
namespace quality_control {
namespace tobject_to_json {

/// Creates and condifures TObject2Json object
class TObject2JsonFactory
{
  public:
    /// Disables copy constructor
    TObject2JsonFactory & operator=(const TObject2JsonFactory&) = delete;
    TObject2JsonFactory(const TObject2JsonFactory&) = delete;

    /// Creates an instance of TObject2Json
    /// \return              renerence to TObject2Json object
    static std::unique_ptr<TObject2Json> Get(std::string url, std::string zeromqUrl);

  private:
    /// Private constructor disallows to create instance of Factory
    TObject2JsonFactory() = default;
};

} // namespace tobject_to_json {
} // namespace quality_control
} // namespace o2

#endif // QUALITYCONTROL_TOBJECT2JSON_FACTORY_H
