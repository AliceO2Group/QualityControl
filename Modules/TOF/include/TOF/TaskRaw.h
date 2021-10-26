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
/// \file   TaskRaw.h
/// \author Nicolo' Jacazio and Francesca Ercolessi
/// \brief  Task To monitor data converted from TOF compressor, and check the diagnostic words of TOF crates received trough the TOF compressor.
///         Here are defined the counters to check the diagnostics words of the TOF crates obtained from the compressor.
///         This is why the class derives from DecoderBase: it reads data from the decoder.
///         This tasks also perform a basic noise monitoring to check the fraction of noisy channels
/// \since  20-11-2020
///

#ifndef QC_MODULE_TOF_TASK_RAW_H
#define QC_MODULE_TOF_TASK_RAW_H

// O2 includes
#include "TOFReconstruction/DecoderBase.h"
#include "DataFormatsTOF/CompressedDataFormat.h"
#include "TOFBase/Geo.h"
using namespace o2::tof::compressed;

// QC includes
#include "QualityControl/TaskInterface.h"
#include "Base/Counter.h"
using namespace o2::quality_control::core;

class TH1;
class TH1F;
class TH2F;

namespace o2::quality_control_modules::tof
{

/// \brief TOF Quality Control class for Decoding Compressed data for TOF Compressed data QC Task
/// \author Nicolo' Jacazio and Francesca Ercolessi
class RawDataDecoder final : public DecoderBase
{
 public:
  /// \brief Constructor
  RawDataDecoder() = default;
  /// Destructor
  ~RawDataDecoder() = default;

  /// Function to run decoding of raw data
  void decode() { DecoderBase::run(); };

  /// Counters to fill
  static constexpr unsigned int ncrates = 72;         /// Number of crates
  static constexpr unsigned int ntrms = 10;           /// Number of TRMs per crate
  static constexpr unsigned int ntrmschains = 2;      /// Number of TRMChains per TRM
  static constexpr unsigned int nsectors = 18;        /// Number of sectors
  static constexpr unsigned int nstrips = 91;         /// Number of strips per sector
  static constexpr unsigned int nwords = 32;          /// Number of diagnostic words of a slot card
  static constexpr unsigned int nslots = 12;          /// Number of slots in a crate
  static constexpr unsigned int nequipments = 172800; /// Number of equipment in the electronic indexing scheme
  static constexpr unsigned int nRDHwords = 3;        /// Number of diagnostic words for RDH

  /// Set parameters for noise analysis
  void setTimeWindowMin(std::string min) { mTimeMin = atoi(min.c_str()); }
  void setTimeWindowMax(std::string max) { mTimeMax = atoi(max.c_str()); }
  void setNoiseThreshold(std::string thresholdnoise) { mNoiseThreshold = atof(thresholdnoise.c_str()); }
  void setDebugCrateMultiplicity(bool debug) { mDebugCrateMultiplicity = debug; }
  const bool& isDebugCrateMultiplicity() const { return mDebugCrateMultiplicity; }

  // Names of diagnostic counters
  static const char* RDHDiagnosticsName[nRDHwords]; /// RDH Counter names
  static const char* DRMDiagnosticName[nwords];     /// DRM Counter names
  static const char* LTMDiagnosticName[nwords];     /// LTM Counter names
  static const char* TRMDiagnosticName[nwords];     /// TRM Counter names
  // Diagnostic counters
  Counter<nRDHwords, RDHDiagnosticsName> mCounterRDH[ncrates];    /// RDH Counters
  Counter<nwords, DRMDiagnosticName> mCounterDRM[ncrates];        /// DRM Counters
  Counter<nwords, LTMDiagnosticName> mCounterLTM[ncrates];        /// LTM Counters
  Counter<nwords, TRMDiagnosticName> mCounterTRM[ncrates][ntrms]; /// TRM Counters
  // Global counters
  Counter<nequipments, nullptr> mCounterIndexEO;          /// Counter for the single electronic index
  Counter<nequipments, nullptr> mCounterIndexEOInTimeWin; /// Counter for the single electronic index for noise analysis
  Counter<nequipments, nullptr> mCounterNoisyChannels;    /// Counter for noisy channels
  Counter<1024, nullptr> mCounterTimeBC;                  /// Counter for the Bunch Crossing Time
  Counter<nstrips, nullptr> mCounterNoiseMap[ncrates][4]; /// Counter for the Noise Hit Map, counts per crate and per FEA (4 per strip)
  Counter<ncrates, nullptr> mCounterRDHTriggers[2];       /// Counter for RDH triggers, one counts the triggers served to TDCs and one counts the triggers received
  Counter<ncrates, nullptr> mCounterRDHOpen;              /// Counter for RDH open
  Counter<800, nullptr> mCounterOrbitsPerCrate[ncrates];  /// Counter for orbits per crate

