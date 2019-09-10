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
/// \author Piotr Konopka
///

#include "Skeleton/SkeletonPostProcessing.h"

using namespace o2::quality_control::postprocessing;

namespace o2::quality_control_modules::skeleton
{

SkeletonPostProcessing::~SkeletonPostProcessing()
{

}

void SkeletonPostProcessing::initialize(Trigger)
{

}

void SkeletonPostProcessing::postProcess(Trigger)
{

}

void SkeletonPostProcessing::finalize(Trigger)
{

}

void SkeletonPostProcessing::store()
{

}

void SkeletonPostProcessing::reset()
{

}
} // namespace o2::quality_control_modules::skeleton
