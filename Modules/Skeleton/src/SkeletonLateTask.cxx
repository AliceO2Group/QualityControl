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
/// \file   SkeletonLateTask.cxx
/// \author My Name
///

#include <TCanvas.h>
#include <TGraph.h>
#include <TH1.h>

#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/QCInputs.h"
#include "QualityControl/QCInputsAdapters.h"
#include "Skeleton/SkeletonLateTask.h"

#include <boost/property_tree/ptree.hpp>
#include <Framework/InputRecordWalker.h>
#include <Framework/DataRefUtils.h>

namespace o2::quality_control_modules::skeleton
{

SkeletonLateTask::~SkeletonLateTask()
{
}

void SkeletonLateTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.

  // This is how logs are created. QcInfoLogger is used. In production, FairMQ logs will go to InfoLogger as well.
  ILOG(Debug, Devel) << "initialize SkeletonLateTask" << ENDM;
  ILOG(Debug, Support) << "A debug targeted for support" << ENDM;
  ILOG(Info, Ops) << "An Info log targeted for operators" << ENDM;

  // This creates and registers a histogram for publication at the end of each cycle, until the end of the task lifetime
  mGraph = std::make_unique<TGraph>();
  mGraph->SetName("graph_example");
  mGraph->SetTitle("graph_example");
  getObjectsManager()->startPublishing(mGraph.get(), PublicationPolicy::Forever);

  auto plots = mCustomParameters.getOptionalPtree("plots");
  if (plots.has_value()) {
    ILOG(Info, Support) << "nested param: " <<  plots.value().get<std::string>("nested") << ENDM;
  }
}

void SkeletonLateTask::startOfActivity(const Activity& activity)
{
  // THIS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;

  // mGraph->Clear();
}

void SkeletonLateTask::process(const quality_control::core::QCInputs& data)
{
  // THIS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.

  if (auto histoOpt = getMonitorObject<TH1>(data, "example")) {
    const TH1& histo = histoOpt.value();

    ILOG(Info, Ops) << "Histogram " << histo.GetName() << " has " << histo.GetEntries() << " entries" << ENDM;
    mGraph->AddPoint(histo.GetEntries(), histo.GetMean());
  }

  if (auto qoOpt = getQualityObject(data, "QcCheck")) {
    ILOG(Info, Ops) << "Got QcCheck result: " << qoOpt.value().get().getQuality() << ENDM;
  }
}

void SkeletonLateTask::endOfActivity(const Activity& /*activity*/)
{
  // THIS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void SkeletonLateTask::reset()
{
  // THIS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.

  // Clean all the monitor objects here.
  ILOG(Debug, Devel) << "Resetting the plots" << ENDM;
  if (mGraph) {
    mGraph->Clear();
  }
}

} // namespace o2::quality_control_modules::skeleton
