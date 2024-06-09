// Copyright 2024 CERN and copyright holders of ALICE O2.
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
/// \file   ProvenanceToQCRunType.h
/// \author Michal Tichak
///

#include <stdexcept>
#include <string>
namespace o2::quality_control::core
{

enum class Provenance {
  SyncQc,
  AsyncQc,
  MCQC
};

inline Provenance toEnum(const std::string& provenance)
{
  if (provenance == "qc_mc") {
    return Provenance::MCQC;
  }

  if (provenance == "qc") {
    return Provenance::SyncQc;
  }

  if (provenance == "qc_async") {
    return Provenance::AsyncQc;
  }

  throw std::runtime_error{ "unknown provenance flag: " + provenance };
}

} // namespace o2::quality_control::core
