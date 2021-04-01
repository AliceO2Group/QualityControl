///
/// \file   PhysicsTaskDigits.h
/// \author Barthelemy von Haller
/// \author Piotr Konopka
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_MUONCHAMBERS_PHYSICSTASKDIGITS_H
#define QC_MODULE_MUONCHAMBERS_PHYSICSTASKDIGITS_H

#include "QualityControl/TaskInterface.h"
#include "MCHRawElecMap/Mapper.h"
#ifdef HAVE_DIGIT_IN_DATAFORMATS
#include "DataFormatsMCH/Digit.h"
#else
#include "MCHBase/Digit.h"
#endif
#include "MCH/GlobalHistogram.h"

class TH1F;
class TH2F;

#define MCH_FEEID_NUM 64

using namespace o2::quality_control::core;

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

/// \brief Quality Control Task for the analysis of MCH physics data
/// \author Andrea Ferrero
/// \author Sebastien Perrin
class PhysicsTaskDigits /*final*/ : public TaskInterface // todo add back the "final" when doxygen is fixed
{
 public:
  /// \brief Constructor
  PhysicsTaskDigits();
  /// Destructor
  ~PhysicsTaskDigits() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  void plotDigit(const o2::mch::Digit& digit);

  o2::mch::raw::Elec2DetMapper mElec2DetMapper;
  o2::mch::raw::Det2ElecMapper mDet2ElecMapper;
  o2::mch::raw::FeeLink2SolarMapper mFeeLink2SolarMapper;
  o2::mch::raw::Solar2FeeLinkMapper mSolar2FeeLinkMapper;

  uint32_t norbits[MCH_FEEID_NUM][12];
  uint32_t lastorbitseen[MCH_FEEID_NUM][12];
  // Mesn Occupancy on each DE
  double MeanOccupancyDE[1100];
  // Mean Occupancy on each DE during the elapsed cycle, and arrays needed to compute it
  double MeanOccupancyDECycle[1100];
  // Arrays needed for the computation of Occupancy on elapsed cycle
  double LastMeanNhitsDE[1100];
  double LastMeanNorbitsDE[1100];
  double NewMeanNhitsDE[1100];
  double NewMeanNorbitsDE[1100];

  int NbinsDE[1100];

  // 2D Histograms, using Elec view (where x and y uniquely identify each pad based on its Elec info (fee, link, de)
  TH2F* mHistogramNHitsElec;
  TH2F* mHistogramNorbitsElec;
  TH2F* mHistogramOccupancyElec;

  // TH1 of the Mean Occupancy on each DE, integrated or only on elapsed cycle - Sent for Trending
  TH1F* mMeanOccupancyPerDE;
  TH1F* mMeanOccupancyPerDECycle;

  TH2F* mHistogramNhits[1100];
  TH1F* mHistogramADCamplitude[1100];
  std::map<int, TH1F*> mHistogramADCamplitudeDE;
  std::map<int, TH2F*> mHistogramNhitsDE[2];
  std::map<int, TH2F*> mHistogramNorbitsDE[2];
  std::map<int, TH2F*> mHistogramOccupancyXY[2];

  GlobalHistogram* mHistogramOccupancy[1];
  GlobalHistogram* mHistogramOrbits[1];
};

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_MUONCHAMBERS_PHYSICSDATAPROCESSOR_H
