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

 private:
  std::string mPathGrpLhcIf;
  std::string mPathDigitQcTask;
  std::string mCycleDurationMoName;
  std::string mCcdbUrl;
  std::string mTimestampSourceLhcIf;
  int mNumOrbitsInTF;
  const unsigned int mNumTriggers = 5;

  std::map<unsigned int, std::string> mMapChTrgNames;
  std::map<unsigned int, std::string> mMapDigitTrgNames;
  std::map<unsigned int, std::string> mMapBasicTrgBits;
  o2::quality_control::repository::DatabaseInterface* mDatabase = nullptr;
  o2::ccdb::CcdbApi mCcdbApi;

  std::unique_ptr<TGraph> mRateOrA;
  std::unique_ptr<TGraph> mRateOrC;
  std::unique_ptr<TGraph> mRateVertex;
  std::unique_ptr<TGraph> mRateCentral;
  std::unique_ptr<TGraph> mRateSemiCentral;
  std::unique_ptr<TH2F> mHistChDataNegBits;
  std::unique_ptr<TH1F> mHistTriggers;

  std::unique_ptr<TH1F> mHistTimeInWindow;

  std::unique_ptr<TH1F> mHistChannelID_outOfBC;
  std::unique_ptr<TH1F> mHistTrgValidation;

  std::unique_ptr<TCanvas> mRatesCanv;
  TProfile* mAmpl;
  TProfile* mTime;

  // if storage size matters it can be replaced with TH1
  // and TH2 can be created based on it on the fly, but only TH1 would be stored
  std::unique_ptr<TH2F> mHistBcPattern;
  std::unique_ptr<TH2F> mHistBcTrgOutOfBunchColl;

  std::map<unsigned int, TH1D*> mMapTrgHistBC;
  int mLowTimeThreshold{ -192 };
  int mUpTimeThreshold{ 192 };
  std::string mAsynchChannelLogic{ "standard" };
};

} // namespace o2::quality_control_modules::ft0

#endif // QC_MODULE_FT0_POSTPROCTASK_H
