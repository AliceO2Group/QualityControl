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
/// \file   TestbeamRawTask.h
/// \author My Name
///

#ifndef QC_MODULE_FOCAL_FOCALTESTBEAMRAWTASK_H
#define QC_MODULE_FOCAL_FOCALTESTBEAMRAWTASK_H

#include <array>
#include <unordered_map>

#include "QualityControl/TaskInterface.h"
#include "CommonDataFormat/InteractionRecord.h"
#include "ITSMFTReconstruction/GBTWord.h"
#include "FOCAL/PadWord.h"
#include "FOCAL/PadDecoder.h"
#include "FOCAL/PadMapper.h"
#include "FOCAL/PixelDecoder.h"
#include "FOCAL/PixelMapper.h"

class TH1;
class TH2;
class TProfile2D;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::focal
{

/// \brief Example Quality Control DPL Task
/// \author My Name
class TestbeamRawTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  TestbeamRawTask() = default;
  /// Destructor
  ~TestbeamRawTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  static constexpr int PIXEL_ROWS_IB = 512,
                       PIXEL_COLS_IB = 1024,
                       PIXEL_ROW_SEGMENTSIZE_IB = 8,
                       PIXEL_COL_SEGMENSIZE_IB = 32,
                       PIXEL_ROWS_OB = 512,
                       PIXEL_COLS_OB = 1024,
                       PIXEL_ROW_SEGMENTSIZE_OB = 8,
                       PIXEL_COL_SEGMENSIZE_OB = 32;
  void processPadPayload(gsl::span<const PadGBTWord> gbtpayload);
  void processPixelPayload(gsl::span<const o2::itsmft::GBTWord> gbtpayload, uint16_t feeID);
  void processPadEvent(gsl::span<const PadGBTWord> gbtpayload);
  std::pair<int, int> getNumberOfPixelSegments(PixelMapper::MappingType_t mappingtype) const;
  std::pair<int, int> getPixelSegment(const PixelHit& hit, PixelMapper::MappingType_t mappingtype) const;

  PadDecoder mPadDecoder;                                                         ///< Decoder for pad data
  PadMapper mPadMapper;                                                           ///< Mapping for Pads
  PixelDecoder mPixelDecoder;                                                     ///< Decoder for pixel data
  std::unique_ptr<PixelMapper> mPixelMapper;                                      ///< Testbeam mapping for pixels
  std::unordered_map<o2::InteractionRecord, int> mPixelNHitsAll;                  ///< Number of hits / event all layers
  std::array<std::unordered_map<o2::InteractionRecord, int>, 2> mPixelNHitsLayer; ///< Number of hits / event layer
  std::vector<int> mHitSegmentCounter;                                            ///< Number of hits / segment
  bool mDebugMode = false;                                                        ///< Additional debug verbosity

  /////////////////////////////////////////////////////////////////////////////////////
  /// Pad histograms
  /////////////////////////////////////////////////////////////////////////////////////
  TH1* mPayloadSizePadsGBT;                             ///< Payload size GBT words of pad data
  std::array<TH2*, PadData::NASICS> mPadASICChannelADC; ///< ADC per channel for each ASIC
  std::array<TH2*, PadData::NASICS> mPadASICChannelTOA; ///< TOA per channel for each ASIC
  std::array<TH2*, PadData::NASICS> mPadASICChannelTOT; ///< TOT per channel for each ASIC
  std::array<TH2*, PadData::NASICS> mHitMapPadASIC;     ///< Hitmap per ASIC

  /////////////////////////////////////////////////////////////////////////////////////
  /// Pixel histograms
  /////////////////////////////////////////////////////////////////////////////////////
  TH1* mLinksWithPayloadPixel;                             ///< HBF with payload per link
  TH2* mTriggersFeePixel;                                  ///< Nunber of triggers per HBF and FEE ID
  TProfile2D* mAverageHitsChipPixel;                       ///< Average number of hits / chip
  TH1* mHitsChipPixel;                                     ///< Number of hits / chip
  TH2* mPixelChipsIDsFound;                                ///< Chip IDs vs FEE IDs
  TH2* mPixelChipsIDsHits;                                 ///< Chip IDs with hits vs FEE IDs
  std::array<TProfile2D*, 2> mPixelChipHitProfileLayer;    ///< Hit profile for pixel chips
  std::array<TH2*, 2> mPixelChipHitmapLayer;               ///< Hit map for pixel chips
  std::array<TProfile2D*, 2> mPixelSegmentHitProfileLayer; ///< Hit profile for pixel segments
  std::array<TH2*, 2> mPixelSegmentHitmapLayer;            ///< Hit map for pixel segments
  std::array<TH2*, 2> mPixelHitDistribitionLayer;          ///< Hit distribution per chip in layer
  TH1* mPixelHitsTriggerAll;                               ///< Number of pixel hits / trigger
  std::array<TH1*, 2> mPixelHitsTriggerLayer;              ///< Number of pixel hits in layer / trigger
};

} // namespace o2::quality_control_modules::focal

#endif // QC_MODULE_FOCAL_FOCALTESTBEAMRAWTASK_H
