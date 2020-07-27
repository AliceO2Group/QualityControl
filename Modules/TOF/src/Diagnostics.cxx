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
/// \file   Diagnostics.cxx
/// \author Nicolo' Jacazio
/// \brief Here are defined the counters to check the diagnostics words of the TOF crates obtained from the compressor. This is why the Diagnostics class derives from DecoderBase: it reads data from the decoder.
///

// O2 includes
#include "DataFormatsTOF/CompressedDataFormat.h"
#include <Framework/DataRefUtils.h>
#include "Headers/RAWDataHeader.h"
#include "DetectorsRaw/HBFUtils.h"

using namespace o2::framework;

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TOF/Diagnostics.h"

namespace o2::quality_control_modules::tof
{

namespace counters
{
// These are the translation between the messages in O2/DataFormats/Detectors/TOF/include/DataFormatsTOF/CompressedDataFormat.h and QC counters
const TString ERDHCounter_t::names[ERDHCounter_t::size] = {
  "RDH_HAS_DATA",
  ""
};

const TString EDRMCounter_t::names[EDRMCounter_t::size] = {
  "DRM_HAS_DATA",               // 0 DRM has read some data
  "",                           // 1 Empty for now
  "",                           // 2 Empty for now
  "",                           // 3 Empty for now
  "DRM_HEADER_MISSING",         // 4
  "DRM_TRAILER_MISSING",        // 5
  "DRM_FEEID_MISMATCH",         // 6
  "DRM_ORBIT_MISMATCH",         // 7
  "DRM_CRC_MISMATCH",           // 8
  "DRM_ENAPARTMASK_DIFFER",     // 9
  "DRM_CLOCKSTATUS_WRONG",      // 10
  "DRM_FAULTSLOTMASK_NOTZERO",  // 11
  "DRM_READOUTTIMEOUT_NOTZERO", // 12
  "DRM_EVENTWORDS_MISMATCH",    // 13
  "",                           // 14
  ""                            // 15
};
static_assert(TESTBIT(o2::tof::diagnostic::DRM_HEADER_MISSING, 4));
static_assert(TESTBIT(o2::tof::diagnostic::DRM_TRAILER_MISSING, 5));
static_assert(TESTBIT(o2::tof::diagnostic::DRM_FEEID_MISMATCH, 6));
static_assert(TESTBIT(o2::tof::diagnostic::DRM_ORBIT_MISMATCH, 7));
static_assert(TESTBIT(o2::tof::diagnostic::DRM_CRC_MISMATCH, 8));
static_assert(TESTBIT(o2::tof::diagnostic::DRM_ENAPARTMASK_DIFFER, 9));
static_assert(TESTBIT(o2::tof::diagnostic::DRM_CLOCKSTATUS_WRONG, 10));
static_assert(TESTBIT(o2::tof::diagnostic::DRM_FAULTSLOTMASK_NOTZERO, 11));
static_assert(TESTBIT(o2::tof::diagnostic::DRM_READOUTTIMEOUT_NOTZERO, 12));
static_assert(TESTBIT(o2::tof::diagnostic::DRM_EVENTWORDS_MISMATCH, 13));
static_assert(TESTBIT(o2::tof::diagnostic::DRM_MAXDIAGNOSTIC_BIT, EDRMCounter_t::size));

const TString ELTMCounter_t::names[ELTMCounter_t::size] = {
  "LTM_HAS_DATA",          // 0 LTM has read some data
  "",                      // 1 Empty for now
  "",                      // 2 Empty for now
  "",                      // 3 Empty for now
  "LTM_HEADER_MISSING",    // 4
  "LTM_TRAILER_MISSING",   // 5
  "",                      // 6
  "LTM_HEADER_UNEXPECTED", // 7
  "",                      // 8
  "",                      // 9
  "",                      // 10
  "",                      // 11
  "",                      // 12
  "",                      // 13
  "",                      // 14
  ""                       // 15
};
static_assert(TESTBIT(o2::tof::diagnostic::LTM_HEADER_MISSING, 4));
static_assert(TESTBIT(o2::tof::diagnostic::LTM_TRAILER_MISSING, 5));
static_assert(TESTBIT(o2::tof::diagnostic::LTM_HEADER_UNEXPECTED, 7));
static_assert(TESTBIT(o2::tof::diagnostic::LTM_MAXDIAGNOSTIC_BIT, ELTMCounter_t::size));

const TString ETRMCounter_t::names[ETRMCounter_t::size] = {
  "TRM_HAS_DATA",             // 0 TRM has read some data
  "",                         // 1 Empty for now
  "",                         // 2 Empty for now
  "",                         // 3 Empty for now
  "TRM_HEADER_MISSING",       // 4
  "TRM_TRAILER_MISSING",      // 5
  "TRM_CRC_MISMATCH",         // 6
  "TRM_HEADER_UNEXPECTED",    // 7
  "TRM_EVENTCNT_MISMATCH",    // 8
  "TRM_EMPTYBIT_NOTZERO",     // 9
  "TRM_LBIT_NOTZERO",         // 10
  "TRM_FAULTSLOTBIT_NOTZERO", // 11
  "TRM_EVENTWORDS_MISMATCH",  // 12
  "TRM_DIAGNOSTIC_SPARE1",    // 13
  "TRM_DIAGNOSTIC_SPARE2",    // 14
  "TRM_DIAGNOSTIC_SPARE3"     // 15
};
static_assert(TESTBIT(o2::tof::diagnostic::TRM_HEADER_MISSING, 4));
static_assert(TESTBIT(o2::tof::diagnostic::TRM_TRAILER_MISSING, 5));
static_assert(TESTBIT(o2::tof::diagnostic::TRM_CRC_MISMATCH, 6));
static_assert(TESTBIT(o2::tof::diagnostic::TRM_HEADER_UNEXPECTED, 7));
static_assert(TESTBIT(o2::tof::diagnostic::TRM_EVENTCNT_MISMATCH, 8));
static_assert(TESTBIT(o2::tof::diagnostic::TRM_EMPTYBIT_NOTZERO, 9));
static_assert(TESTBIT(o2::tof::diagnostic::TRM_LBIT_NOTZERO, 10));
static_assert(TESTBIT(o2::tof::diagnostic::TRM_FAULTSLOTBIT_NOTZERO, 11));
static_assert(TESTBIT(o2::tof::diagnostic::TRM_EVENTWORDS_MISMATCH, 12));
static_assert(TESTBIT(o2::tof::diagnostic::TRM_DIAGNOSTIC_SPARE1, 13));
static_assert(TESTBIT(o2::tof::diagnostic::TRM_DIAGNOSTIC_SPARE2, 14));
static_assert(TESTBIT(o2::tof::diagnostic::TRM_DIAGNOSTIC_SPARE3, 15));
static_assert(TESTBIT(o2::tof::diagnostic::TRM_MAXDIAGNOSTIC_BIT, ETRMCounter_t::size));

const TString ETRMChainCounter_t::names[ETRMChainCounter_t::size] = {
  "TRMCHAIN_HAS_DATA",            // 0 TRM Chain read some data
  "",                             // 1 Empty for now
  "",                             // 2 Empty for now
  "",                             // 3 Empty for now
  "",                             // 4
  "",                             // 5
  "",                             // 6
  "",                             // 7
  "",                             // 8
  "",                             // 9
  "",                             // 10
  "",                             // 11
  "",                             // 12
  "",                             // 13
  "",                             // 14
  "",                             // 15
  "TRMCHAIN_A_HEADER_MISSING",    // 16
  "TRMCHAIN_A_TRAILER_MISSING",   // 17
  "TRMCHAIN_A_STATUS_NOTZERO",    // 18
  "TRMCHAIN_A_EVENTCNT_MISMATCH", // 19
  "TRMCHAIN_A_TDCERROR_DETECTED", // 20
  "TRMCHAIN_A_BUNCHCNT_MISMATCH", // 21
  "TRMCHAIN_A_DIAGNOSTIC_SPARE1", // 22
  "TRMCHAIN_A_DIAGNOSTIC_SPARE2", // 23
  "TRMCHAIN_B_HEADER_MISSING",    // 24
  "TRMCHAIN_B_TRAILER_MISSING",   // 25
  "TRMCHAIN_B_STATUS_NOTZERO",    // 26
  "TRMCHAIN_B_EVENTCNT_MISMATCH", // 27
  "TRMCHAIN_B_TDCERROR_DETECTED", // 28
  "TRMCHAIN_B_BUNCHCNT_MISMATCH", // 29
  "TRMCHAIN_B_DIAGNOSTIC_SPARE1", // 30
  "TRMCHAIN_B_DIAGNOSTIC_SPARE2"  // 31
};
static_assert(TESTBIT(o2::tof::diagnostic::TRMCHAIN_HEADER_MISSING, 16));
static_assert(TESTBIT(o2::tof::diagnostic::TRMCHAIN_TRAILER_MISSING, 17));
static_assert(TESTBIT(o2::tof::diagnostic::TRMCHAIN_STATUS_NOTZERO, 18));
static_assert(TESTBIT(o2::tof::diagnostic::TRMCHAIN_EVENTCNT_MISMATCH, 19));
static_assert(TESTBIT(o2::tof::diagnostic::TRMCHAIN_TDCERROR_DETECTED, 20));
static_assert(TESTBIT(o2::tof::diagnostic::TRMCHAIN_BUNCHCNT_MISMATCH, 21));
static_assert(TESTBIT(o2::tof::diagnostic::TRMCHAIN_DIAGNOSTIC_SPARE1, 22));
static_assert(TESTBIT(o2::tof::diagnostic::TRMCHAIN_DIAGNOSTIC_SPARE2, 23));
static_assert(TESTBIT(o2::tof::diagnostic::TRMCHAIN_MAXDIAGNOSTIC_BIT, ETRMChainCounter_t::size - 8));

} // namespace counters
void Diagnostics::decode()
{
  DecoderBase::run();
}

void Diagnostics::headerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* /*crateOrbit*/)
{
  mDRMCounter[crateHeader->drmID].Count(0);
  if (crateHeader->slotPartMask & 1 << 0) {
    mLTMCounter[crateHeader->drmID].Count(0);
  }
  for (Int_t i = 1; i < 11; i++) {
    if (crateHeader->slotPartMask & 1 << i) {
      mTRMCounter[crateHeader->drmID][i - 1].Count(0);
    }
  }
}

