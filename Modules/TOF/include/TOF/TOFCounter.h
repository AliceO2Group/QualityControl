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
/// \file   TOFCounter.h
/// \author Nicolo' Jacazio
///

#ifndef QC_MODULE_TOF_TOFCOUNTER_H
#define QC_MODULE_TOF_TOFCOUNTER_H

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
// Enums for counters
/// RDH counters: there will only be one instance of such counters per crate
enum ERDHCounter_t {
  kRDHCounter_Data,
  kRDHCounter_Error,
  kNRDHCounters
};
static const TString RDHCounterName[kNRDHCounters] = { "counterA", "counterB" };

/// TRM counters: there will be 10 instances of such counters per crate
enum ETRMCounter_t {
  kTRMCounter_Data,
  kTRMCounter_Error,
  kNTRMCounters
};
/// Name of TRM counters
static const TString TRMCounterName[kNTRMCounters] = { "counterA", "counterB" };

/// TRM Chain: counters there will be 20 instances of such counters per crate
enum ETRMChainCounter_t {
  kTRMChainCounter_Data,
  kTRMChainCounter_Error,
  kNTRMChainCounters
};
/// Name of TRMChain counters
static const TString TRMChainCounterName[kNTRMChainCounters] = { "counterA", "counterB" };

/// DRM counters: there will only be one instance of such counters per crate
enum EDRMCounter_t {
  kDRMCounter_A,
  kDRMCounter_B,
  kNDRMCounters
};
/// Name of DRM counters
static const TString DRMCounterName[kNDRMCounters] = { "counterA", "counterB" };
} // namespace counters

/// \brief TOF Quality Control struct to contain all the counters for the words created by a crate
/// \author Nicolo' Jacazio
struct CrateCounter {
  Counter<counters::ERDHCounter_t, counters::kNRDHCounters> mRDHCounter;
  Counter<counters::EDRMCounter_t, counters::kNDRMCounters> mDRMCounter;
  Counter<counters::ETRMCounter_t, counters::kNTRMCounters> mTRMCounter[10];
  Counter<counters::ETRMChainCounter_t, counters::kNTRMChainCounters> mTRMChainCounter[10][2];
};

/// \brief TOF Quality Control class for Decoding Compressed data for TOF Compressed data QC Task
/// \author Nicolo' Jacazio
class TOFCounter /*final*/
  : public DecoderBase
// todo add back the "final" when doxygen is fixed
{
 public:
  /// \brief Constructor
  TOFCounter() = default;
  /// Destructor
  ~TOFCounter() = default;

  /// Function to run decoding
  void decode();

  /// Counters to fill
  static const int ncrates = 72;
  static const int ntrms = 10;
  static const int ntrmschains = 2;
  // static const int nslots = 14;
  // CounterMatrix<ncrates, nslots, ECrateCounter_t, kNCrateCounters> mDiagnostic;
  // CrateCounter crate_counter[ncrates];
  Counter<counters::ERDHCounter_t, counters::kNRDHCounters> mRDHCounter[ncrates];
  Counter<counters::EDRMCounter_t, counters::kNDRMCounters> mDRMCounter[ncrates];
  Counter<counters::ETRMCounter_t, counters::kNTRMCounters> mTRMCounter[ncrates][ntrms];
  Counter<counters::ETRMChainCounter_t, counters::kNTRMChainCounters> mTRMChainCounter[ncrates][ntrms][ntrmschains];

 private:
  /** decoding handlers **/
  // void rdhHandler(const o2::header::RAWDataHeader* rdh) override;
  // void headerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* crateOrbit) override;
  // void frameHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* crateOrbit,
  //                   const FrameHeader_t* frameHeader, const PackedHit_t* packedHits) override;
  void trailerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* crateOrbit,
                      const CrateTrailer_t* crateTrailer, const Diagnostic_t* diagnostics) override;
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_TOFCOUNTER_H
