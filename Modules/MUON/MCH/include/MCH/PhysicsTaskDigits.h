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
#include "MCH/MergeableTH2Ratio.h"

class TH1F;
class TH2F;

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
  void storeOrbit(const uint64_t& orb);
  void plotDigit(const o2::mch::Digit& digit);
  void updateOrbits();

  static constexpr int sMaxFeeId = 64;
  static constexpr int sMaxLinkId = 12;
  static constexpr int sMaxDsId = 40;

  o2::mch::raw::Elec2DetMapper mElec2DetMapper;
  o2::mch::raw::Det2ElecMapper mDet2ElecMapper;
  o2::mch::raw::FeeLink2SolarMapper mFeeLink2SolarMapper;
  o2::mch::raw::Solar2FeeLinkMapper mSolar2FeeLinkMapper;

  uint32_t mNOrbits[sMaxFeeId][sMaxLinkId];
  uint32_t mLastOrbitSeen[sMaxFeeId][sMaxLinkId];

  // 2D Histograms, using Elec view (where x and y uniquely identify each pad based on its Elec info (fee, link, de)
  TH2F* mHistogramNHits[sMaxFeeId * sMaxLinkId]; // Histogram of Number of hits per LinkId and FeeId
  TH2F* mHistogramNHitsElec;                     // Histogram of Number of hits (Elec view)
  TH2F* mHistogramNorbitsElec;                   // Histogram of Number of orbits (Elec view)
  MergeableTH2Ratio* mHistogramOccupancyElec;    // Mergeable object, Occupancy histogram (Elec view)

  std::map<int, TH1F*> mHistogramADCamplitudeDE;              // Histogram of ADC distribution per DE
  std::map<int, TH2F*> mHistogramNhitsDE[2];                  // Histogram of Number of hits (XY view)
  std::map<int, TH2F*> mHistogramNorbitsDE[2];                // Histogram of Number of orbits (XY view)
  std::map<int, MergeableTH2Ratio*> mHistogramOccupancyDE[2]; // Mergeable object, Occupancy histogram (XY view)

  GlobalHistogram* mHistogramNHitsAllDE;       // Global histogram (all DE) of Number of hits
  GlobalHistogram* mHistogramOrbitsAllDE;      // Global histogram (all DE) of Number of orbits
  MergeableTH2Ratio* mHistogramOccupancyAllDE; //Mergeable object, Global histogram (all DE) of Occupancy
};

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_MUONCHAMBERS_PHYSICSDATAPROCESSOR_H
