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
/// \file   TOFCounter.cxx
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
#include "TOF/TOFCounter.h"

namespace o2::quality_control_modules::tof
{

void TOFCounter::decode()
{
  DecoderBase::run();
}

void TOFCounter::trailerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* /*crateOrbit*/,
                                const CrateTrailer_t* crateTrailer, const Diagnostic_t* diagnostics)
{
  LOG(INFO) << "DECODING!!!" << ENDM;

  for (int i = 0; i < crateTrailer->numberOfDiagnostics; ++i) {
    auto diagnostic = diagnostics + i;
    // mHistos.at("hDiagnostic")->Fill(crateHeader->drmID, diagnostic->slotID);
    // mDiagnostic.Count(crateHeader->drmID, diagnostic->slotID, kCrateCounter_Error);
    mRDHCounter[crateHeader->drmID].Count(counters::kRDHCounter_Data);
  }
}

} // namespace o2::quality_control_modules::tof
