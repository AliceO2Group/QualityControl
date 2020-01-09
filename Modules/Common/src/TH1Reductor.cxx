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
/// \file   TH1Reductor.cxx
/// \author Piotr Konopka
///

#include <TH1.h>
#include "Common/TH1Reductor.h"

namespace o2::quality_control_modules::common
{

void* TH1Reductor::getBranchAddress()
{
  return &mStats;
}

const char* TH1Reductor::getBranchLeafList()
{
  return "mean/D:stddev:entries";
}

void TH1Reductor::update(TObject* obj)
{
  // todo: use GetStats() instead?
  auto histo = dynamic_cast<TH1*>(obj);
  if (histo) {
    mStats.entries = histo->GetEntries();
    mStats.stddev = histo->GetStdDev();
    mStats.mean = histo->GetMean();
  }
}

} // namespace o2::quality_control_modules::common
