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

#ifndef QC_MODULE_FDD_POSTPROCTASK_H
#define QC_MODULE_FDD_POSTPROCTASK_H

#include "QualityControl/PostProcessingInterface.h"
#include "QualityControl/DatabaseInterface.h"
#include "CCDB/CcdbApi.h"
#include "CommonConstants/LHCConstants.h"

#include "FDDBase/Constants.h"
#include "DataFormatsFDD/ChannelData.h"
#include "DataFormatsFDD/Digit.h"

#include <TH2.h>
#include <TCanvas.h>
#include <TGraph.h>

class TH1F;
class TCanvas;
class TLegend;
class TProfile;

namespace o2::quality_control_modules::fdd
{

/// \brief Basic Postprocessing Task for FDD, computes among others the trigger rates
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
  constexpr static std::size_t sNCHANNELS_PM = 20;

 private:
  std::string mPathGrpLhcIf;
  std::string mPathDigitQcTask;
  std::string mCycleDurationMoName;
  std::string mCcdbUrl;
  std::string mTimestampSourceLhcIf;
  int mNumOrbitsInTF;
  const unsigned int mNumTriggers = 5;

  std::map<o2::fdd::ChannelData::EEventDataBit, std::string> mMapChTrgNames;
  std::map<int, std::string> mMapDigitTrgNames;

  o2::quality_control::repository::DatabaseInterface* mDatabase = nullptr;
  o2::ccdb::CcdbApi mCcdbApi;

  std::unique_ptr<TGraph> mRateOrA;
  std::unique_ptr<TGraph> mRateOrC;
  std::unique_ptr<TGraph> mRateVertex;
  std::unique_ptr<TGraph> mRateCentral;
  std::unique_ptr<TGraph> mRateSemiCentral;
  std::unique_ptr<TH2F> mHistChDataNegBits;
  std::unique_ptr<TH1F> mHistTriggers;

  std::unique_ptr<TH1F> mHistTimeUpperFraction;
  std::unique_ptr<TH1F> mHistTimeLowerFraction;
  std::unique_ptr<TH1F> mHistTimeInWindow;

  std::unique_ptr<TCanvas> mRatesCanv;
  TProfile* mAmpl = nullptr;
  TProfile* mTime = nullptr;

  // if storage size matters it can be replaced with TH1
  // and TH2 can be created based on it on the fly, but only TH1 would be stored
  std::unique_ptr<TH2F> mHistBcPattern;
  std::unique_ptr<TH2F> mHistBcTrgOutOfBunchColl;

  std::map<unsigned int, TH1D*> mMapTrgHistBC;
};

} // namespace o2::quality_control_modules::fdd

#endif // QC_MODULE_FDD_POSTPROCTASK_H
