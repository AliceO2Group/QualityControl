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
/// \file   Diagnostics.h
/// \author Nicolo' Jacazio
/// \brief Here are defined the counters to check the diagnostics words of the TOF crates obtained from the compressor. This is why the Diagnostics class derives from DecoderBase: it reads data from the decoder.
///

#ifndef QC_MODULE_TOF_DIAGNOSTICS_H
#define QC_MODULE_TOF_DIAGNOSTICS_H

// O2 includes
#include "TOFReconstruction/DecoderBase.h"
#include "DataFormatsTOF/CompressedDataFormat.h"
using namespace o2::tof::compressed;

// QC includes
#include "Base/Counter.h"

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::tof
{
namespace counters
{
// Structs for counters
/// RDH counters: there will only be one instance of such counters per crate
struct ERDHCounter_t {
  /// Number of RDH counters
  static const UInt_t size = 2;
  /// Name of RDH counters
  static const TString names[size];
};

/// DRM counters: there will only be one instance of such counters per crate
struct EDRMCounter_t {
  /// Number of DRM counters
  static const UInt_t size = 17;
  /// Name of DRM counters
  static const TString names[size];
};

/// LTM counters: there will only be ten instance of such counters per crate
struct ELTMCounter_t {
  /// Number of LTM counters
  static const UInt_t size = 17;
  /// Name of LTM counters
  static const TString names[size];
};

/// TRM counters: there will only be ten instance of such counters per crate
struct ETRMCounter_t {
  /// Number of TRM counters
  static const UInt_t size = 17;
  /// Name of TRM counters
  static const TString names[size];
};

/// TRMChain: counters there will be 20 instances of such counters per crate
struct ETRMChainCounter_t {
  /// Number of TRMChain counters
  static const UInt_t size = 33;
  /// Name of TRMChain counters
  static const TString names[size];
};

} // namespace counters

using namespace counters;

/// \brief TOF Quality Control class for Decoding Compressed data for TOF Compressed data QC Task
/// \author Nicolo' Jacazio
class Diagnostics final
  : public DecoderBase
{
 public:
  /// \brief Constructor
  Diagnostics() = default;
  /// Destructor
  ~Diagnostics() = default;

  /// Function to run decoding
  void decode();

  /// Counters to fill
  static const int ncrates = 72;                                             /// Number of crates
  static const int ntrms = 10;                                               /// Number of TRMs per crate
  static const int ntrmschains = 2;                                          /// Number of TRMChains per TRM
  Counter<ERDHCounter_t> mRDHCounter[ncrates];                               /// RDH Counters
  Counter<EDRMCounter_t> mDRMCounter[ncrates];                               /// DRM Counters
  Counter<ELTMCounter_t> mLTMCounter[ncrates];                               /// LTM Counters
  Counter<ETRMCounter_t> mTRMCounter[ncrates][ntrms];                        /// TRM Counters
  Counter<ETRMChainCounter_t> mTRMChainCounter[ncrates][ntrms][ntrmschains]; /// TRMChain Counters

 private:
  /** decoding handlers **/
  // void rdhHandler(const o2::header::RAWDataHeader* rdh) override;
  void headerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* crateOrbit) override;
  // void frameHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* crateOrbit,
  //                   const FrameHeader_t* frameHeader, const PackedHit_t* packedHits) override;
  void trailerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* crateOrbit,
                      const CrateTrailer_t* crateTrailer, const Diagnostic_t* diagnostics,
                      const Error_t* errors) override;
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_DIAGNOSTICS_H
