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
/// \file   VDriftCalibReductor.cxx
/// \author Marcel Lesch
///

#include "TPC/VDriftCalibReductor.h"

#include <DataFormatsTPC/VDriftCorrFact.h>

namespace o2::quality_control_modules::tpc
{

void* VDriftCalibReductor::getBranchAddress()
{
  return &mStats;
}

const char* VDriftCalibReductor::getBranchLeafList()
{
  return "vdrift/F:vdrifterror";
}

bool VDriftCalibReductor::update(ConditionRetriever& retriever)
{
  if (auto vdriftCalib = retriever.retrieve<o2::tpc::VDriftCorrFact>()) {
    mStats.vdrift = vdriftCalib->getVDrift();
    mStats.vdrifterror = vdriftCalib->getVDriftError();
    return true;
  }
  return false;
}

} // namespace o2::quality_control_modules::tpc