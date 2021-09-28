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
/// \file   BasicPPTask.h
/// \author Sebastian Bysiak sbysiak@cern.ch
///

#ifndef QC_MODULE_FT0_BASICPPTASK_H
#define QC_MODULE_FT0_BASICPPTASK_H

#include "QualityControl/PostProcessingInterface.h"
#include "QualityControl/DatabaseInterface.h"

class TH1F;
class TGraph;
class TCanvas;
class TLegend;

namespace o2::quality_control_modules::ft0
{

/// \brief Basic Postprocessing Task for FT0, computes among others the trigger rates
/// \author Sebastian Bysiak sbysiak@cern.ch
class BasicPPTask final : public quality_control::postprocessing::PostProcessingInterface
{
 public:
  BasicPPTask() = default;
  ~BasicPPTask() override;
  void initialize(quality_control::postprocessing::Trigger, framework::ServiceRegistry&) override;
  void update(quality_control::postprocessing::Trigger, framework::ServiceRegistry&) override;
  void finalize(quality_control::postprocessing::Trigger, framework::ServiceRegistry&) override;

 private:
  o2::quality_control::repository::DatabaseInterface* mDatabase = nullptr;
  TGraph* mRateOrA = nullptr;
  TGraph* mRateOrC = nullptr;
  TGraph* mRateVertex = nullptr;
  TGraph* mRateCentral = nullptr;
  TGraph* mRateSemiCentral = nullptr;
  TCanvas* mRatesCanv = nullptr;
};

} // namespace o2::quality_control_modules::ft0

#endif //QC_MODULE_FT0_BASICPPTASK_H
