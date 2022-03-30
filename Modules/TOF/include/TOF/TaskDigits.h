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
/// \file   TaskDigits.h
/// \author Nicolò Jacazio <nicolo.jacazio@cern.ch>
/// \brief  Task to monitor quantities in TOF digits in both data and MC
///

#ifndef QC_MODULE_TOF_TASK_DIGITS_H
#define QC_MODULE_TOF_TASK_DIGITS_H

#include "QualityControl/TaskInterface.h"
#include "Base/Counter.h"
#include "TOF/TaskRaw.h"
#include "CommonConstants/LHCConstants.h"

class TH1F;
class TH2F;
class TH1I;
class TH2I;
class TProfile;
class TProfile2D;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::tof
{

/// \brief TOF Quality Control DPL Task for digits. Monitoring multiplicity, time, ToT and readout errors
/// \author Nicolò Jacazio <nicolo.jacazio@cern.ch>
class TaskDigits final : public TaskInterface
{
 public:
  /// \brief Constructor
  TaskDigits();
  /// Destructor
  ~TaskDigits() override = default;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  ////////////////////////
  // Histogram binnings //
  ////////////////////////

  // Orbit
  static constexpr int mBinsOrbitId = 1024;        /// Number of bins for the orbitID axis
  static constexpr int mRangeMaxOrbitId = 1048576; /// Max range in the orbitID axis
  // BC
  static constexpr int mBinsBC = 594;                                     /// Number of bins for the BC axis
  static constexpr float mRangeMaxBC = o2::constants::lhc::LHCMaxBunches; /// Max range in the BC axis
  // Event counter
  static constexpr int mBinsEventCounter = 1000;                  /// Number of bins for the EventCounter axis
  static constexpr int mRangeMaxEventCounter = mBinsEventCounter; /// Max range in the EventCounter axis
  // Orbit in the Time Frame
  static constexpr int mRangeMaxOrbitPerTimeFrame = 256;                        /// Number of bins for the OrbitPerTimeFrame axis.
  static constexpr int mBinsOrbitPerTimeFrame = mRangeMaxOrbitPerTimeFrame * 3; /// Max range in the OrbitPerTimeFrame axis. 3 orbits are recorded per time frame
  // Multiplicity
  int mBinsMultiplicity = 2000;                   /// Number of bins in multiplicity plot
  static constexpr int mRangeMinMultiplicity = 0; /// Min range in multiplicity plot
  int mRangeMaxMultiplicity = mBinsMultiplicity;  /// Max range in multiplicity plot
  // Time
  int mBinsTime = 300;                                  /// Number of bins in time plot
  float fgkNbinsWidthTime = 2.44;                       /// Width of bins in time plot
  float mRangeMinTime = 0.f;                            /// Range min in time plot
  float mRangeMaxTime = o2::constants::lhc::LHCOrbitNS; /// Range max in time plot
  // ToT
  int mBinsToT = 100;        /// Number of bins in ToT plot
  float mRangeMinToT = 0;    /// Range min in ToT plot
  float mRangeMaxToT = 48.8; /// Range max in ToT plot

  ///////////
  // Flags //
  ///////////

  bool mFlagEnableDiagnostic = false;       /// Flag to enable or disable diagnostic plots
  bool mFlagEnableOrphanPerChannel = false; /// Flag to enable the histogram of the orphan counter per channel
  ////////////////
  // Histograms //
  ////////////////

  // Event info
  std::shared_ptr<TH2F> mHistoOrbitID = nullptr;            /// Orbits seen
  std::shared_ptr<TH2F> mHistoBCID = nullptr;               /// Bunch crossings seen
  std::shared_ptr<TH2F> mHistoEventCounter = nullptr;       /// Event counters seen
  std::shared_ptr<TH2F> mHistoHitMap = nullptr;             /// TOF hit map (1 bin = 1 FEA = 24 channels)
  std::shared_ptr<TH2F> mHistoTimeVsBCID = nullptr;         /// TOF time vs BCID
  std::shared_ptr<TProfile2D> mHistoOrbitVsCrate = nullptr; /// Orbits per crate
  std::shared_ptr<TH1I> mHistoROWSize = nullptr;            /// Readout window size
  std::shared_ptr<TH2I> mHistoDecodingErrors = nullptr;     /// Decoding error monitoring
  std::shared_ptr<TH1S> mHistoOrphanPerChannel = nullptr;   /// Orphans per channel

