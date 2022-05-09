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
/// \file   TracksQcTask.h
/// \author Valerie Ramillien
///

#ifndef QC_MODULE_MID_MIDTRACKSQCTASK_H
#define QC_MODULE_MID_MIDTRACKSQCTASK_H

#include "QualityControl/TaskInterface.h"

class TH1F;
class TH2F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::mid
{

class TracksQcTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  TracksQcTask() = default;
  /// Destructor
  ~TracksQcTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  std::shared_ptr<TH1F> mHistogram{ nullptr };

  std::shared_ptr<TH1F> mMultTracks{ nullptr };
  std::shared_ptr<TH2F> mTrackMapXY{ nullptr };
  std::shared_ptr<TH1F> mTrackDevX{ nullptr };
  std::shared_ptr<TH1F> mTrackDevY{ nullptr };
  std::shared_ptr<TH2F> mTrackDevXY{ nullptr };

  std::shared_ptr<TH1F> mTrackBCCounts{ nullptr };
  ///////////////////////////
  int nROF = 0;
};

} // namespace o2::quality_control_modules::mid

#endif // QC_MODULE_MID_MIDTRACKSQCTASK_H
