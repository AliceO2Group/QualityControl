// Copyright 2019-2024 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#ifndef QC_HASH_DATA_DESCRIPTION_H
#define QC_HASH_DATA_DESCRIPTION_H

#include <cstddef>
#include <string>

#include <Headers/DataHeader.h>

namespace o2::common::hash
{

o2::header::DataDescription createDataDescription(const std::string& name, size_t hash_length = 4);

} // namespace o2::common::hash

#endif
