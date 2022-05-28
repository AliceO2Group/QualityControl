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
/// \file   PhysicsTaskPreclusters.h
/// \author Barthelemy von Haller
/// \author Piotr Konopka
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_MUONCHAMBERS_PHYSICSTASKPRECLUSTERS_H
#define QC_MODULE_MUONCHAMBERS_PHYSICSTASKPRECLUSTERS_H

#include <vector>

#include <TH2.h>
#include "QualityControl/TaskInterface.h"
#include "MUONCommon/MergeableTH1Ratio.h"
#include "MUONCommon/MergeableTH2Ratio.h"
#include "MCH/MergeableTH1PseudoEfficiencyPerDE.h"
//#include "MCH/MergeableTH1PseudoEfficiencyPerDECycle.h"
//#include "MCH/MergeableTH1MPVPerDECycle.h"
#include "MCH/GlobalHistogram.h"
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
class PhysicsTaskPreclusters /*final*/ : public o2::quality_control::core::TaskInterface // todo add back the "final" when doxygen is fixed
{
 public:
  /// \brief Constructor
  PhysicsTaskPreclusters();
  /// Destructor
  ~PhysicsTaskPreclusters() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(o2::quality_control::core::Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(o2::quality_control::core::Activity& activity) override;
  void reset() override;

 private:
  template <typename T>
  void publishObject(std::shared_ptr<T> histo, std::string drawOption, bool statBox, bool isExpert)
  {
    histo->SetOption(drawOption.c_str());
    if (!statBox) {
      histo->SetStats(0);
    }
    mAllHistograms.push_back(histo.get());
    if (mDiagnostic || (isExpert == false)) {
      getObjectsManager()->startPublishing(histo.get());
      getObjectsManager()->setDefaultDrawOptions(histo.get(), drawOption);
    }
  }

  bool plotPrecluster(const o2::mch::PreCluster& preCluster, gsl::span<const o2::mch::Digit> digits);
  void printPrecluster(gsl::span<const o2::mch::Digit> preClusterDigits);
  void printPreclusters(gsl::span<const o2::mch::PreCluster> preClusters, gsl::span<const o2::mch::Digit> digits);

  void computePseudoEfficiency();
  bool getFecChannel(int deId, int padId, int& fecId, int& channel);

  static constexpr int sMaxFeeId = 64;
  static constexpr int sMaxLinkId = 12;
  static constexpr int sMaxDsId = 40;

  bool mDiagnostic{ false };

  o2::mch::raw::Elec2DetMapper mElec2DetMapper;
  o2::mch::raw::Det2ElecMapper mDet2ElecMapper;
  o2::mch::raw::FeeLink2SolarMapper mFeeLink2SolarMapper;
  o2::mch::raw::Solar2FeeLinkMapper mSolar2FeeLinkMapper;

  o2::mch::DigitFilter mIsSignalDigit;

  // TH1 of Mean Pseudo-efficiency on DEs
  // Since beginning of run
  // std::shared_ptr<MergeableTH1PseudoEfficiencyPerDE> mMeanPseudoeffPerDE[2]; // Pseudoefficiency of B and NB
  std::shared_ptr<MergeableTH2Ratio> mHistogramPseudoeffElec; // Mergeable object, Occupancy histogram (Elec view)

  std::shared_ptr<GlobalHistogram> mHistogramNumST12;          // Histogram of Number of hits (global XY view)
  std::shared_ptr<GlobalHistogram> mHistogramDenST12;          // Histogram of Number of orbits (global XY view)
  std::shared_ptr<MergeableTH2Ratio> mHistogramPseudoeffST12;  // Mergeable object, Occupancy histogram (global XY view)
  std::shared_ptr<GlobalHistogram> mHistogramNumST345;         // Histogram of Number of hits (global XY view)
  std::shared_ptr<GlobalHistogram> mHistogramDenST345;         // Histogram of Number of orbits (global XY view)
  std::shared_ptr<MergeableTH2Ratio> mHistogramPseudoeffST345; // Mergeable object, Occupancy histogram (global XY view)

  std::shared_ptr<MergeableTH1Ratio> mHistogramMeanPseudoeffPerDE[2]; // Pseudoefficiency of B and NB

  std::shared_ptr<MergeableTH1Ratio> mHistogramPreclustersPerDE;       // number of pre-clusters per DE and per TF
  std::shared_ptr<MergeableTH1Ratio> mHistogramPreclustersSignalPerDE; // number of pre-clusters with signal per DE and per TF

  std::map<int, std::shared_ptr<TH1F>> mHistogramClchgDE;
  std::map<int, std::shared_ptr<TH1F>> mHistogramClchgDEOnCycle;
  std::map<int, std::shared_ptr<TH1F>> mHistogramClsizeDE;
  std::map<int, std::shared_ptr<TH1F>> mHistogramClsizeDE_B;
  std::map<int, std::shared_ptr<TH1F>> mHistogramClsizeDE_NB;

  std::map<int, std::shared_ptr<DetectorHistogram>> mHistogramPreclustersXY[4];
  std::map<int, std::shared_ptr<MergeableTH2Ratio>> mHistogramPseudoeffXY[2];

  std::vector<TH1*> mAllHistograms;
};

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_MUONCHAMBERS_PHYSICSDATAPROCESSOR_H
