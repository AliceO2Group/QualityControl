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
/// \file   CalibrationTask.h
/// \author Milosz Filus

#ifndef QUALITYCONTROL_FT0_CALIBRATIONTASK_H
#define QUALITYCONTROL_FT0_CALIBRATIONTASK_H

#include "QualityControl/TaskInterface.h"
#include <memory>
#include "TH1.h"
#include "TH2.h"
#include "TTree.h"
#include "TFile.h"
#include "TGraph.h"
#include "TMultiGraph.h"
#include "Rtypes.h"
#include "FT0Calibration/FT0ChannelTimeCalibrationObject.h"

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::ft0
{

class CalibrationTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  CalibrationTask() = default;
  /// Destructor
  ~CalibrationTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  static constexpr int CHANNEL_TIME_HISTOGRAM_RANGE = 200;
  static constexpr const char* CCDB_PARAM_KEY = "CCDBUrl";

  // Object which will be published
  std::unique_ptr<TH1F> mNotCalibratedChannelTimeHistogram;
  std::unique_ptr<TH1F> mCalibratedChannelTimeHistogram;
  std::unique_ptr<TGraph> mChannelTimeCalibrationObjectGraph;
  std::unique_ptr<TH2F> mCalibratedTimePerChannelHistogram;
  std::unique_ptr<TH2F> mNotCalibratedTimePerChannelHistogram;
  o2::ft0::FT0ChannelTimeCalibrationObject* mCurrentChannelTimeCalibrationObject;
};

} // namespace o2::quality_control_modules::ft0

#endif //QUALITYCONTROL_FT0_CALIBRATIONTASK_H
