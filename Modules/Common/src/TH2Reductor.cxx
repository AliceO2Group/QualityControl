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
/// \file   TH2Reductor.cxx
/// \author Piotr Konopka
///

#include <TH2.h>
#include "Common/TH2Reductor.h"

namespace o2::quality_control_modules::common
{

void* TH2Reductor::getBranchAddress()
{
  return &mStats;
}

const char* TH2Reductor::getBranchLeafList()
{
  return "sumw/D:sumw2:sumwx:sumwx2:sumwy:sumwy2:sumwxy:entries";
}

void TH2Reductor::update(TObject* obj)
{
  auto histo = dynamic_cast<TH2*>(obj);
  if (histo) {
    histo->GetStats(mStats.sums.array);
    mStats.entries = histo->GetEntries();
  }
}

} // namespace o2::quality_control_modules::common
