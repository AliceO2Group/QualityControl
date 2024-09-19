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
/// \file   QualitiesToFlagCollectionConverter.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_QUALITIESTOQCFCOLLECTIONCONVERTER_H
#define QUALITYCONTROL_QUALITIESTOQCFCOLLECTIONCONVERTER_H

#include <DataFormatsQualityControl/QualityControlFlag.h>
#include <DataFormatsQualityControl/QualityControlFlagCollection.h>
#include <QualityControl/ValidityInterval.h>

#include <memory>
#include <vector>
#include <functional>

namespace o2::quality_control::core
{

class QualityObject;

/// \brief Converts series of Quality Objects from the same path into a QualityControlFlagCollection.
class QualitiesToFlagCollectionConverter
{
 public:
  QualitiesToFlagCollectionConverter(std::unique_ptr<QualityControlFlagCollection> emptyQcfc, std::string qoPath);

  ~QualitiesToFlagCollectionConverter() = default;

  /// \brief Converts a Quality into FlagCollection. The converter should get Qualities in chronological order.
  void operator()(const QualityObject&);

  /// \brief Moves the final FlagCollection out and resets the converter.
  std::unique_ptr<QualityControlFlagCollection> getResult();

  size_t getQOsIncluded() const;
  size_t getWorseThanGoodQOs() const;

  /// Sets the provided validity interval, trims affected flags and fills extensions with UnknownQuality
  void updateValidityInterval(const ValidityInterval validityInterval);

 private:
  /// \brief inserts the provided flag to the buffer, takes care of merging and trimming
  void insert(QualityControlFlag&& flag);

  /// \brief trims all buffered flags which match the predicate using the provided interval
  void trimBufferWithInterval(
    ValidityInterval interval,
    const std::function<bool(const QualityControlFlag&)>& predicate = [](const auto&) { return true; });

  /// \brief trims the provided flag with all buffered flags which match the predicate
  ///
  /// The result is a vector, because a flag interval split in the middle becomes two flags.
  std::vector<QualityControlFlag> trimFlagAgainstBuffer(
    const QualityControlFlag& newFlag,
    const std::function<bool(const QualityControlFlag&)>& predicate = [](const auto&) { return true; });

  std::string mQOPath; // this is only to indicate what is the missing Quality in QC Flag
  std::unique_ptr<QualityControlFlagCollection> mConverted;
  std::set<QualityControlFlag> mFlagBuffer;
  size_t mQOsIncluded = 0;
  size_t mWorseThanGoodQOs = 0;
};

} // namespace o2::quality_control::core

#endif // QUALITYCONTROL_QUALITIESTOQCFCOLLECTIONCONVERTER_H
