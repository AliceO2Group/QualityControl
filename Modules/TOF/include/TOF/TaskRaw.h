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
/// \file   TaskRaw.h
/// \author Nicolo' Jacazio and Francesca Ercolessi
/// \brief  Task To monitor data converted from TOF compressor, and check the diagnostic words of TOF crates received trough the TOF compressor.
///         Here are defined the counters to check the diagnostics words of the TOF crates obtained from the compressor.
///         This is why the class derives from DecoderBase: it reads data from the decoder.
/// \since  20-11-2020
///

#ifndef QC_MODULE_TOF_TASKRAW_H
#define QC_MODULE_TOF_TASKRAW_H

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
  void decode();

  /// Counters to fill
  static constexpr unsigned int ncrates = 72;    /// Number of crates
  static constexpr unsigned int ntrms = 10;      /// Number of TRMs per crate
  static constexpr unsigned int ntrmschains = 2; /// Number of TRMChains per TRM
  static constexpr unsigned int nsectors = 18;   /// Number of sectors
  static constexpr unsigned int nstrips = 91;    /// Number of strips per sector

  /// Initialize noise analysis variables
  Int_t mTimeMin = 0, mTimeMax = -1;
  Double_t max_noise = 1.e+3 /* [Hz] */, tdc_width = 24.3660e-12 /* [s] */;

  /// Set parameters for noise analysis
  void setTimeMin(std::string min) { mTimeMin = atoi(min.c_str()); }
  void setTimeMax(std::string max) { mTimeMax = atoi(max.c_str()); }
  void setMaxNoise(std::string thresholdnoise) { max_noise = atoi(thresholdnoise.c_str()); }

  // Names of diagnostic counters
  static const char* RDHDiagnosticsName[2]; /// RDH Counter names
  static const char* DRMDiagnosticName[32]; /// DRM Counter names
  static const char* LTMDiagnosticName[32]; /// LTM Counter names
  static const char* TRMDiagnosticName[32]; /// TRM Counter names
  // Diagnostic counters
  Counter<2, RDHDiagnosticsName> mRDHCounter[ncrates];        /// RDH Counters
  Counter<32, DRMDiagnosticName> mDRMCounter[ncrates];        /// DRM Counters
  Counter<32, LTMDiagnosticName> mLTMCounter[ncrates];        /// LTM Counters
  Counter<32, TRMDiagnosticName> mTRMCounter[ncrates][ntrms]; /// TRM Counters
  // Global counters
  Counter<172800, nullptr> mCounterIndexE;      /// Counter for the single electronic index
  Counter<172800, nullptr> mCounterIndexENoise; /// Counter for the single electronic index for noise analysis
  Counter<1024, nullptr> mCounterTimeBC;        /// Counter for the Bunch Crossing Time

  /// Histograms to fill
  std::map<std::string, std::shared_ptr<TH1>> mHistos;

  Int_t rdhread = 0; /// Number of times a RDH is read

  /// Function to init histograms
  void initHistograms();

  /// Function to reset histograms
  void resetHistograms();

  // Function for noise estimation
  void estimateNoise(std::shared_ptr<TH2F> hFeaStripNoise);

  // Histograms filled in the decoder to be kept to the bare bone so as to increase performance
  std::shared_ptr<TH1F> mHits;          /// Number of TOF hits
  std::shared_ptr<TH1F> mTime;          /// Time
  std::shared_ptr<TH1F> mTOT;           /// Time-Over-Threshold
  std::shared_ptr<TH2F> mSlotPartMask;  /// Participating slot
  std::shared_ptr<TH2F> mDiagnostic;    /// Diagnostic words
  std::shared_ptr<TH1F> mNErrors;       /// Number of errors
  std::shared_ptr<TH1F> mErrorBits;     /// Bits of errors
  std::shared_ptr<TH2F> mError;         /// Errors in slot and TDC
  std::shared_ptr<TH1F> mNTests;        /// Number of tests
  std::shared_ptr<TH2F> mTest;          /// Tests in slot and TDC
  std::shared_ptr<TH2F> mOrbitID;       /// Orbit ID for the header and trailer words
  std::shared_ptr<TH2F> mFeaStripNoise; /// Noise map per FEA cards

 private:
  /** decoding handlers **/
  void rdhHandler(const o2::header::RAWDataHeader* /*rdh*/) override{};
  void headerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* crateOrbit) override;
  void frameHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* crateOrbit,
                    const FrameHeader_t* frameHeader, const PackedHit_t* packedHits) override;
  void trailerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* crateOrbit,
                      const CrateTrailer_t* crateTrailer, const Diagnostic_t* diagnostics,
                      const Error_t* errors) override;
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
  std::shared_ptr<TH2F> mRDHHisto;                        /// Words per RDH
  std::shared_ptr<TH2F> mDRMHisto;                        /// Words per DRM
  std::shared_ptr<TH2F> mLTMHisto;                        /// Words per LTM
  std::shared_ptr<TH2F> mTRMHisto[RawDataDecoder::ntrms]; /// Words per TRM
  std::shared_ptr<TH1F> mIndexE;                          /// Index in electronic
  std::shared_ptr<TH1F> mIndexENoise;                     /// Index in electronic for noise analysis
  std::shared_ptr<TH1F> mTimeBC;                          /// Time in Bunch Crossing

  RawDataDecoder mDecoderRaw; /// Decoder for TOF Compressed data useful for the Task and filler of histograms for compressed raw data
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_TASKRAW_H