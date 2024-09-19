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
/// \file   FlagHelpers.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_FLAGHELPERS_H
#define QUALITYCONTROL_FLAGHELPERS_H

#include "DataFormatsQualityControl/QualityControlFlag.h"
#include "QualityControl/ValidityInterval.h"
#include <vector>
#include <optional>

namespace o2::quality_control::core::flag_helpers
{

/// \brief returns true if the provided intervals are valid and are overlapping or adjacent
bool intervalsConnect(const ValidityInterval& one, const ValidityInterval& other);

/// \brief returns true if the provided intervals are valid and are overlapping (there is at least one 1ms common)
bool intervalsOverlap(const ValidityInterval& one, const ValidityInterval& other);

/// \brief Removes the provided interval from the flag.
///
/// Removes the provided interval from the flag. A result can be:
/// - an empty vector if the interval fully covers the flag's interval
/// - a vector with one flag if the interval covers the flag's interval on one side
/// - a vector with two flags if the interval is fully contained by the flag's interval
std::vector<QualityControlFlag> excludeInterval(const QualityControlFlag& flag, ValidityInterval interval);

/// Trims the provided flag to the intersection with the provided interval.
/// If the intersection does not exist, it returns nullopt
std::optional<QualityControlFlag> intersection(const QualityControlFlag& flag, ValidityInterval interval);

} // namespace o2::quality_control::core::flag_helpers
#endif // QUALITYCONTROL_FLAGHELPERS_H
