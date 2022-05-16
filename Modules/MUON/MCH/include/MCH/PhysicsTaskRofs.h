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
/// \file   PhysicsTaskRofs.h
/// \author Andrea Ferrero
/// \author Sebastien Perrin
///

#ifndef QC_MODULE_MCH_PHYSICSTASKROFS_H
#define QC_MODULE_MCH_PHYSICSTASKROFS_H

#include <TH2.h>
#include <gsl/span>

#include "DataFormatsMCH/Digit.h"
#include "DataFormatsMCH/ROFRecord.h"
#include "MUONCommon/MergeableTH2Ratio.h"
#include "MCHDigitFiltering/DigitFilter.h"
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
class PhysicsTaskRofs /*final*/ : public TaskInterface
{
 public:
  /// \brief Constructor
  PhysicsTaskRofs();
  /// Destructor
  ~PhysicsTaskRofs() override;

  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  void plotROF(const o2::mch::ROFRecord& rof, gsl::span<const o2::mch::Digit> digits, int plotId);
  /// \brief helper function for storing the histograms to a ROOT file on disk
  void writeHistos();

  bool mSaveToRootFile{ false }; ///< flag for saving the histograms to a local ROOT file

  o2::mch::DigitFilter mIsSignalDigit; ///< functor to select signal-like digits

  std::shared_ptr<TH1F> mHistRofSize[2];       ///< number of digits per ROF (all and filtered)
  std::shared_ptr<TH1F> mHistRofSizeSignal[2]; ///< number of signal-like digits per ROF (all and filtered)
  std::shared_ptr<TH1F> mHistRofWidth[2];      ///< ROF width in BC (all and filtered)
  std::shared_ptr<TH1F> mHistRofNStations[2];  ///< number of stations per ROF (all and filtered)
  std::shared_ptr<TH1F> mHistRofFilterRatio;   ///< fraction of ROFs remaining after filtering

  std::vector<TH1*> mAllHistograms;
};

} // namespace o2::quality_control_modules::muonchambers

#endif // QC_MODULE_MCH_DECODINGERRORSTASK_H
