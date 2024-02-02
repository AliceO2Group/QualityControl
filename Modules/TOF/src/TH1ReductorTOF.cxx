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
/// \file   TH1ReductorTOFTOF.cxx
/// \author Francesca Ercolessi
///

#include <TH1.h>
#include "TOF/TH1ReductorTOF.h"

namespace o2::quality_control_modules::tof
{

void* TH1ReductorTOF::getBranchAddress()
{
  return &mStats;
}

const char* TH1ReductorTOF::getBranchLeafList()
{
  return "mean/D:stddev:entries:maxpeak";
}

void TH1ReductorTOF::update(TObject* obj)
{
  // todo: use GetStats() instead?
  auto histo = dynamic_cast<TH1*>(obj);
  if (histo) {
    mStats.entries = histo->GetEntries();
    mStats.stddev = histo->GetStdDev();
    mStats.mean = histo->GetMean();
    mStats.maxpeak = histo->GetBinCenter(histo->GetMaximumBin());
  }
}

} // namespace o2::quality_control_modules::tof