  // Multiplicity
  std::shared_ptr<TH1I> mHistoMultiplicity = nullptr;          /// TOF raw hit multiplicity per event
  std::shared_ptr<TH1I> mHistoMultiplicityIA = nullptr;        /// TOF raw hit multiplicity per event - I/A side
  std::shared_ptr<TH1I> mHistoMultiplicityOA = nullptr;        /// TOF raw hit multiplicity per event - O/A side
  std::shared_ptr<TH1I> mHistoMultiplicityIC = nullptr;        /// TOF raw hit multiplicity per event - I/C side
  std::shared_ptr<TH1I> mHistoMultiplicityOC = nullptr;        /// TOF raw hit multiplicity per event - O/C side
  std::shared_ptr<TProfile> mHitMultiplicityVsCrate = nullptr; /// TOF raw hit multiplicity per event vs Crate

  // Time
  std::shared_ptr<TH1F> mHistoTime = nullptr;        /// TOF hit time (ns)
  std::shared_ptr<TH1F> mHistoTimeIA = nullptr;      /// TOF hit time (ns) - I/A side
  std::shared_ptr<TH1F> mHistoTimeOA = nullptr;      /// TOF hit time (ns) - O/A side
  std::shared_ptr<TH1F> mHistoTimeIC = nullptr;      /// TOF hit time (ns) - I/C side
  std::shared_ptr<TH1F> mHistoTimeOC = nullptr;      /// TOF hit time (ns) - O/C side
  std::shared_ptr<TH1F> mHistoTimeOrphans = nullptr; /// TOF hit time (ns) for orphan hits (missing trailer word)

  // ToT
  std::shared_ptr<TH1F> mHistoToT = nullptr;   /// TOF hit ToT (ns)
  std::shared_ptr<TH1F> mHistoToTIA = nullptr; /// TOF hit ToT (ns) - I/A side
  std::shared_ptr<TH1F> mHistoToTOA = nullptr; /// TOF hit ToT (ns) - O/A side
  std::shared_ptr<TH1F> mHistoToTIC = nullptr; /// TOF hit ToT (ns) - I/C side
  std::shared_ptr<TH1F> mHistoToTOC = nullptr; /// TOF hit ToT (ns) - O/C side

  // std::shared_ptr<TH2F> mTOFRawTimeVsTRM035 = nullptr;      /// TOF raws - Hit time vs TRM - crates 0 to 35
  // std::shared_ptr<TH2F> mTOFRawTimeVsTRM3671 = nullptr;     /// TOF raws - Hit time vs TRM - crates 36 to 72
  // std::shared_ptr<TH2F> mTOFTimeVsStrip = nullptr;          /// TOF raw hit time vs. MRPC (along z axis)
  // std::shared_ptr<TH2F> mTOFchannelEfficiencyMap = nullptr; /// TOF channels (HWok && efficient && !noisy && !problematic)
  // std::shared_ptr<TH2F> mTOFhitsCTTM = nullptr;             /// Map of hit pads according to CTTM numbering
  // std::shared_ptr<TH2F> mTOFmacropadCTTM = nullptr;         /// Map of hit macropads according to CTTM numbering
  // std::shared_ptr<TH2F> mTOFmacropadDeltaPhiTime = nullptr; /// #Deltat vs #Delta#Phi of hit macropads
  // std::shared_ptr<TH2I> mBXVsCttmBit = nullptr;             /// BX ID in TOF matching window vs trg channel
  // std::shared_ptr<TH2F> mTimeVsCttmBit = nullptr;           /// TOF raw time vs trg channel

  // Counters
  static constexpr unsigned int nchannels = RawDataDecoder::ncrates * RawDataDecoder::nstrips * 24; /// Number of channels
  Counter<RawDataDecoder::ncrates, nullptr> mHitCounterPerStrip[RawDataDecoder::nstrips];           /// Hit map counter in the crate, one per strip
  Counter<nchannels, nullptr> mHitCounterPerChannel;                                                /// Hit map counter in the single channel
  Counter<nchannels, nullptr> mOrphanCounterPerChannel;                                             /// Orphan counter in the single channel
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_TASK_DIGITS_H