  /// Function to init histograms
  void initHistograms();

  /// Function to reset histograms
  void resetHistograms();

  // Function for noise estimation
  void estimateNoise(std::shared_ptr<TH1F> hIndexEOIsNoise);

  // Histograms filled in the decoder to be kept to the bare bone so as to increase performance
  std::shared_ptr<TH1I> mHistoHits;               /// Number of TOF hits
  std::shared_ptr<TH1I> mHistoHitsCrate[ncrates]; /// Number of TOF hits in TRMs per Crate
  std::shared_ptr<TH1F> mHistoTime;               /// Time
  std::shared_ptr<TH1F> mHistoTOT;                /// Time-Over-Threshold
  std::shared_ptr<TH2F> mHistoDiagnostic;         /// Diagnostic words
  std::shared_ptr<TH1F> mHistoNErrors;            /// Number of errors
  std::shared_ptr<TH1F> mHistoErrorBits;          /// Bits of errors
  std::shared_ptr<TH2F> mHistoError;              /// Errors in slot and TDC
  std::shared_ptr<TH1F> mHistoNTests;             /// Number of tests
  std::shared_ptr<TH2F> mHistoTest;               /// Tests in slot and TDC
  std::shared_ptr<TH2F> mHistoOrbitID;            /// Orbit ID for the header and trailer words
  std::shared_ptr<TH2F> mHistoNoiseMap;           /// Noise map, one bin corresponds to one FEA card
  std::shared_ptr<TH1F> mHistoIndexEOHitRate;     /// Noise rate x channel

 private:
  /** decoding handlers **/
  void rdhHandler(const o2::header::RAWDataHeader* /*rdh*/) override;
  void headerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* crateOrbit) override;
  void frameHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* crateOrbit,
                    const FrameHeader_t* frameHeader, const PackedHit_t* packedHits) override;
  void trailerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* crateOrbit,
                      const CrateTrailer_t* crateTrailer, const Diagnostic_t* diagnostics,
                      const Error_t* errors) override;

  // Decoder parameters
  /// Noise analysis variables
  int mTimeMin = 0;                                /// Start of the time window in bins of the TDC
  int mTimeMax = -1;                               /// End of the time window in bins of the TDC
  static constexpr double mTDCWidth = 24.3660e-12; /// Width of the TDC bins in [s]
  double mNoiseThreshold = 1.e+3;                  /// Threshold used to define noisy channels [Hz]
  bool mDebugCrateMultiplicity = false;            /// Save 72 histo with multiplicity per crate
};

/// \brief TOF Quality Control DPL Task for TOF Compressed data
/// \author Nicolo' Jacazio and Francesca Ercolessi
class TaskRaw final : public TaskInterface
{
 public:
  /// \brief Constructor
  TaskRaw() = default;
  /// Destructor
  ~TaskRaw() override = default;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  // Histograms
  // Diagnostic words
  std::shared_ptr<TH2F> mHistoRDH;                            /// Words per RDH
  std::shared_ptr<TH2F> mHistoDRM;                            /// Words per DRM
  std::shared_ptr<TH2F> mHistoLTM;                            /// Words per LTM
  std::shared_ptr<TH2F> mHistoTRM[RawDataDecoder::ntrms];     /// Words per TRM
  std::shared_ptr<TH2F> mHistoCrate[RawDataDecoder::ncrates]; /// Words of each slot in a crate
  std::shared_ptr<TH2F> mHistoSlotParticipating;              /// Participating slot per crate

  // Indices in the electronic scheme
  std::shared_ptr<TH1F> mHistoIndexEO;          /// Index in electronic
  std::shared_ptr<TH1F> mHistoIndexEOInTimeWin; /// Index in electronic for noise analysis
  std::shared_ptr<TH1F> mHistoIndexEOIsNoise;   /// Noise hit map x channel
  std::shared_ptr<TH1F> mHistoRDHTriggers;      /// RDH trigger efficiency, ratio of total triggers served to total triggers received per crate
  std::shared_ptr<TH2F> mHistoOrbitsPerCrate;   /// Orbit per crate

  // Other observables
  std::shared_ptr<TH1F> mHistoTimeBC; /// Time in Bunch Crossing

  RawDataDecoder mDecoderRaw; /// Decoder for TOF Compressed data useful for the Task and filler of histograms for compressed raw data
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_TASK_RAW_H