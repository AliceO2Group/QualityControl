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
/// \file   TOFDecoderCompressed.h
/// \author Nicolo' Jacazio
///

#ifndef QC_MODULE_TOF_TOFDECODERCOMPRESSED_H
#define QC_MODULE_TOF_TOFDECODERCOMPRESSED_H

#include "TH1.h"

// O2 includes
#include "TOFReconstruction/DecoderBase.h"
#include "DataFormatsTOF/CompressedDataFormat.h"
using namespace o2::tof::compressed;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::tof
{

/// \brief TOF Quality Control class for Decoding Compressed data for TOF Compressed data QC Task
/// \author Nicolo' Jacazio
class TOFDecoderCompressed /*final*/
  : public DecoderBase
// todo add back the "final" when doxygen is fixed
{
 public:
  /// \brief Constructor
  TOFDecoderCompressed() = default;
  /// Destructor
  ~TOFDecoderCompressed() = default;

  /// Function to run decoding
  void decode();

  /// Histograms to fill
  std::map<std::string, TH1*> mHistos;

  Int_t rdhread = 0; /// Number of times a RDH is read

 private:
  /** decoding handlers **/
  void rdhHandler(const o2::header::RAWDataHeader* rdh) override;
  void headerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* crateOrbit) override;
  void frameHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* crateOrbit,
                    const FrameHeader_t* frameHeader, const PackedHit_t* packedHits) override;
  void trailerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* crateOrbit,
                      const CrateTrailer_t* crateTrailer, const Diagnostic_t* diagnostics) override;
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_TOFDECODERCOMPRESSED_H
