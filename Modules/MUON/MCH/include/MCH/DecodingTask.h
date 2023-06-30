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
/// \file   DecodingTask.h
/// \author Andrea Ferrero
/// \author Sebastien Perrin
///

#ifndef QC_MODULE_MCH_DECODINGTASK_H
#define QC_MODULE_MCH_DECODINGTASK_H

#include "MUONCommon/MergeableTH2Ratio.h"
#include "MCH/Helpers.h"
#include <DPLUtils/DPLRawParser.h>
#include <MCHRawDecoder/PageDecoder.h>
#include "MCHGlobalMapping/DsIndex.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/TaskInterface.h"
#include "QualityControl/CheckInterface.h"
#include "QualityControl/MonitorObject.h"

using namespace o2::quality_control_modules::muon;

namespace o2::quality_control_modules::muonchambers
{

/// \brief Quality Control Task for the analysis of raw data decoding errors
/// \author Andrea Ferrero
/// \author Sebastien Perrin
class DecodingTask /*final*/ : public TaskInterface
{
 public:
  /// \brief Constructor
  DecodingTask() = default;
  /// Destructor
  ~DecodingTask() override = default;

  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  /// \brief functions that instatiate and publish the plots
  void createErrorHistos();
  void createHeartBeatHistos();
  /// \brief decoding of the TimeFrame data sent from the (sub)TimeFrame builder
  void decodeTF(framework::ProcessingContext& pc);
  /// \brief decoding of the TimeFrame data sent directly from readout
  void decodeReadout(const o2::framework::DataRef& input);
  /// \brief helper function to process the TimeFrame data
  void decodeBuffer(gsl::span<const std::byte> buf);
  /// \brief helper function to process one CRU page
  void decodePage(gsl::span<const std::byte> page);
  /// \brief helper function to process the array of errors sent via DPL
  void processErrors(framework::ProcessingContext& pc);
  /// \brief helper function for updating the error histograms
  void plotError(uint16_t solarId, int dsAddr, int chip, uint32_t error);
  /// \brief helper function to process the heart-beat packets sent via DPL
  void processHBPackets(framework::ProcessingContext& pc);
  /// \brief helper function for updating the heart-beat packets histograms
  void plotHBPacket(uint16_t solarId, int dsAddr, int chip, uint32_t bc);
  /// \brief checker function for the heart-beat packets histograms
  void updateSyncErrors();

  /// \brief helper function for pubishing and formatting a given plot
  template <typename T>
  void publishObject(T* histo, std::string drawOption, std::string displayHints, bool statBox, bool isExpert);

  /// \brief electronics mapping functions
  o2::mch::raw::Elec2DetMapper mElec2DetMapper{ nullptr };

  /// \brief raw data decoder
  o2::mch::raw::PageDecoder mDecoder;

  /// \brief expected bunch-crossing value in heart-beat packets
  int mHBExpectedBc{ 456190 };

  struct HBCount {
    HBCount() = default;
    uint16_t nSync{ 0 };
    uint16_t nOutOfSync{ 0 };
  };
  std::array<HBCount, o2::mch::NumberOfDualSampas> mHBcount;

  /// \brief number of processed time-frames
  std::unique_ptr<TH1F> mHistogramTimeFramesCount;

  /// \brief decoding error plots
  std::unique_ptr<MergeableTH2Ratio> mHistogramErrorsFEC; ///< error codes per FEC

  /// \brief time synchronization diagnostics plots
  std::unique_ptr<MergeableTH2Ratio> mHistogramHBTimeFEC;       ///< bunch-crossing from HB packets vs. FEC id
  std::unique_ptr<MergeableTH2Ratio> mHistogramHBCoarseTimeFEC; ///< bunch-crossing from HB packets vs. FEC id, coarse scale
  std::unique_ptr<MergeableTH2Ratio> mSyncStatusFEC;            ///< time synchronization status of each DS board (OK, out-of-synch, missing good HB)

  std::vector<TH1*> mAllHistograms;
};

template <typename T>
void DecodingTask::publishObject(T* histo, std::string drawOption, std::string displayHints, bool statBox, bool isExpert)
{
  histo->SetOption(drawOption.c_str());
  if (!statBox) {
    histo->SetStats(0);
  }
  mAllHistograms.push_back(histo);
  getObjectsManager()->startPublishing(histo);
  getObjectsManager()->setDefaultDrawOptions(histo, drawOption);
  getObjectsManager()->setDisplayHint(histo, displayHints);
}

} // namespace o2::quality_control_modules::muonchambers

#endif // QC_MODULE_MCH_DECODINGTASK_H
