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
/// \file   RawQcTask.cxx
/// \author Bogdan Vulpescu / Xavier Lopez
///

#include <TCanvas.h>
#include <TH1.h>

#include "QualityControl/QcInfoLogger.h"
#include "MID/RawQcTask.h"

#include "Framework/DataRefUtils.h"
#include "DataFormatsMID/ColumnData.h"

namespace o2::quality_control_modules::mid
{

RawQcTask::~RawQcTask()
{
  if (mDetElemID) {
    delete mDetElemID;
  }
}

void RawQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info) << "initialize RawQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  mDetElemID = new TH1F("mDetElemID", "Id of detector element", 81, -0.5, 80.5);
  getObjectsManager()->startPublishing(mDetElemID);
}

void RawQcTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "startOfActivity" << ENDM;
  mDetElemID->Reset();
}

void RawQcTask::startOfCycle()
{
  ILOG(Info) << "startOfCycle" << ENDM;
}

void RawQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  auto msg = ctx.inputs().get("digits");
  gsl::span<const o2::mid::ColumnData> patterns = o2::framework::DataRefUtils::as<const o2::mid::ColumnData>(msg);

  // Loop on stripPatterns
  for (auto& col : patterns) {
    int deIndex = col.deId;
    mDetElemID->Fill(deIndex);
  }
}

void RawQcTask::endOfCycle()
{
  ILOG(Info) << "endOfCycle" << ENDM;
}

void RawQcTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "endOfActivity" << ENDM;
}

void RawQcTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info) << "Resetting the histogram" << ENDM;
  mDetElemID->Reset();
}

} // namespace o2::quality_control_modules::mid
