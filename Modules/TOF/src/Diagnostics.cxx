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

void Diagnostics::decode()
{
  DecoderBase::run();
}

void Diagnostics::headerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* /*crateOrbit*/)
{
  // mDRMCounter[crateHeader->drmID].Count(counters::kDRM_HAS_DATA);
  mDRMCounter[crateHeader->drmID].Count(0);
  for (Int_t i = 1; i < 11; i++) {
    if (crateHeader->slotEnableMask & 1 << i) { // Magari includere l'LTM come i==0
      // mTRMCounter[crateHeader->drmID][i - 1].Count(counters::kTRM_HAS_DATA);
      mTRMCounter[crateHeader->drmID][i - 1].Count(0);
    }
  }
}

void Diagnostics::trailerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* /*crateOrbit*/,
                                 const CrateTrailer_t* crateTrailer, const Diagnostic_t* diagnostics)
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
