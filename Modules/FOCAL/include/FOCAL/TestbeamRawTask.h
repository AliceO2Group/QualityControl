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
#include "FOCALReconstruction/PadWord.h"
#include "FOCALReconstruction/PadDecoder.h"
#include "FOCALReconstruction/PadMapper.h"
#include "FOCALReconstruction/PixelDecoder.h"
#include "FOCALReconstruction/PixelMapper.h"

class TH1;
class TH2;
class TProfile2D;

namespace o2::focal
{
class PadPedestal;
class PadBadChannelMap;
} // namespace o2::focal

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
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  static constexpr int PAD_ASICS = 18,
                       PIXEL_ROWS_IB = 512,
                       PIXEL_COLS_IB = 1024,
                       PIXEL_ROW_SEGMENTSIZE_IB = 8,
                       PIXEL_COL_SEGMENSIZE_IB = 32,
                       PIXEL_ROWS_OB = 512,
                       PIXEL_COLS_OB = 1024,
                       PIXEL_ROW_SEGMENTSIZE_OB = 8,
                       PIXEL_COL_SEGMENSIZE_OB = 32;
  struct PadChannelProjections {
    PadChannelProjections() = default;
    ~PadChannelProjections();
    std::unordered_map<int, TH1*> mHistos;
    void init(const std::vector<int> channels, int ASICid);
    void startPublishing(o2::quality_control::core::ObjectsManager& manager);
    void reset();
  };
  void default_init();
  bool isLostTimeframe(framework::ProcessingContext& ctx) const;
  void processPadPayload(gsl::span<const o2::focal::PadGBTWord> gbtpayload);
  void processPixelPayload(gsl::span<const o2::itsmft::GBTWord> gbtpayload, uint16_t feeID);
  void processPadEvent(gsl::span<const o2::focal::PadGBTWord> gbtpayload);
  std::pair<int, int> getNumberOfPixelSegments(o2::focal::PixelMapper::MappingType_t mappingtype) const;
  std::pair<int, int> getPixelSegment(const o2::focal::PixelHit& hit, o2::focal::PixelMapper::MappingType_t mappingtype, const o2::focal::PixelMapper::ChipPosition& chipMapping) const;

  o2::focal::PadDecoder mPadDecoder;                                              ///< Decoder for pad data
  o2::focal::PadMapper mPadMapper;                                                ///< Mapping for Pads
  o2::focal::PixelDecoder mPixelDecoder;                                          ///< Decoder for pixel data
  o2::focal::PadPedestal* mPadPedestalHandler = nullptr;                          ///< Pedestal handler for pad pedestal subtraction
  o2::focal::PadBadChannelMap* mPadBadChannelMap = nullptr;                       ///< Bad channel map for pads
  std::unique_ptr<o2::focal::PixelMapper> mPixelMapper;                           ///< Testbeam mapping for pixels
  std::unordered_map<o2::InteractionRecord, int> mPixelNHitsAll;                  ///< Number of hits / event all layers
  std::array<std::unordered_map<o2::InteractionRecord, int>, 2> mPixelNHitsLayer; ///< Number of hits / event layer
  std::vector<int> mHitSegmentCounter;                                            ///< Number of hits / segment
  std::vector<int> mChannelsPadProjections;                                       ///< Channels selected for pad projections
  int mPadTOTCutADC = 1;                                                          ///< Max TOT for ADC plot
  bool mDebugMode = false;                                                        ///< Additional debug verbosity
  bool mDisablePads = false;                                                      ///< Disable pads
  bool mDisablePixels = false;                                                    ///< Disable pixels
  bool mEnablePedestalSubtraction = false;                                        ///< Enable pedestal subtraction pads
  bool mEnableBadChannelMask = false;                                             ///< Enable bad channel map for pads

  /////////////////////////////////////////////////////////////////////////////////////
  /// General histograms
  /////////////////////////////////////////////////////////////////////////////////////
  TH1* mTFerrorCounter = nullptr; ///< Number of TF builder errors
  TH1* mFEENumberHBF = nullptr;   ///< Number of HBFs per FEE
  TH1* mFEENumberTF = nullptr;    ///< Number of TFs per FEE
  TH1* mNumLinksTF = nullptr;     ///< Number of links per timeframe
  TH1* mNumHBFPerCRU = nullptr;   ///< Number of HBFs per CRU
  TH2* mCRUcounter = nullptr;     ///< CRU counter
  TH1* mPayloadSizeTF = nullptr;  ///< Payload size per timeframe

  /////////////////////////////////////////////////////////////////////////////////////
  /// Pad histograms
  /////////////////////////////////////////////////////////////////////////////////////
  TH1* mPayloadSizePadsGBT = nullptr;                                                   ///< Payload size GBT words of pad data
  std::array<TH2*, PAD_ASICS> mPadASICChannelADC;                                       ///< ADC per channel for each ASIC
  std::array<TH2*, PAD_ASICS> mPadASICChannelTOA;                                       ///< TOA per channel for each ASIC
  std::array<TH2*, PAD_ASICS> mPadASICChannelTOT;                                       ///< TOT per channel for each ASIC
  std::array<TH2*, PAD_ASICS> mHitMapPadASIC;                                           ///< Hitmap per ASIC
  std::array<TH1*, PAD_ASICS> mPadTOTSumASIC;                                           ///< Sum of TOT per ASIC
  std::array<TH1*, PAD_ASICS> mPadADCSumASIC;                                           ///< Sum of ADC per ASIC
  std::array<TH2*, PAD_ASICS> mPadTOTCorrASIC;                                          ///< TOT correlation between ASICs
  std::array<TH2*, PAD_ASICS> mPadADCCorrASIC;                                          ///< ADC correlation between ASICs
  TH1* mPadTOTSumGlobal = nullptr;                                                      ///< Sum of TOT per event
  TH1* mPadADCSumGlobal = nullptr;                                                      ///< Sum of ADC per event
  std::array<std::unique_ptr<PadChannelProjections>, PAD_ASICS> mPadChannelProjections; ///< ADC projections per ASIC channel
  TH2* mPadTOAvsASIC = nullptr;                                                         ///< average TOA for each ASICs
  TH2* mPadTOAvsASIC_Ch14 = nullptr;                                                    ///< (no average) TOA for each ASICs for channel 14
  TH2* mPadTOAvsASIC_Ch16 = nullptr;                                                    ///< (no average) TOA for each ASICs for channel 16
  TH2* mPadTOAvsASIC_Ch19 = nullptr;                                                    ///< (no average) TOA for each ASICs for channel 19
  TH2* mPadTOAvsASIC_Ch48 = nullptr;                                                    ///< (no average) TOA for each ASICs for channel 48
  TH2* mPadTOAvsASIC_Ch52 = nullptr;                                                    ///< (no average) TOA for each ASICs for channel 52
  TH2* mPadTOAvsASIC_Ch61 = nullptr;                                                    ///< (no average) TOA for each ASICs for channel 61

  TH1* mPadGlobalMIPADC_Ch14_Asic0 = nullptr;       ///  ADC for channel 14 for asic 0 after cmn subtraction
  TH1* mPadGlobalMIPADC_Ch16_Asic0 = nullptr;       ///  ADC for channel 16 for asic 0 after cmn subtraction
  TH1* mPadGlobalMIPADC_Ch19_Asic0 = nullptr;       ///  ADC for channel 19 for asic 0 after cmn subtraction
  TH1* mPadGlobalMIPADC_Ch48_Asic0 = nullptr;       ///  ADC for channel 48 for asic 0 after cmn subtraction
  TH1* mPadGlobalMIPADC_Ch52_Asic0 = nullptr;       ///  ADC for channel 52 for asic 0 after cmn subtraction
  TH1* mPadGlobalMIPADC_Ch61_Asic0 = nullptr;       ///  ADC for channel 61 for asic 0 after cmn subtraction
  std::array<TH2*, PAD_ASICS> mPadTRIGvsWindowASIC; ///< Number of triggers per window for each ASIC
  TH2* mPadGlobalTOTvsADC = nullptr;                ///< TOT vs ADC for all pad layers
  TH2* mPadGlobalTOAvsADC = nullptr;                ///< TOA vs ADC for all pad layers
  /////////////////////////////////////////////////////////////////////////////////////
  /// Pixel histograms
  /////////////////////////////////////////////////////////////////////////////////////
  TH1* mPayloadSizePixelsGBT = nullptr;                    ///< Payload size pixel data in GBT word
  TH1* mLinksWithPayloadPixel = nullptr;                   ///< HBF with payload per link
  TH2* mTriggersFeePixel = nullptr;                        ///< Nunber of triggers per HBF and FEE ID
  TProfile2D* mAverageHitsChipPixel = nullptr;             ///< Average number of hits / chip
  TH1* mHitsChipPixel = nullptr;                           ///< Number of hits / chip
  TH2* mPixelChipsIDsFound = nullptr;                      ///< Chip IDs vs FEE IDs
  TH2* mPixelChipsIDsHits = nullptr;                       ///< Chip IDs with hits vs FEE IDs
  std::array<TH1*, 4> mPixelLaneIDChipIDFEE;               ///< Lane ID vs. chip ID for each FEE
  std::array<TProfile2D*, 2> mPixelChipHitProfileLayer;    ///< Hit profile for pixel chips
  std::array<TH2*, 2> mPixelChipHitmapLayer;               ///< Hit map for pixel chips
  std::array<TProfile2D*, 2> mPixelSegmentHitProfileLayer; ///< Hit profile for pixel segments
  std::array<TH2*, 2> mPixelSegmentHitmapLayer;            ///< Hit map for pixel segments
  std::array<TH2*, 2> mPixelHitDistribitionLayer;          ///< Hit distribution per chip in layer
  TH1* mPixelHitsTriggerAll = nullptr;                     ///< Number of pixel hits / trigger
  std::array<TH1*, 2> mPixelHitsTriggerLayer;              ///< Number of pixel hits in layer / trigger
};

} // namespace o2::quality_control_modules::focal

#endif // QC_MODULE_FOCAL_FOCALTESTBEAMRAWTASK_H
