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
/// \file    SkeletonPostProcessing.cxx
/// \author  Piotr Konopka
///

#ifndef QUALITYCONTROL_SKELETONPOSTPROCESSING_H
#define QUALITYCONTROL_SKELETONPOSTPROCESSING_H

#include "QualityControl/PostProcessingInterface.h"

class TH1F;

namespace o2::quality_control_modules::skeleton
{

/// \brief Example Quality Control Postprocessing Task
/// \author Piotr Konopka
class SkeletonPostProcessing final : public quality_control::postprocessing::PostProcessingInterface
{
  public:
  /// \brief Constructor
  SkeletonPostProcessing() = default;
  /// Destructor
  ~SkeletonPostProcessing() override;

  // user gets to know what triggered the init
  void initialize(quality_control::postprocessing::Trigger) override;
  // user gets to know what triggered the processing
  void postProcess(quality_control::postprocessing::Trigger) override;
  // user gets to know what triggered the end
  void finalize(quality_control::postprocessing::Trigger) override;
  // store your stuff
  void store() override;
  // reset your stuff. maybe a trigger needed?
  void reset() override;

  private:
  TH1F* mHistogram = nullptr;
};

} // namespace o2::quality_control_modules::skeleton

#endif //QUALITYCONTROL_SKELETONPOSTPROCESSING_H
