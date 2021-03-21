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
#if HAVE_DIGIT_IN_DATAFORMATS
#include "DataFormatsMCH/Digit.h"
#else
#include "MCHBase/Digit.h"
#endif
#include "MCHBase/PreCluster.h"

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

  bool plotPrecluster(const o2::mch::PreCluster& preCluster, gsl::span<const o2::mch::Digit> digits);
  void printPreclusters(gsl::span<const o2::mch::PreCluster> preClusters, gsl::span<const o2::mch::Digit> digits);

 private:
  double MeanPseudoeffDE[1100];
  double MeanPseudoeffDECycle[1100];
  // Arrays needed to compute the Pseudo-efficiency on the elapsed cycle
  double LastPreclBNBDE[1100];
  double NewPreclBNBDE[1100];
  double LastPreclNumDE[1100];
  double NewPreclNumDE[1100];

  std::vector<std::unique_ptr<mch::Digit>> digits;

  // TH1 of Mean Pseudo-efficiency on DEs, integrated or only on the elapsed cycle - Sent for Trending
  TH1F* mMeanPseudoeffPerDE;
  TH1F* mMeanPseudoeffPerDECycle;

  std::map<int, TH1F*> mHistogramClchgDE;
  std::map<int, TH1F*> mHistogramClchgDEOnCycle;
  std::map<int, TH1F*> mHistogramClsizeDE;

  std::map<int, TH2F*> mHistogramPreclustersXY[4];
  std::map<int, TH2F*> mHistogramPseudoeffXY[3];

  GlobalHistogram* mHistogramPseudoeff[3];
};

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_MUONCHAMBERS_PHYSICSDATAPROCESSOR_H
