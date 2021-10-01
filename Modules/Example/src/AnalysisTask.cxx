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
/// \file   AnalysisTask.cxx
/// \author Piotr Konopka
///

#include <TCanvas.h>
#include <TH1.h>

#include "QualityControl/QcInfoLogger.h"
#include "Example/AnalysisTask.h"
#include <Framework/TableConsumer.h>
#include <Framework/InputRecord.h>
#include <arrow/table.h>

namespace o2::quality_control_modules::example
{

AnalysisTask::~AnalysisTask()
{
  delete mHistogram;
}

void AnalysisTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize AnalysisTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  mHistogram = new TH1F("example", "example", 20, 0, 30000);
  getObjectsManager()->startPublishing(mHistogram);
}

void AnalysisTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity" << activity.mId << ENDM;
  mHistogram->Reset();
}

void AnalysisTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void AnalysisTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  ILOG(Info, Devel) << "Monitor data" << ENDM;
  auto s = ctx.inputs().get<framework::TableConsumer>("aod-data");

  auto table = s->asArrowTable();
  if (table->num_rows() == 0) {
    ILOG(Error, Support) << "The arrow table is empty" << table->num_rows() << ENDM;
    return;
  }
  ILOG(Info, Devel) << "The arrow table has " << table->num_rows() << " rows" << ENDM;
  mHistogram->Fill(table->num_rows());

  if (table->num_columns() == 0) {
    ILOG(Error, Support) << "No columns in the arrow table" << table->num_columns() << ENDM;
    return;
  }
  ILOG(Info) << "The arrow table has " << table->num_columns() << " columns" << ENDM;
  mHistogram->Fill(table->num_columns());

  // Here you can perform analysis of the columnar data.
  // Please refer to the documentation of DPL Analysis, Apache Arrow
  // and RDataFrame's support of Apache Arrow.
}

void AnalysisTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void AnalysisTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void AnalysisTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  mHistogram->Reset();
}

} // namespace o2::quality_control_modules::example
