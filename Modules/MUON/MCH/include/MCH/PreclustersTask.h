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
/// \author Barthelemy von Haller
/// \author Piotr Konopka
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_MUONCHAMBERS_PRECLUSTERSTASK_H
#define QC_MODULE_MUONCHAMBERS_PRECLUSTERSTASK_H

#include "QualityControl/TaskInterface.h"
#include "MUONCommon/MergeableTH1Ratio.h"
#include "MUONCommon/MergeableTH2Ratio.h"
#include "MCHRawElecMap/Mapper.h"
#ifdef HAVE_DIGIT_IN_DATAFORMATS
#include "DataFormatsMCH/Digit.h"
#else
#include "MCHBase/Digit.h"
#endif
#include "MCHDigitFiltering/DigitFilter.h"
#include "MCHBase/PreCluster.h"

using namespace o2::quality_control_modules::muon;

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
  ~PreclustersTask() = default;

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

  void plotPrecluster(const o2::mch::PreCluster& preCluster, gsl::span<const o2::mch::Digit> digits);

  o2::mch::raw::Det2ElecMapper mDet2ElecMapper;
  o2::mch::raw::Solar2FeeLinkMapper mSolar2FeeLinkMapper;

  o2::mch::DigitFilter mIsSignalDigit;

  std::unique_ptr<MergeableTH2Ratio> mHistogramPseudoeffElec; // Mergeable object, Occupancy histogram (Elec view)

  std::unique_ptr<MergeableTH1Ratio> mHistogramPreclustersPerDE;       // number of pre-clusters per DE and per TF
  std::unique_ptr<MergeableTH1Ratio> mHistogramPreclustersSignalPerDE; // number of pre-clusters with signal per DE and per TF

  std::unique_ptr<TH2F> mHistogramClusterCharge;
  std::unique_ptr<TH2F> mHistogramClusterSize;

  std::vector<TH1*> mAllHistograms;
};

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_MUONCHAMBERS_PRECLUSTERSTASK_H
