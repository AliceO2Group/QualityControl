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

#ifndef QC_MODULE_FV0_BASICPPTASK_H
#define QC_MODULE_FV0_BASICPPTASK_H

#include "QualityControl/PostProcessingInterface.h"
#include "QualityControl/DatabaseInterface.h"
#include "FV0Base/Constants.h"
#include <TGraph.h>
#include <TCanvas.h>

class TH1F;
class TGraph;
class TCanvas;
class TLegend;
class TProfile;

namespace o2::quality_control_modules::fv0
{

/// \brief Basic Postprocessing Task for FV0, computes among others the trigger rates
/// \author Sebastian Bysiak sbysiak@cern.ch
class BasicPPTask final : public quality_control::postprocessing::PostProcessingInterface
{
 public:
  BasicPPTask() = default;
  ~BasicPPTask() override;
  void configure(std::string, const boost::property_tree::ptree&) override;
  void initialize(quality_control::postprocessing::Trigger, framework::ServiceRegistry&) override;
  void update(quality_control::postprocessing::Trigger, framework::ServiceRegistry&) override;
  void finalize(quality_control::postprocessing::Trigger, framework::ServiceRegistry&) override;

  constexpr static std::size_t sNCHANNELS_PM = 60; //48(for PM) + 12(just in case for possible PM-LCS)

 private:
  std::string mCycleDurationMoName;
  int mNumOrbitsInTF;

  o2::quality_control::repository::DatabaseInterface* mDatabase = nullptr;
  std::unique_ptr<TGraph> mRateMinBias;
  std::unique_ptr<TGraph> mRateOuterRing;
  std::unique_ptr<TGraph> mRateNChannels;
  std::unique_ptr<TGraph> mRateCharge;
  std::unique_ptr<TGraph> mRateInnerRing;
  std::unique_ptr<TCanvas> mRatesCanv;
  TProfile* mAmpl = nullptr;
  TProfile* mTime = nullptr;
};

} // namespace o2::quality_control_modules::fv0

#endif //QC_MODULE_FV0_BASICPPTASK_H
