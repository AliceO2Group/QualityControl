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
/// \file   QualitiesToTRFCollectionConverter.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_QUALITIESTOTRFCOLLECTIONCONVERTER_H
#define QUALITYCONTROL_QUALITIESTOTRFCOLLECTIONCONVERTER_H

#include "QualityControl/QualityObject.h"
#include <DataFormatsQualityControl/FlagReasons.h>
#include <DataFormatsQualityControl/TimeRangeFlagCollection.h>
#include <memory>
#include <optional>

namespace o2::quality_control::core
{

/// \brief Converts a set of chronologically provided Qualities from the same path into a TRFCollection.
class QualitiesToTRFCollectionConverter
{
 public:
  QualitiesToTRFCollectionConverter(std::string trfcName, std::string detectorCode, uint64_t startTimeLimit, uint64_t endTimeLimit, std::string qoPath);
  ~QualitiesToTRFCollectionConverter() = default;

  /// \brief Converts a Quality into TRFCollection. The converter should get Qualities in chronological order.
  void operator()(const QualityObject&);

  /// \brief Moves the final TRFCollection out and resets the converter.
  std::unique_ptr<TimeRangeFlagCollection> getResult();

  size_t getQOsIncluded() const;
  size_t getWorseThanGoodQOs() const;

 private:
  uint64_t mStartTimeLimit;
  uint64_t mEndTimeLimit;
  std::string mQOPath; // this is only to indicate what is the missing Quality in TRF

  std::unique_ptr<TimeRangeFlagCollection> mConverted;

  uint64_t mCurrentStartTime;
  uint64_t mCurrentEndTime;
  std::optional<TimeRangeFlag> mCurrentTRF;
  size_t mQOsIncluded;
  size_t mWorseThanGoodQOs;
};

} // namespace o2::quality_control::core

#endif //QUALITYCONTROL_QUALITIESTOTRFCOLLECTIONCONVERTER_H
