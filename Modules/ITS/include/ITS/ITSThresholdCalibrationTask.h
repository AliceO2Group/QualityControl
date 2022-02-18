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
/// \file   ITSThreshodCalibrationTask.h
/// \author Artem Isakov
///

#ifndef QC_MODULE_ITS_ITSTHRESHOLDCALIBRATIONTASK_H
#define QC_MODULE_ITS_ITSTHRESHOLDCALIBRATIONTASK_H

#include "QualityControl/TaskInterface.h"
#include <TH1D.h>
#include <TH2D.h>
#include <ITSBase/GeometryTGeo.h>
#include <TTree.h>

class TH1D;
class TH2D;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::its
{

class ITSThresholdCalibrationTask : public TaskInterface
{

 public:
  ITSThresholdCalibrationTask();
  ~ITSThresholdCalibrationTask() override;

  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  void publishHistos();
  void formatAxes(TH1* h, const char* xTitle, const char* yTitle, float xOffset = 1., float yOffset = 1.);
  void addObject(TObject* aObject);
  void createAllHistos();

  static constexpr int NLayer = 7;
  static constexpr int NLayerIB = 3;


  std::string mRunNumber;
  std::string mRunNumberPath;

};
} // namespace o2::quality_control_modules::its

#endif // QC_MODULE_ITS_ITSTHRESHOLDCALIBRATIONTASK_H
