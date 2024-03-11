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

namespace o2::quality_control::core
{

/// \brief Creates DataDescription from given name.
///
/// If the length of the name is <= 16 (hardcoded in DataDescription) it creates DataDescription from the original name.
/// However, if the length of the name is > 16, it will create hash of the whole name and replace ending hashLength of bytes
/// of the name with hexa representation of computed hash.
/// eg.: name == "veryLongNameThatIsLongerThan16B" with hashLengh == 4 will result in "veryLongNameABCD", where ABCD
/// is the hash create inside the function
///
/// \param name - name which should cut and hashed
/// \param hashLenght - number of bytes which will overwrite the end of the name
o2::header::DataDescription createDataDescription(const std::string& name, size_t hashLength);

} // namespace o2::quality_control::core

#endif
