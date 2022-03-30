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
/// \file   QcMFTReadoutTrend.cxx
/// \author Tomas Herman
/// \author Guillermo Contreras
/// \author Katarina Krizkova Gajdosova
/// \author Diana Maria Krupova
///

#include <TH1.h>
#include "MFT/QcMFTReadoutTrend.h"

namespace o2::quality_control_modules::mft
{

void* QcMFTReadoutTrend::getBranchAddress()
{
  return &mStats;
}

const char* QcMFTReadoutTrend::getBranchLeafList()
{
  return "binContent[936]:binContentOverflow:mean/D:stddev:entries";
}

void QcMFTReadoutTrend::update(TObject* obj)
{
  auto histo = dynamic_cast<TH1*>(obj);
  if (histo) {
    for (int i = 0; i < 936; i++) {
      mStats.binContent[i] = histo->GetBinContent(i + 1);
    }
    mStats.binContentOverflow = histo->GetBinContent(937);
    mStats.entries = histo->GetEntries();
    mStats.stddev = histo->GetStdDev();
    mStats.mean = histo->GetMean();
  }
}

} // namespace o2::quality_control_modules::mft
