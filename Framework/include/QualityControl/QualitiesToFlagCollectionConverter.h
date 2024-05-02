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
#include <memory>
#include <vector>

namespace o2::quality_control::core
{

class QualityObject;

/// \brief Converts a set of chronologically provided Qualities from the same path into a QualityControlFlagCollection.
class QualitiesToFlagCollectionConverter
{
 public:
  QualitiesToFlagCollectionConverter(std::unique_ptr<QualityControlFlagCollection> qcfc, std::string qoPath);

  ~QualitiesToFlagCollectionConverter() = default;

  /// \brief Converts a Quality into FlagCollection. The converter should get Qualities in chronological order.
  void operator()(const QualityObject&);

  /// \brief Moves the final FlagCollection out and resets the converter.
  std::unique_ptr<QualityControlFlagCollection> getResult();

  size_t getQOsIncluded() const;
  size_t getWorseThanGoodQOs() const;

 private:
  std::string mQOPath; // this is only to indicate what is the missing Quality in QC Flag

  std::unique_ptr<QualityControlFlagCollection> mConverted;

  uint64_t mCurrentStartTime = 0;
  uint64_t mCurrentEndTime;
  std::vector<QualityControlFlag> mCurrentFlags;
  size_t mQOsIncluded = 0;
  size_t mWorseThanGoodQOs = 0;
};

} // namespace o2::quality_control::core

#endif // QUALITYCONTROL_QUALITIESTOQCFCOLLECTIONCONVERTER_H
