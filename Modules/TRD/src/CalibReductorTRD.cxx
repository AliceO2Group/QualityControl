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
/// \author
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
  return "vdriftmean/F:vdrifterr/F";
}

bool CalibReductorTRD::update(ConditionRetriever& retriever)
{
  if (auto retvdrift = retriever.retrieve<o2::trd::CalVdriftExB>()) {
    double sum = 0;
    double sumSq = 0;
    for (int i = 0; i < o2::trd::constants::MAXCHAMBER; i++) {
      double vdrift = retvdrift->getVdrift(i);
      sum += vdrift;
      sumSq += vdrift * vdrift;
    }
    mStats.vdriftmean = sum / o2::trd::constants::MAXCHAMBER;
    mStats.vdrifterr = std::sqrt(sumSq / o2::trd::constants::MAXCHAMBER - mStats.vdriftmean * mStats.vdriftmean);
    return true;
  }
  return false;
}

} // namespace o2::quality_control_modules::trd
