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
/// \file   PreclustersTask.h
/// \author Andrea Ferrero
/// \author Sebastien Perrin
///

#ifndef QC_MODULE_MUONCHAMBERS_PRECLUSTERSTASK_H
#define QC_MODULE_MUONCHAMBERS_PRECLUSTERSTASK_H

#include "QualityControl/TaskInterface.h"
#include "Common/TH1Ratio.h"
#include "Common/TH2Ratio.h"
#ifdef HAVE_DIGIT_IN_DATAFORMATS
#include "DataFormatsMCH/Digit.h"
#else
#include "MCHBase/Digit.h"
#endif
#include "MCHDigitFiltering/DigitFilter.h"
#include "MCHBase/PreCluster.h"

using namespace o2::quality_control_modules::common;

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

/// \brief Quality Control Task for the analysis of MCH physics data
/// \author Andrea Ferrero
/// \author Sebastien Perrin
class PreclustersTask /*final*/ : public o2::quality_control::core::TaskInterface // todo add back the "final" when doxygen is fixed
{
 public:
  /// \brief Constructor
  PreclustersTask() = default;
  /// Destructor
  ~PreclustersTask() override = default;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const o2::quality_control::core::Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const o2::quality_control::core::Activity& activity) override;
  void reset() override;

 private:
  template <typename T>
  void publishObject(T* histo, std::string drawOption, bool statBox);

  void plotPrecluster(const o2::mch::PreCluster& preCluster, gsl::span<const o2::mch::Digit> digits);

  o2::mch::DigitFilter mIsSignalDigit;

  std::unique_ptr<TH2FRatio> mHistogramPseudoeffElec; // Mergeable object, Occupancy histogram (Elec view)

  std::unique_ptr<TH1DRatio> mHistogramPreclustersPerDE;       // number of pre-clusters per DE and per TF
  std::unique_ptr<TH1DRatio> mHistogramPreclustersSignalPerDE; // number of pre-clusters with signal per DE and per TF

  std::unique_ptr<TH2F> mHistogramClusterCharge;
  std::unique_ptr<TH2F> mHistogramClusterSize;

  std::vector<TH1*> mAllHistograms;
};

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_MUONCHAMBERS_PRECLUSTERSTASK_H
