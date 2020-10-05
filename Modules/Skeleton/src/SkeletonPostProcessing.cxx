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

void SkeletonPostProcessing::initialize(Trigger, framework::ServiceRegistry&)
{
  mHistogram = new TH1F("example", "example", 20, 0, 30000);
  getObjectsManager()->startPublishing(mHistogram);
}

void SkeletonPostProcessing::update(Trigger t, framework::ServiceRegistry&)
{
  ILOG(Info, Support) << "Trigger type is: " << t.triggerType << ", the timestamp is " << t.timestamp << ENDM;
  mHistogram->Fill(t.timestamp % 30000);
}

void SkeletonPostProcessing::finalize(Trigger, framework::ServiceRegistry&)
{
  // Only if you don't want it to be published after finalisation.
  getObjectsManager()->stopPublishing(mHistogram);
}

} // namespace o2::quality_control_modules::skeleton
