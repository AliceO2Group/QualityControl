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
/// \file   DigitQcTask.h
/// \author Artur Furs afurs@cern.ch
/// modified by Sebastin Bysiak sbysiak@cern.ch
/// QC Task for FT0 detector, mostly for data visualisation during FEE tests

#ifndef QC_MODULE_FT0_FT0RECOQCTASK_H
#define QC_MODULE_FT0_FT0RECOQCTASK_H

#include <Framework/InputRecord.h>
#include <FT0/AmpTimeDistribution.h>
#include "QualityControl/QcInfoLogger.h"
#include <FT0Base/Geometry.h>
#include <DataFormatsFT0/RecPoints.h>
#include <DataFormatsFT0/ChannelData.h>
#include "QualityControl/TaskInterface.h"
#include "FITCommon/DetectorFIT.h"
#include <memory>
#include <regex>
#include <set>
#include <map>
#include <string>
#include <array>
#include <type_traits>
#include <boost/algorithm/string.hpp>
#include <TH1.h>
#include <TH2.h>
#include <TList.h>
#include <Rtypes.h>

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::ft0
{

class RecPointsQcTask final : public TaskInterface
{
  static constexpr int sNCHANNELS = o2::ft0::Geometry::Nchannels;

 public:
  /// \brief Constructor
  RecPointsQcTask() = default;
  /// Destructor
  ~RecPointsQcTask() override;
  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;
  using Detector_t = o2::quality_control_modules::fit::detectorFIT::DetectorFT0;
  void initHists();

 private:
  std::array<o2::fit::AmpTimeDistribution, sNCHANNELS> mArrAmpTimeDistribution;
  TList* mListHistGarbage;
  std::set<unsigned int> mSetAllowedChIDs;
  unsigned int mTrgPos_minBias;
  unsigned int mTrgPos_allEvents;
  // Objects which will be published
  std::unique_ptr<TH2F> mHistAmp2Ch;
  std::unique_ptr<TH2F> mHistTime2Ch;
  std::unique_ptr<TH1F> mHistCollTimeAC;
  std::unique_ptr<TH1F> mHistCollTimeA;
  std::unique_ptr<TH1F> mHistCollTimeC;
  std::unique_ptr<TH2F> mHistSumTimeAC_perTrg;
  std::unique_ptr<TH2F> mHistDiffTimeAC_perTrg;
  std::unique_ptr<TH2F> mHistTimeA_perTrg;
  std::unique_ptr<TH2F> mHistTimeC_perTrg;
  std::unique_ptr<TH2F> mHistBC_perTriggers;
  std::unique_ptr<TH1F> mHistResCollTimeA;
  std::unique_ptr<TH1F> mHistResCollTimeC;
  std::map<unsigned int, TH2F*> mMapHistAmpVsTime;

  typename Detector_t::TrgMap_t mMapTrgBits = Detector_t::sMapTrgBits;
};

} // namespace o2::quality_control_modules::ft0

#endif // QC_MODULE_FT0_FT0RecoQcTask_H
