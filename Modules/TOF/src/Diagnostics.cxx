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
const TString ERDHCounter_t::names[ERDHCounter_t::size] = {
  "counterA",
  "counterB"
};

const TString EDRMCounter_t::names[EDRMCounter_t::size] = {
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

const TString ETRMCounter_t::names[ETRMCounter_t::size] = {
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

const TString ETRMChainCounter_t::names[ETRMChainCounter_t::size] = {
  "counterA",
  "counterB"
};
} // namespace counters
void Diagnostics::decode()
{
  DecoderBase::run();
}

void Diagnostics::headerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* /*crateOrbit*/)
{
  // mDRMCounter[crateHeader->drmID].Count(counters::kDRM_HAS_DATA);
  mDRMCounter[crateHeader->drmID].Count(0);
  for (Int_t i = 1; i < 11; i++) {
    if (crateHeader->slotPartMask & 1 << i) { // Magari includere l'LTM come i==0
      // mTRMCounter[crateHeader->drmID][i - 1].Count(counters::kTRM_HAS_DATA);
      mTRMCounter[crateHeader->drmID][i - 1].Count(0);
    }
  }
}

void Diagnostics::trailerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* /*crateOrbit*/,
                                 const CrateTrailer_t* crateTrailer, const Diagnostic_t* diagnostics,
                                 const Error_t* /*errors*/)
{
  LOG(INFO) << "DECODING!!!" << ENDM;
  Int_t drmID = crateHeader->drmID;
  // mDRMCounter[drmID].Count(counters::kDRM_HAS_DATA);
  for (int i = 0; i < crateTrailer->numberOfDiagnostics; ++i) {
    auto diagnostic = diagnostics + i;
    const Int_t slotID = diagnostic->slotID;
    if (slotID == 1) { // Here we have a DRM
      for (Int_t j = 0; j < 28; j++) {
        if (diagnostic->faultBits & 1 << j) {
          mDRMCounter[drmID].Count(j + 4);
        }
      }
    } else if (slotID == 2) { // Here we have a LTM

    } else { // Here we have a TRM
      Int_t trmID = slotID - 3;
      for (Int_t j = 0; j < 28; j++) {
        if (diagnostic->faultBits & 1 << j) {
          mTRMCounter[drmID][trmID].Count(j + 4);
        }
      }
    }
    // mRDHCounter[drmID].Count(counters::kRDHCounter_Data);
  }
}

} // namespace o2::quality_control_modules::tof
