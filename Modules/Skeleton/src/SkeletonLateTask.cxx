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
#include <TH2I.h>

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

  // This creates and registers a graph for publication, we will track "example" histogram mean here
  mMeanTrend = std::make_unique<TGraph>();
  mMeanTrend->SetName("mean_trend");
  mMeanTrend->SetTitle("mean_trend");
  mMeanTrend->GetXaxis()->SetTimeDisplay(kTRUE);
  mMeanTrend->SetMarkerStyle(kStar); // star markers
  mMeanTrend->SetLineStyle(kSolid);  // solid line
  getObjectsManager()->startPublishing(mMeanTrend.get(), PublicationPolicy::Forever);

  // This creates and registers a 2D histogram for publication, we will fill it with means of "example" and "example2" histograms
  mCorrelation = std::make_unique<TH2I>("correlation", "correlation", 20, 0, 10500, 20, 0, 255);
  getObjectsManager()->startPublishing(mCorrelation.get(), PublicationPolicy::Forever);

  // this demonstrates how to get a property tree of custom parameters
  auto plots = mCustomParameters.getOptionalPtree("plots");
  if (plots.has_value()) {
    ILOG(Info, Support) << "nested param: " << plots.value().get<std::string>("nested") << ENDM;
  }
}

void SkeletonLateTask::startOfActivity(const Activity& activity)
{
  // THIS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;

  // remove all existing data in plots to have them clean in a start->stop->start sequence
  reset();
}

void SkeletonLateTask::process(const quality_control::core::QCInputs& data)
{
  // THIS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.

  // this is how a MonitorObject can be obtained, incl. the wrapper itself
  if (auto moOpt = getMonitorObject(data, "example")) {
    const MonitorObject& mo = moOpt.value();
    auto validityEndSeconds = mo.getValidity().getMax() / 1000;

    auto histo = dynamic_cast<TH1*>(mo.getObject());
    if (histo) {
      mMeanTrend->AddPoint(validityEndSeconds, histo->GetMean());
      ILOG(Debug, Devel) << "New point in graph" << ENDM;
    }
  }

  // this is how objects can be retrieved without a MonitorObject wrapper
  auto example1Opt = getMonitorObject<TH1>(data, "example");
  auto example2Opt = getMonitorObject<TH1>(data, "example2");
  if (example1Opt.has_value() && example2Opt.has_value()) {
    const TH1& example1 = example1Opt.value();
    const TH1& example2 = example2Opt.value();

    mCorrelation->Fill(example1.GetMean(), example2.GetMean());
    ILOG(Debug, Devel) << "New entry in correlation" << ENDM;
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
  if (mMeanTrend) {
    mMeanTrend->Set(0);
  }
  if (mCorrelation) {
    mCorrelation->Reset();
  }
}

} // namespace o2::quality_control_modules::skeleton
