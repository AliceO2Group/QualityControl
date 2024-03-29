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

namespace o2::quality_control_modules::skeleton
{

SkeletonPostProcessing::~SkeletonPostProcessing()
{
  delete mHistogram;
}

void SkeletonPostProcessing::initialize(Trigger, framework::ServiceRegistryRef)
{
  mHistogram = new TH1F("example", "example", 20, 0, 30000);
  getObjectsManager()->startPublishing(mHistogram);
}

void SkeletonPostProcessing::update(Trigger t, framework::ServiceRegistryRef)
{
  ILOG(Info, Support) << "Trigger type is: " << t.triggerType << ", the timestamp is " << t.timestamp << ENDM;
  mHistogram->Fill(t.timestamp % 30000);
}

void SkeletonPostProcessing::finalize(Trigger, framework::ServiceRegistryRef)
{
  // Only if you don't want it to be published after finalisation.
  getObjectsManager()->stopPublishing(mHistogram);
}

} // namespace o2::quality_control_modules::skeleton
