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
#include "MCH/GlobalHistogram.h"
#ifdef HAVE_DIGIT_IN_DATAFORMATS
#include "DataFormatsMCH/Digit.h"
#else
#include "MCHBase/Digit.h"
#endif
#include "MCHBase/PreCluster.h"
#include "MCH/MergeableTH2Ratio.h"
#include "MCH/MergeableTH1PseudoEfficiencyPerDE.h"
//#include "MCH/MergeableTH1PseudoEfficiencyPerDECycle.h"
//#include "MCH/MergeableTH1MPVPerDECycle.h"

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
  bool plotPrecluster(const o2::mch::PreCluster& preCluster, gsl::span<const o2::mch::Digit> digits);
  void printPrecluster(gsl::span<const o2::mch::Digit> preClusterDigits);
  void printPreclusters(gsl::span<const o2::mch::PreCluster> preClusters, gsl::span<const o2::mch::Digit> digits);

  void computePseudoEfficiency();
  void writeHistos();

  // TH1 of Mean Pseudo-efficiency on DEs
  //Since beginning of run
  std::shared_ptr<MergeableTH1PseudoEfficiencyPerDE> mMeanPseudoeffPerDE[2]; //Pseudoefficiency of B and NB

  //On last cycle only
  // WARNING: the code for the efficiency on cycle is currently broken and therefore disabled
  //MergeableTH1PseudoEfficiencyPerDECycle* mMeanPseudoeffPerDE_DoesGoodNBHaveSomethingB_Cycle;
  //MergeableTH1PseudoEfficiencyPerDECycle* mMeanPseudoeffPerDE_DoesGoodBHaveSomethingNB_Cycle;
  //MergeableTH1MPVPerDECycle* mMPVCycle; //MPV of the Landau best fitted to the cluster charge distribution on each DE each cycle

  std::shared_ptr<TH2F> mDigitChargeVsSize[4];
  std::shared_ptr<TH2F> mDigitClsizeVsCharge[2];
  std::shared_ptr<TH2F> mDigitChargeNBVsChargeB;

  std::map<int, std::shared_ptr<TH2F>> mHistogramClchgDE;
  std::map<int, std::shared_ptr<TH1F>> mHistogramClchgDEOnCycle;
  std::map<int, std::shared_ptr<TH2F>> mHistogramClsizeDE;

  std::map<int, std::shared_ptr<DetectorHistogram>> mHistogramPreclustersXY[4];
  std::map<int, std::shared_ptr<MergeableTH2Ratio>> mHistogramPseudoeffXY[2];

  std::vector<TH1*> mAllHistograms;
};

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_MUONCHAMBERS_PHYSICSDATAPROCESSOR_H
