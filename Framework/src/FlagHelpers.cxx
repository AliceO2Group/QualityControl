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
/// \file   FlagHelpers.cxx
/// \author Piotr Konopka
///

#include "QualityControl/FlagHelpers.h"

namespace o2::quality_control::core::flag_helpers
{

bool intervalsConnect(const ValidityInterval& one, const ValidityInterval& other)
{
  // Object validity in CCDB is a right-open range, which means it includes the beginning and excludes the ending.
  // In other words, for the validity [1, 10), 9 is the last integer to be included.
  // Thus, ranges [1, 10) and [10, 20) are considered adjacent, while [1, 10) and [11, 20) are already separate
  // and should not be merged.
  return one.isValid() && other.isValid() && one.getMax() >= other.getMin() && one.getMin() <= other.getMax();
}

bool intervalsOverlap(const ValidityInterval& one, const ValidityInterval& other)
{
  return one.isValid() && other.isValid() && one.getMax() > other.getMin() && one.getMin() < other.getMax();
}

std::vector<QualityControlFlag> excludeInterval(const QualityControlFlag& flag, ValidityInterval interval)
{
  std::vector<QualityControlFlag> result;

  if (flag.getInterval().isInvalid()) {
    return result;
  }

  if (auto overlap = flag.getInterval().getOverlap(interval); overlap.isInvalid() || overlap.isZeroLength()) {
    result.push_back(flag);
    return result;
  }

  if (interval.getMin() > flag.getStart()) {
    result.emplace_back(flag.getStart(), interval.getMin(), flag.getFlag(), flag.getComment(), flag.getSource());
  }
  if (interval.getMax() < flag.getEnd()) {
    result.emplace_back(interval.getMax(), flag.getEnd(), flag.getFlag(), flag.getComment(), flag.getSource());
  }
  return result;
}

std::optional<QualityControlFlag> intersection(const QualityControlFlag& flag, ValidityInterval interval)
{
  if (flag.getInterval().isInvalid()) {
    return std::nullopt;
  }
  if (interval.isInvalid()) {
    return flag;
  }
  auto intersection = flag.getInterval().getOverlap(interval);
  if (intersection.isInvalid() || intersection.isZeroLength()) {
    return std::nullopt;
  }
  return QualityControlFlag{ intersection.getMin(), intersection.getMax(), flag.getFlag(), flag.getComment(), flag.getSource() };
}

} // namespace o2::quality_control::core::flag_helpers
