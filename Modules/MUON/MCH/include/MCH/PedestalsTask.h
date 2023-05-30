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
/// \file   PedestalsTask.h
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_MUONCHAMBERS_PEDESTALSTASK_H
#define QC_MODULE_MUONCHAMBERS_PEDESTALSTASK_H

#include "QualityControl/TaskInterface.h"
#include "MCHRawElecMap/Mapper.h"
#include "MCH/GlobalHistogram.h"
#include "Framework/DataRef.h"
#include "MCHCalibration/PedestalData.h"
#include "MCHRawCommon/DataFormats.h"
#include <TH1F.h>
#include <TH2F.h>
#include <TCanvas.h>

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::muonchambers
{

/// \brief Quality Control Task for the analysis of MCH pedestal data
/// \author Andrea Ferrero
/// \author Sebastien Perrin
class PedestalsTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  PedestalsTask();
  /// Destructor
  ~PedestalsTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  template <typename T>
  void publishObject(T* histo, std::string drawOption, bool statBox)
  {
    histo->SetOption(drawOption.c_str());
    if (!statBox) {
      histo->SetStats(0);
    }
    mAllHistograms.push_back(histo);
    getObjectsManager()->startPublishing(histo);
    getObjectsManager()->setDefaultDrawOptions(histo, drawOption);
  }

  /// check if a given electronics channel is associated with a detector pad
  bool checkPadMapping(uint16_t feeId, uint8_t linkId, uint8_t eLinkId, o2::mch::raw::DualSampaChannelId channel, int& deId, int& padId);

  void monitorDataDigits(o2::framework::ProcessingContext& ctx);
  void monitorDataPedestals(o2::framework::ProcessingContext& ctx);
  void monitorDataBadChannels(o2::framework::ProcessingContext& ctx);

  void PlotPedestal(uint16_t solarID, uint8_t dsID, uint8_t channel, double stat, double mean, double rms);
  void PlotPedestalDE(uint16_t solarID, uint8_t dsID, uint8_t channel, double stat, double mean, double rms);
  void PlotBadChannel(uint16_t solarID, uint8_t dsID, uint8_t channel);
  void PlotBadChannelDE(uint16_t solarID, uint8_t dsID, uint8_t channel);
  void processElecMaps();

  static constexpr int sMaxFeeId = 64;
  static constexpr int sMaxLinkId = 12;
  static constexpr int sMaxDsId = 40;

  o2::mch::raw::Elec2DetMapper mElec2DetMapper;
  o2::mch::raw::Det2ElecMapper mDet2ElecMapper;
  o2::mch::raw::FeeLink2SolarMapper mFeeLink2SolarMapper;
  o2::mch::raw::Solar2FeeLinkMapper mSolar2FeeLinkMapper;

  /// helper class that performs the actual computation of the pedestals from the input digits
  o2::mch::calibration::PedestalData mPedestalData;

  std::unique_ptr<TH2F> mHistogramStat;
  std::unique_ptr<TH2F> mHistogramPedestals;
  std::unique_ptr<TH2F> mHistogramNoise;
  std::unique_ptr<TH2F> mHistogramBadChannels;

  std::unique_ptr<TH1F> mHistogramStatDE;
  std::unique_ptr<TH1F> mHistogramPedestalsDE;
  std::unique_ptr<TH1F> mHistogramNoiseDE;
  std::unique_ptr<TH1F> mHistogramEmptyChannelsDE;
  std::unique_ptr<TH1F> mHistogramBadChannelsDE;

  std::array<std::map<int, std::shared_ptr<DetectorHistogram>>, 2> mHistogramStatXY;
  std::array<std::map<int, std::shared_ptr<DetectorHistogram>>, 2> mHistogramPedestalsXY;
  std::array<std::map<int, std::shared_ptr<DetectorHistogram>>, 2> mHistogramNoiseXY;
  std::array<std::map<int, std::shared_ptr<DetectorHistogram>>, 2> mHistogramBadChannelsXY;

  std::map<int, std::shared_ptr<TH1F>> mHistogramNoiseDistributionDE[5][2];
  std::array<std::unique_ptr<TH1F>, 5> mHistogramNoiseDistribution;

  std::array<std::unique_ptr<GlobalHistogram>, 2> mHistogramStatMCH;
  std::array<std::unique_ptr<GlobalHistogram>, 2> mHistogramPedestalsMCH;
  std::array<std::unique_ptr<GlobalHistogram>, 2> mHistogramNoiseMCH;
  std::array<std::unique_ptr<GlobalHistogram>, 2> mHistogramBadChannelsMCH;

  std::unique_ptr<TCanvas> mCanvasCheckerMessages;

  int mPrintLevel;

  std::vector<TH1*> mAllHistograms;
};

} // namespace o2::quality_control_modules::muonchambers

#endif // QC_MODULE_MUONCHAMBERS_PEDESTALSTASK_H
