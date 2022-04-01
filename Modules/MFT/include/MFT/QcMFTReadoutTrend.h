// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   QcMFTReadoutTrend.h
/// \author Tomas Herman
/// \author Guillermo Contreras
/// \author Katarina Krizkova Gajdosova
/// \author Diana Maria Krupova
///
#ifndef MFT_READOUT_TREND_H
#define MFT_READOUT_TREND_H

#include "QualityControl/Reductor.h"

namespace o2::quality_control_modules::mft
{

/// \brief A Reductor which obtains the  bin content of a TH1.
/// A Reductor which obtains the bin content of a  TH1.

class QcMFTReadoutTrend : public quality_control::postprocessing::Reductor
{
 public:
  QcMFTReadoutTrend() = default;
  ~QcMFTReadoutTrend() = default;

  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  void update(TObject* obj) override;

 private:
  struct {
    float binContent[936];
    float binContentOverflow;
    Double_t mean;
    Double_t stddev;
    Double_t entries;
  } mStats;
};

} // namespace o2::quality_control_modules::mft

#endif // QC_MFT_READOUT_TREND_H
