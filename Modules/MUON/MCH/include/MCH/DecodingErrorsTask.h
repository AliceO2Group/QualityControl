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
/// \file   DecodingErrorsTask.h
/// \author Andrea Ferrero
/// \author Sebastien Perrin
///

#ifndef QC_MODULE_MCH_DECODINGERRORSTASK_H
#define QC_MODULE_MCH_DECODINGERRORSTASK_H

#include "MUONCommon/MergeableTH1Ratio.h"
#include "MUONCommon/MergeableTH2Ratio.h"
#include <DPLUtils/DPLRawParser.h>
#include <MCHRawDecoder/PageDecoder.h>
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/TaskInterface.h"
#include "QualityControl/CheckInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

using namespace o2::quality_control_modules::muon;

namespace o2::quality_control_modules::muonchambers
{

/// \brief Quality Control Task for the analysis of raw data decoding errors
/// \author Andrea Ferrero
/// \author Sebastien Perrin
class DecodingErrorsTask /*final*/ : public TaskInterface
{
 public:
  /// \brief Constructor
  DecodingErrorsTask();
  /// Destructor
  ~DecodingErrorsTask() override;

  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
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
  void plotError(int solarId, int dsAddr, int chip, uint32_t error);
  /// \brief helper function to process the heart-beat packets sent via DPL
  void processHBPackets(framework::ProcessingContext& pc);
  /// \brief helper function for updating the heart-beat packets histograms
  void plotHBPacket(int solarId, int dsAddr, int chip, uint32_t bc);
  /// \brief checker function for the heart-beat packets histograms
  void checkSyncErrors();
  /// \brief function returning the detection element corresponding to a given FEEID/LINKID/ELINKID triplet
  int getDeId(uint16_t feeId, uint8_t linkId, uint8_t eLinkId);

  /// \brief helper function for pubishing and formatting a given plot
  template <typename T>
  void publishObject(std::shared_ptr<T> histo, std::string drawOption, bool statBox, bool isExpert);

  /// \brief functions providing the forward and backward electronics mapping
  mch::raw::Elec2DetMapper mElec2Det{ nullptr };
  mch::raw::FeeLink2SolarMapper mFee2Solar{ nullptr };
  o2::mch::raw::Solar2FeeLinkMapper mSolar2Fee{ nullptr };

  /// \brief raw data decoder
  o2::mch::raw::PageDecoder mDecoder;

  /// \brief expected bunch-crossing value in heart-beat packets
  int mHBExpectedBc{ 456190 };

  /// \brief number of processed time-frames
  std::shared_ptr<TH1F> mHistogramTimeFramesCount;

  /// \brief decoding error plots
  std::shared_ptr<MergeableTH2Ratio> mHistogramErrorsPerChamber; ///< error codes per chamber
  std::shared_ptr<MergeableTH2Ratio> mHistogramErrorsPerFeeId;   ///< error codes per detection element
  /// \brief histograms up to previously processed cycle
  std::shared_ptr<MergeableTH2Ratio> mHistogramErrorsPerChamberPrevCycle; ///< error codes per chamber
  std::shared_ptr<MergeableTH2Ratio> mHistogramErrorsPerFeeIdPrevCycle;   ///< error codes per detection element
  /// \brief histograms from last processed cycle
  std::shared_ptr<MergeableTH2Ratio> mHistogramErrorsPerChamberOnCycle; ///< error codes per chamber
  std::shared_ptr<MergeableTH2Ratio> mHistogramErrorsPerFeeIdOnCycle;   ///< error codes per detection element

  /// \brief time synchronization diagnostics plots
  std::shared_ptr<MergeableTH2Ratio> mHistogramHBTimeFEC;             ///< bunch-crossing from HB packets vs. FEC id
  std::shared_ptr<MergeableTH2Ratio> mHistogramHBCoarseTimeFEC;       ///< bunch-crossing from HB packets vs. FEC id, coarse scale
  std::shared_ptr<MergeableTH1Ratio> mHistogramSynchErrorsPerDE;      ///< fraction of out-of-sync DS boards per detection element
  std::shared_ptr<MergeableTH1Ratio> mHistogramSynchErrorsPerChamber; ///< fraction of out-of-sync DS boards per chamber
  std::shared_ptr<TH2F> mSyncStatusFEC;                               ///< time synchronization status of each DS board (OK, out-of-synch, missing good HB)

  std::vector<TH1*> mAllHistograms;
};

template <typename T>
void DecodingErrorsTask::publishObject(std::shared_ptr<T> histo, std::string drawOption, bool statBox, bool isExpert)
{
  histo->SetOption(drawOption.c_str());
  if (!statBox) {
    histo->SetStats(0);
  }
  mAllHistograms.push_back(histo.get());
  getObjectsManager()->startPublishing(histo.get());
  getObjectsManager()->setDefaultDrawOptions(histo.get(), drawOption);
}

} // namespace o2::quality_control_modules::muonchambers

#endif // QC_MODULE_MCH_DECODINGERRORSTASK_H
