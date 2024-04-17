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
/// \file   CalibReductorTRD.cxx
/// \author Salman Malik
///

#include "TRD/CalibReductorTRD.h"

#include <DataFormatsTRD/CalVdriftExB.h>

namespace o2::quality_control_modules::trd
{

void* CalibReductorTRD::getBranchAddress()
{
  return &mStats;
}

const char* CalibReductorTRD::getBranchLeafList()
{
  return Form("vdrift[%i]:vdriftmean/F:vdrifterr:exbmean/F:exberr", o2::trd::constants::MAXCHAMBER);
}

bool CalibReductorTRD::update(ConditionRetriever& retriever)
{
  if (auto retvdrift = retriever.retrieve<o2::trd::CalVdriftExB>()) {
    double sumVdrift = 0, sumSqVdrift = 0;
    double sumExb = 0, sumSqExb = 0;

    for (int i = 0; i < o2::trd::constants::MAXCHAMBER; i++) {
      mStats.vdrift[i] = retvdrift->getVdrift(i);
      sumVdrift += retvdrift->getVdrift(i);
      sumSqVdrift += retvdrift->getVdrift(i) * retvdrift->getVdrift(i);
      sumExb += retvdrift->getExB(i);
      sumSqExb += retvdrift->getExB(i) * retvdrift->getExB(i);
    }

    mStats.vdriftmean = sumVdrift / o2::trd::constants::MAXCHAMBER;
    mStats.vdrifterr = std::sqrt(sumSqVdrift / o2::trd::constants::MAXCHAMBER - mStats.vdriftmean * mStats.vdriftmean);

    mStats.exbmean = sumExb / o2::trd::constants::MAXCHAMBER;
    mStats.exberr = std::sqrt(sumSqExb / o2::trd::constants::MAXCHAMBER - mStats.exbmean * mStats.exbmean);

    return true;
  }
  return false;
}

} // namespace o2::quality_control_modules::trd
