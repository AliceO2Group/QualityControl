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
/// \file   TOFDecoderCompressed.cxx
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
#include "TOF/TOFDecoderCompressed.h"

#define VERBOSEDECODERCOMPRESSED //Flag for a verbose Decoder

namespace o2::quality_control_modules::tof
{

void TOFDecoderCompressed::decode()
{
  DecoderBase::run();
}

void TOFDecoderCompressed::headerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* crateOrbit)
{
  for (int ibit = 0; ibit < 11; ++ibit)
    if (crateHeader->slotEnableMask & (1 << ibit))
      mHistos["hSlotEnableMask"]->Fill(crateHeader->drmID, ibit + 2);
}

void TOFDecoderCompressed::frameHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* crateOrbit,
                                        const FrameHeader_t* frameHeader, const PackedHit_t* packedHits)
{
  mHistos["hHits"]->Fill(frameHeader->numberOfHits);
  for (int i = 0; i < frameHeader->numberOfHits; ++i) {
    auto packedHit = packedHits + i;
    auto indexE = packedHit->channel +
                  8 * packedHit->tdcID +
                  120 * packedHit->chain +
                  240 * (frameHeader->trmID - 3) +
                  2400 * crateHeader->drmID;
    int time = packedHit->time;
    int timebc = time % 1024;
    time += (frameHeader->frameID << 13);

    mHistos["hIndexE"]->Fill(indexE);
    mHistos["hTime"]->Fill(time);
    mHistos["hTimeBC"]->Fill(timebc);
    mHistos["hTOT"]->Fill(packedHit->tot);
  }
}

void TOFDecoderCompressed::trailerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* crateOrbit,
                                          const CrateTrailer_t* crateTrailer, const Diagnostic_t* diagnostics)
{
  for (int i = 0; i < crateTrailer->numberOfDiagnostics; ++i) {
    auto diagnostic = diagnostics + i;
    mHistos["hDiagnostic"]->Fill(crateHeader->drmID, diagnostic->slotID);
  }
}

void TOFDecoderCompressed::rdhHandler(const o2::header::RAWDataHeader* rdh)
{
#ifdef VERBOSEDECODERCOMPRESSED
  LOG(INFO) << "Reading RDH #" << rdhread++ / 2;
  o2::raw::HBFUtils::printRDH(*rdh);
  if (rdh && 0) {
    LOG(INFO) << "Processing RDH";
  }
#endif
}

} // namespace o2::quality_control_modules::tof
