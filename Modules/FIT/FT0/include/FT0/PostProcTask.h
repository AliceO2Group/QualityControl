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
/// \file   PostProcTask.h
/// \author Sebastian Bysiak sbysiak@cern.ch
///

#ifndef QC_MODULE_FT0_POSTPROCTASK_H
#define QC_MODULE_FT0_POSTPROCTASK_H

#include "QualityControl/PostProcessingInterface.h"
#include "QualityControl/DatabaseInterface.h"
#include "FITCommon/PostProcHelper.h"
#include "FITCommon/DetectorFIT.h"

#include "CCDB/CcdbApi.h"
#include "CommonConstants/LHCConstants.h"
#include "FT0Base/Constants.h"
#include "DataFormatsFT0/ChannelData.h"
#include "DataFormatsFT0/Digit.h"

#include <TH2.h>
#include <TCanvas.h>
#include <TGraph.h>

class TH1F;
class TCanvas;
class TLegend;
class TProfile;

namespace o2::quality_control_modules::ft0
{

/// \brief Basic Postprocessing Task for FT0, computes among others the trigger rates
/// \author Sebastian Bysiak sbysiak@cern.ch
class PostProcTask final : public quality_control::postprocessing::PostProcessingInterface
{
 public:
  PostProcTask() = default;
  ~PostProcTask() override;
  void configure(const boost::property_tree::ptree&) override;
  void initialize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;
  void update(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;
  void finalize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;

  constexpr static std::size_t sBCperOrbit = o2::constants::lhc::LHCMaxBunches;
  constexpr static std::size_t sNCHANNELS_PM = o2::ft0::Constants::sNCHANNELS_PM;
  using Detector_t = o2::quality_control_modules::fit::detectorFIT::DetectorFT0;

 private:
  o2::quality_control_modules::fit::PostProcHelper mPostProcHelper;
  bool mIsFirstIter{ true };
  typename Detector_t::TrgMap_t mMapPMbits = Detector_t::sMapPMbits;
  typename Detector_t::TrgMap_t mMapTechTrgBits = Detector_t::sMapTechTrgBits;
  typename Detector_t::TrgMap_t mMapTrgBits = Detector_t::sMapTrgBits;
  // MOs
  std::unique_ptr<TH2F> mHistChDataNOTbits;
  std::unique_ptr<TH1F> mHistTriggers;
  std::unique_ptr<TH1F> mHistTriggerRates;
  std::unique_ptr<TH1F> mHistTimeInWindow;
  std::unique_ptr<TH1F> mHistCFDEff;
  std::unique_ptr<TH1F> mHistChannelID_outOfBC;
  std::unique_ptr<TH1F> mHistTrg_outOfBC;
  std::unique_ptr<TH1F> mHistTrgValidation;
  std::unique_ptr<TH2F> mHistBcPattern;
  std::unique_ptr<TH2F> mHistBcTrgOutOfBunchColl;
  TProfile* mAmpl = nullptr;
  TProfile* mTime = nullptr;
  // Configurations
  int mLowTimeThreshold{ -192 };
  int mUpTimeThreshold{ 192 };
  std::string mAsynchChannelLogic{ "standard" };
  //
  void setTimestampToMOs();
  // TO REMOVE
  std::vector<unsigned int> mVecChannelIDs{};
  std::vector<std::string> mVecHistsToDecompose{};
  using HistDecomposed_t = TH1D;
  using MapHistsDecomposed_t = std::map<std::string, std::map<unsigned int, std::shared_ptr<HistDecomposed_t>>>;
  MapHistsDecomposed_t mMapHistsToDecompose{};
  void decomposeHists();
};

} // namespace o2::quality_control_modules::ft0

#endif // QC_MODULE_FT0_POSTPROCTASK_H
