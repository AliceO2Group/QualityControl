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
/// \file    ValidityInterval.h
/// \author  Piotr Konopka
///

#ifndef QUALITYCONTROL_VALIDITYINTERVAL_H
#define QUALITYCONTROL_VALIDITYINTERVAL_H

#include <MathUtils/detail/Bracket.h>

namespace o2::quality_control::core
{

using time_type = uint64_t; // ms since epoch
using ValidityInterval = o2::math_utils::detail::Bracket<time_type>;
const static ValidityInterval gInvalidValidityInterval{
  std::numeric_limits<time_type>::max(), std::numeric_limits<time_type>::min()
};

} // namespace o2::quality_control::core

#endif //QUALITYCONTROL_VALIDITYINTERVAL_H
