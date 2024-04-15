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
/// \file   PostProcessingInterface.cxx
/// \author My Name
///

#include "Skeleton/SkeletonPostProcessing.h"
#include "QualityControl/QcInfoLogger.h"

#include <TH1F.h>

using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control::core;

namespace o2::quality_control_modules::skeleton
{

SkeletonPostProcessing::~SkeletonPostProcessing()
{
}

void SkeletonPostProcessing::configure(const boost::property_tree::ptree& config)
{
  // A histogram created here can be reused throughout the task lifetime
  mHistogramA = std::make_unique<TH1F>("exampleA", "exampleA", 20, 0, 30000);
  // We request that the histogram A is published until the end of the task lifetime
  getObjectsManager()->startPublishing(mHistogramA.get(), PublicationPolicy::Forever);
}

void SkeletonPostProcessing::initialize(Trigger, framework::ServiceRegistryRef)
{
  // Resetting the histogram A, so it is empty at each new START
  mHistogramA->Reset();

  // We want the histogram B to be recreated at each START, so first we delete the existing one from the possible
  // previous run, then create the new one. ROOT may crash if there are two objects with the same name at the same time.
  mHistogramB.reset();
  mHistogramB = std::make_unique<TH1F>("exampleB", "exampleB", 20, 0, 30000);
  // We request that the histogram A is published after each update(), up to and including finalize()
  // The framework will stop publishing it after finalize()
  getObjectsManager()->startPublishing(mHistogramB.get(), PublicationPolicy::ThroughStop);
}

void SkeletonPostProcessing::update(Trigger t, framework::ServiceRegistryRef)
{
  ILOG(Info, Support) << "Trigger type is: " << t.triggerType << ", the timestamp is " << t.timestamp << ENDM;

  // Histogram C is recreated at each update and is expected to be published just one. We delete the previous
  // histogram and create the new one, then we start publishing it.
  mHistogramC.reset();
  mHistogramC = std::make_unique<TH1F>("exampleC", "exampleC", 20, 0, 30000);
  getObjectsManager()->startPublishing(mHistogramC.get(), PublicationPolicy::Once);

  // We fill the histograms
  mHistogramA->Fill(t.timestamp % 30000);
  mHistogramB->Fill(t.timestamp % 30000);
  mHistogramC->Fill(t.timestamp % 30000);
}

void SkeletonPostProcessing::finalize(Trigger, framework::ServiceRegistryRef)
{
}

} // namespace o2::quality_control_modules::skeleton
