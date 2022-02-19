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

#include <TH2.h>

#include "MUONCommon/MergeableTH2Ratio.h"
#include <DPLUtils/DPLRawParser.h>
#include <MCHRawDecoder/PageDecoder.h>
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/TaskInterface.h"
#include "QualityControl/CheckInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

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

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  // decoding of the TimeFrame data sent from the (sub)TimeFrame builder
  void decodeTF(framework::ProcessingContext& pc);
  // decoding of the TimeFrame data sent directly from readout
  void decodeReadout(const o2::framework::DataRef& input);
  // helper function to process the TimeFrame data
  void decodeBuffer(gsl::span<const std::byte> buf);
  // helper function to process one CRU page
  void decodePage(gsl::span<const std::byte> page);

  // helper function for storing the histograms to a ROOT file on disk
  void writeHistos();

  mch::raw::Elec2DetMapper mElec2Det{ nullptr };
  mch::raw::FeeLink2SolarMapper mFee2Solar{ nullptr };
  o2::mch::raw::Solar2FeeLinkMapper mSolar2Fee{ nullptr };
  o2::mch::raw::PageDecoder mDecoder;

  bool mSaveToRootFile{ false }; ///< flag for saving the histograms to a local ROOT file

  std::shared_ptr<MergeableTH2Ratio> mHistogramErrorsPerChamber; ///< histogram to visualize the decoding errors
  std::shared_ptr<MergeableTH2Ratio> mHistogramErrorsPerFeeId;   ///< histogram to visualize the decoding errors

  std::vector<TH1*> mAllHistograms;
};

} // namespace o2::quality_control_modules::muonchambers

#endif // QC_MODULE_MCH_DECODINGERRORSTASK_H
