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
/// \file   GPUErrorQA.cxx
/// \author Anton Riede, anton.riedel@cern.ch
///

// root includes
#include <TH1.h>

// O2 includes
#include "Framework/ProcessingContext.h"
#include <Framework/InputRecord.h>

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TPC/GPUErrorQA.h"

namespace o2::quality_control_modules::tpc
{

void GPUErrorQA::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize TPC GPUErrorQA QC task" << ENDM;
  mGPUErrorQA.initializeHistograms();
}

void GPUErrorQA::startOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Support) << "startOfActivity" << ENDM;
  mGPUErrorQA.resetHistograms();
}

void GPUErrorQA::startOfCycle()
{
  ILOG(Debug, Support) << "startOfCycle" << ENDM;
}

void GPUErrorQA::monitorData(o2::framework::ProcessingContext& ctx)
{
  ILOG(Debug, Devel) << "monitorData" << ENDM;
  auto errors = ctx.inputs().get<std::vector<std::array<uint32_t, 4>>>("error-qa");
  mGPUErrorQA.processErrors(errors);
}

void GPUErrorQA::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void GPUErrorQA::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void GPUErrorQA::reset()
{
  // clean all the monitor objects here
  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  mGPUErrorQA.resetHistograms();
}

} // namespace o2::quality_control_modules::tpc
