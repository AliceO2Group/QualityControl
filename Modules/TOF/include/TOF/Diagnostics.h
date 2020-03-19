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
// Enums for counters

/// RDH counters: there will only be one instance of such counters per crate
enum ERDHCounter_t {
  kNRDHCounters = 2
};
/// Name of RDH counters
static const TString RDHCounterName[kNRDHCounters] = { "counterA", "counterB" };

/// Number of DRM counters
enum EDRMCounter_t {
  kNDRMCounters = 14
};
/// Name of DRM counters
static const TString DRMCounterName[kNDRMCounters] = {
  "DRM_HAS_DATA", // DRM has read some data
  "",             // Empty for now
  "",             // Empty for now
  "",             // Empty for now
  "DRM_HEADER_MISSING",
  "DRM_TRAILER_MISSING",
  "DRM_FEEID_MISMATCH",
  "DRM_ORBIT_MISMATCH",
  "DRM_CRC_MISMATCH",
  "DRM_ENAPARTMASK_DIFFER",
  "DRM_CLOCKSTATUS_WRONG",
  "DRM_FAULTSLOTMASK_NOTZERO",
  "DRM_READOUTTIMEOUT_NOTZERO",
  "DRM_MAXDIAGNOSTIC_BIT"
};

/// Number of TRM counters
enum ETRMCounter_t {
  kNTRMCounters = 13
};
/// Name of TRM counters
static const TString TRMCounterName[kNTRMCounters] = {
  "TRM_HAS_DATA", // TRM has read some data
  "",             // Empty for now
  "",             // Empty for now
  "",             // Empty for now
  "TRM_HEADER_MISSING",
  "TRM_TRAILER_MISSING",
  "TRM_CRC_MISMATCH",
  "TRM_HEADER_UNEXPECTED",
  "TRM_EVENTCNT_MISMATCH",
  "TRM_EMPTYBIT_NOTZERO",
  "TRM_LBIT_NOTZERO",
  "TRM_FAULTSLOTBIT_NOTZERO",
  "TRM_MAXDIAGNOSTIC_BIT"
};

/// TRM Chain: counters there will be 20 instances of such counters per crate
enum ETRMChainCounter_t {
  kNTRMChainCounters = 2
};
static const TString TRMChainCounterName[kNTRMChainCounters] = { "counterA", "counterB" };

} // namespace counters

using namespace counters;

/// \brief TOF Quality Control class for Decoding Compressed data for TOF Compressed data QC Task
/// \author Nicolo' Jacazio
class Diagnostics /*final*/
  : public DecoderBase
// todo add back the "final" when doxygen is fixed
{
 public:
  /// \brief Constructor
  Diagnostics() = default;
  /// Destructor
  ~Diagnostics() = default;

  /// Function to run decoding
  void decode();

  /// Counters to fill
  static const int ncrates = 72;
  static const int ntrms = 10;
  static const int ntrmschains = 2;
  Counter<ERDHCounter_t, kNRDHCounters, RDHCounterName> mRDHCounter[ncrates];
  Counter<EDRMCounter_t, kNDRMCounters, DRMCounterName> mDRMCounter[ncrates];
  Counter<ETRMCounter_t, kNTRMCounters, TRMCounterName> mTRMCounter[ncrates][ntrms];
  Counter<ETRMChainCounter_t, kNTRMChainCounters, TRMChainCounterName> mTRMChainCounter[ncrates][ntrms][ntrmschains];

 private:
  /** decoding handlers **/
  // void rdhHandler(const o2::header::RAWDataHeader* rdh) override;
  void headerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* crateOrbit) override;
  // void frameHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* crateOrbit,
  //                   const FrameHeader_t* frameHeader, const PackedHit_t* packedHits) override;
  void trailerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* crateOrbit,
                      const CrateTrailer_t* crateTrailer, const Diagnostic_t* diagnostics) override;
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_DIAGNOSTICS_H