void Diagnostics::trailerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* /*crateOrbit*/,
                                 const CrateTrailer_t* crateTrailer, const Diagnostic_t* diagnostics,
                                 const Error_t* /*errors*/)
{
  const Int_t drmID = crateHeader->drmID;
  const Int_t words_to_check = 32 - 4;
  for (int i = 0; i < crateTrailer->numberOfDiagnostics; ++i) {
    auto diagnostic = diagnostics + i;
    const Int_t slotID = diagnostic->slotID;
    if (slotID == 1) { // Here we have a DRM
      for (Int_t j = 0; j < words_to_check; j++) {
        if (diagnostic->faultBits & 1 << j) {
          mDRMCounter[drmID].Count(j + 4);
        }
      }
    } else if (slotID == 2) { // Here we have a LTM
      for (Int_t j = 0; j < words_to_check; j++) {
        if (diagnostic->faultBits & 1 << j) {
          mLTMCounter[drmID].Count(j + 4);
        }
      }
    } else { // Here we have a TRM
      const Int_t trmID = slotID - 3;
      for (Int_t j = 0; j < words_to_check; j++) {
        if (diagnostic->faultBits & 1 << j) {
          if (j < 16) { // TRM board
            mTRMCounter[drmID][trmID].Count(j + 4);
          } else if (j < 24) { // Readout chain A
            mTRMChainCounter[drmID][trmID][0].Count(j + 4);
          } else { // Readout chain B
            mTRMChainCounter[drmID][trmID][1].Count(j + 4);
          }
        }
      }
    }
  }
}

} // namespace o2::quality_control_modules::tof
