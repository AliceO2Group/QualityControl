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
/// \file   TaskCompressedData.h
/// \author Nicolo' Jacazio
/// \brief  Task To monitor data converted from TOF compressor, it implements a dedicated decoder from DecoderBase
///

#ifndef QC_MODULE_TOF_TASKCOMPRESSEDDATA_H
#define QC_MODULE_TOF_TASKCOMPRESSEDDATA_H

// O2 includes
#include "TOFReconstruction/DecoderBase.h"
#include "DataFormatsTOF/CompressedDataFormat.h"
using namespace o2::tof::compressed;

// QC includes
#include "QualityControl/TaskInterface.h"

class TH1;
class TH1F;
class TH2F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::tof
{

/// \brief TOF Quality Control class for Decoding Compressed data for TOF Compressed data QC Task
/// \author Nicolo' Jacazio
class CompressedDataDecoder final : public DecoderBase
{
 public:
  /// \brief Constructor
  CompressedDataDecoder() = default;
  /// Destructor
  ~CompressedDataDecoder() = default;

  /// Function to run decoding
  void decode();

  /// Histograms to fill
  std::map<std::string, std::shared_ptr<TH1>> mHistos;

  Int_t rdhread = 0; /// Number of times a RDH is read

 private:
  /** decoding handlers **/
  void rdhHandler(const o2::header::RAWDataHeader* rdh) override;
  void headerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* crateOrbit) override;
  void frameHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* crateOrbit,
                    const FrameHeader_t* frameHeader, const PackedHit_t* packedHits) override;
  void trailerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* crateOrbit,
                      const CrateTrailer_t* crateTrailer, const Diagnostic_t* diagnostics,
                      const Error_t* errors) override;
};

/// \brief TOF Quality Control DPL Task for TOF Compressed data
/// \author Nicolo' Jacazio
class TaskCompressedData final : public TaskInterface
{
 public:
  /// \brief Constructor
  TaskCompressedData() = default;
  /// Destructor
  ~TaskCompressedData() override = default;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  CompressedDataDecoder mDecoder;      /// Decoder for TOF Compressed data useful for the Task
  std::shared_ptr<TH1F> mHits;         /// Number of TOF hits
  std::shared_ptr<TH1F> mTime;         /// Time
  std::shared_ptr<TH1F> mTimeBC;       /// Time in Bunch Crossing
  std::shared_ptr<TH1F> mTOT;          /// Time-Over-Threshold
  std::shared_ptr<TH1F> mIndexE;       /// Index in electronic
  std::shared_ptr<TH2F> mSlotPartMask; /// Participating slot
  std::shared_ptr<TH2F> mDiagnostic;   /// Diagnostic words
  std::shared_ptr<TH1F> mNErrors;      /// Number of errors
  std::shared_ptr<TH1F> mErrorBits;    /// Bits of errors
  std::shared_ptr<TH2F> mError;        /// Errors in slot and TDC
  std::shared_ptr<TH1F> mNTests;       /// Number of tests
  std::shared_ptr<TH2F> mTest;         /// Tests in slot and TDC
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_TASKCOMPRESSEDDATA_H
