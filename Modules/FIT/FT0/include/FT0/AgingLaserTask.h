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
/// \file   AgingLaserTask.h
/// \author Andreas Molander <andreas.molander@cern.ch>, Sandor Lokos <sandor.lokos@cern.ch>, Edmundo Garcia-Solis <edmundo.garcia@cern.ch>
///

#ifndef QC_MODULE_FT0_AGINGLASERTASK_H
#define QC_MODULE_FT0_AGINGLASERTASK_H

#include "QualityControl/TaskInterface.h"

#include <CommonConstants/LHCConstants.h>
#include <FT0Base/Constants.h>

#include <TH1I.h>
#include <TH2I.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <vector>

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::ft0
{

class AgingLaserTask final : public TaskInterface
{
 public:
  AgingLaserTask() = default;
  ~AgingLaserTask() override;

  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  /// Check if the laser was triggered for this BC
  /// \param bc      BC to check
  /// \param bcDelay Expected BC delay from trigger to signal
  /// \return        True if the laser was triggered
  bool bcIsTrigger(int bc, int bcDelay) const;

  /// Check if a detector signal is expected for this BC
  /// \param bc BC to check
  /// \return   True if a detector signal is expected
  bool bcIsDetector(int bc) const;

  /// Check if the first reference signal is expected for this BC
  /// \param bc      BC to check
  /// \param refChId Rerernce channel where signal is seen
  /// \return        True if the first reference signal is expected for this BC
  bool bcIsPeak1(int bc, int refChId) const;

  /// Check if the second reference signal is expected for this BC
  /// \param bc      BC to check
  /// \param refChId Rerernce channel where signal is seen
  /// \return        True if the second reference signal is expected for this BC
  bool bcIsPeak2(int bc, int refChId) const;

  // Constants
  constexpr static std::size_t sNCHANNELS_PM = o2::ft0::Constants::sNCHANNELS_PM; ///< Max number of FT0 channels
  constexpr static std::size_t sMaxBC = o2::constants::lhc::LHCMaxBunches;        ///< Max number of BCs

  // Task parameters
  std::vector<uint8_t> mDetectorChIDs;            ///< Enabled detector channels
  std::vector<uint8_t> mReferenceChIDs;           ///< Enabled reference channels
  int mDetectorAmpCut;                            ///< Amplitude cut for detector channels
  int mReferenceAmpCut;                           ///< Amplitude cut for reference channels
  std::vector<int> mLaserTriggerBCs;              ///< BCs in which the laser is triggered
  int mDetectorBCdelay;                           ///< BC delay for detector channels (same for all)
  std::map<uint8_t, int> mReferencePeak1BCdelays; ///< BC delays for reference channel peak 1 (per channel)
  std::map<uint8_t, int> mReferencePeak2BCdelays; ///< BC delays for reference channel peak 2 (per channel)

  bool mDebug = false; ///< Enable more histograms in debug mode

  // Histograms

  // Amplitude per channel
  std::unique_ptr<TH2I> mHistAmpVsChADC0;      ///< Amplitude per channel for ADC0 (detector + reference channels)
  std::unique_ptr<TH2I> mHistAmpVsChADC1;      ///< Amplitude per channel for ADC1 (detector + reference channels)
  std::unique_ptr<TH2I> mHistAmpVsChPeak1ADC0; ///< Amplitude per channel for peak 1 for ADC0 (reference channels)
  std::unique_ptr<TH2I> mHistAmpVsChPeak1ADC1; ///< Amplitude per channel for peak 1 for ADC1 (reference channels)
  std::unique_ptr<TH2I> mHistAmpVsChPeak2ADC0; ///< Amplitude per channel for peak 2 for ADC0 (reference channels)
  std::unique_ptr<TH2I> mHistAmpVsChPeak2ADC1; ///< Amplitude per channel for peak 2 for ADC1 (reference channels)

  // // Time per channel
  std::unique_ptr<TH2I> mHistTimeVsCh;      ///< Time per channel (detector + reference channels)
  std::unique_ptr<TH2I> mHistTimeVsChPeak1; ///< Time per channel for peak 1 (reference channels, both ADCs)
  std::unique_ptr<TH2I> mHistTimeVsChPeak2; ///< Time per channel for peak 2 (reference channels, both ADCs)

  // Debug histograms

  // Ampltiude per channel
  std::unique_ptr<TH2I> mHistAmpVsCh; ///< Amplitude per channel (detector + reference channels)

  // Ampltidue histograms for reference channel peaks
  std::map<uint8_t, std::unique_ptr<TH1I>> mMapDebugHistAmp;          ///< Amplitude (both ADCs and peaks)
  std::map<uint8_t, std::unique_ptr<TH1I>> mMapDebugHistAmpADC0;      ///< Ampltidue for ADC0
  std::map<uint8_t, std::unique_ptr<TH1I>> mMapDebugHistAmpADC1;      ///< Ampltidue for ADC1
  std::map<uint8_t, std::unique_ptr<TH1I>> mMapDebugHistAmpPeak1;     ///< Amplitude for peak 1
  std::map<uint8_t, std::unique_ptr<TH1I>> mMapDebugHistAmpPeak2;     ///< Amplitude for peak 2
  std::map<uint8_t, std::unique_ptr<TH1I>> mMapDebugHistAmpPeak1ADC0; ///< Amplitude for peak 1 for ADC0
  std::map<uint8_t, std::unique_ptr<TH1I>> mMapDebugHistAmpPeak1ADC1; ///< Amplitude for peak 1 for ADC1
  std::map<uint8_t, std::unique_ptr<TH1I>> mMapDebugHistAmpPeak2ADC0; ///< Amplitude for peak 2 for ADC0
  std::map<uint8_t, std::unique_ptr<TH1I>> mMapDebugHistAmpPeak2ADC1; ///< Amplitude for peak 2 for ADC1

  // Time per channel
  std::unique_ptr<TH2I> mDebugHistTimeVsChADC0;      ///< Time per channel for ADC0 (detector + reference channels)
  std::unique_ptr<TH2I> mDebugHistTimeVsChADC1;      ///< Time per channel for ADC1 (detector + reference channels)
  std::unique_ptr<TH2I> mDebugHistTimeVsChPeak1ADC0; ///< Time per channel for peak 1 for ADC0 (reference channels)
  std::unique_ptr<TH2I> mDebugHistTimeVsChPeak1ADC1; ///< Time per channel for peak 1 for ADC1 (reference channels)
  std::unique_ptr<TH2I> mDebugHistTimeVsChPeak2ADC0; ///< Time per channel for peak 2 for ADC0 (reference channels)
  std::unique_ptr<TH2I> mDebugHistTimeVsChPeak2ADC1; ///< Time per channel for peak 2 for ADC1 (reference channels)

  // Time histograms for reference channel peaks
  // TODO: add mMapDebugHistTime, mMapDebugHistTimeADC0, mMapDebugHistTimeADC1
  std::map<uint8_t, std::unique_ptr<TH1I>> mMapDebugHistTimePeak1;
  std::map<uint8_t, std::unique_ptr<TH1I>> mMapDebugHistTimePeak2;
  std::map<uint8_t, std::unique_ptr<TH1I>> mMapDebugHistTimePeak1ADC0;
  std::map<uint8_t, std::unique_ptr<TH1I>> mMapDebugHistTimePeak1ADC1;
  std::map<uint8_t, std::unique_ptr<TH1I>> mMapDebugHistTimePeak2ADC0;
  std::map<uint8_t, std::unique_ptr<TH1I>> mMapDebugHistTimePeak2ADC1;

  // BC
  std::unique_ptr<TH1I> mDebugHistBC;                    ///< All BCs
  std::unique_ptr<TH1I> mDebugHistBCDetector;            ///< Detector channel BCs
  std::unique_ptr<TH1I> mDebugHistBCReference;           ///< Reference channel BCs
  std::unique_ptr<TH1I> mDebugHistBCAmpCut;              ///< All BCs with amplitude cut
  std::unique_ptr<TH1I> mDebugHistBCAmpCutADC0;          ///< All BCs with amplitude cut for ADC0
  std::unique_ptr<TH1I> mDebugHistBCAmpCutADC1;          ///< All BCs with amplitude cut for ADC1
  std::unique_ptr<TH1I> mDebugHistBCDetectorAmpCut;      ///< Detector channel BCs with amplitude cut
  std::unique_ptr<TH1I> mDebugHistBCDetectorAmpCutADC0;  ///< Detector channel BCs with amplitude cut for ADC0
  std::unique_ptr<TH1I> mDebugHistBCDetectorAmpCutADC1;  ///< Detector channel BCs with amplitude cut for ADC1
  std::unique_ptr<TH1I> mDebugHistBCReferenceAmpCut;     ///< Reference channel BCs with amplitude cut
  std::unique_ptr<TH1I> mDebugHistBCReferenceAmpCutADC0; ///< Reference channel BCs with amplitude cut for ADC0
  std::unique_ptr<TH1I> mDebugHistBCReferenceAmpCutADC1; ///< Reference channel BCs with amplitude cut for ADC1

  // Amplitude per BC for reference channels
  std::map<uint8_t, std::unique_ptr<TH2I>> mMapDebugHistAmpVsBC;     ///< Amplitude vs BC (both ADCs and peaks = 4 peaks)
  std::map<uint8_t, std::unique_ptr<TH2I>> mMapDebugHistAmpVsBCADC0; ///< Amplitude vs BC for ADC0 (both peaks)
  std::map<uint8_t, std::unique_ptr<TH2I>> mMapDebugHistAmpVsBCADC1; ///< Amplitude vs BC for ADC1 (both peaks)

  // Including the histograms below will produce 'object of class TObjArray read too many bytes' in --local-batch mode

  // // Time per BC for reference channels
  // std::map<uint8_t, std::unique_ptr<TH2I>> mMapDebugHistTimeVsBC;     ///< Time vs BC (both ADCs and peaks = 4 peaks)
  // std::map<uint8_t, std::unique_ptr<TH2I>> mMapDebugHistTimeVsBCADC0; ///< Time vs BC for ADC0 (both peaks)
  // std::map<uint8_t, std::unique_ptr<TH2I>> mMapDebugHistTimeVsBCADC1; ///< Time vs BC for ADC1 (both peaks)
};

} // namespace o2::quality_control_modules::ft0

#endif // QC_MODULE_FT0_AGINGLASERTASK_H
