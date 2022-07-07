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
#include "MUONCommon/MergeableTH2Ratio.h"
#include "MCH/GlobalHistogram.h"

class TH1F;
class TH2F;

using namespace o2::quality_control::core;
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
  void addDefaultOrbitsInTF();
  void plotDigit(const o2::mch::Digit& digit);
  void updateOrbits();

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

  struct GlobalHistogramRatio {
    std::shared_ptr<GlobalHistogram> mHistNum; // Histogram of Number of hits (global XY view)
    std::shared_ptr<GlobalHistogram> mHistDen; // Histogram of Number of orbits (global XY view)
    std::shared_ptr<MergeableTH2Ratio> mHistOccupancy;
    GlobalHistogramRatio(std::string name, std::string title, int id);
    void setNum(std::map<int, std::shared_ptr<DetectorHistogram>>& histB, std::map<int, std::shared_ptr<DetectorHistogram>>& histNB);
    void setDen(std::map<int, std::shared_ptr<DetectorHistogram>>& histB, std::map<int, std::shared_ptr<DetectorHistogram>>& histNB);
    void update();
  };

  static constexpr int sMaxFeeId = 64;
  static constexpr int sMaxLinkId = 12;
  static constexpr int sMaxDsId = 40;

  bool mOnCycle{ false };    // publish plots from last processed cycle
  bool mDiagnostic{ false }; // publish extra diagnostics plots

  o2::mch::raw::Elec2DetMapper mElec2DetMapper;
  o2::mch::raw::Det2ElecMapper mDet2ElecMapper;
  o2::mch::raw::FeeLink2SolarMapper mFeeLink2SolarMapper;
  o2::mch::raw::Solar2FeeLinkMapper mSolar2FeeLinkMapper;

  uint32_t mNOrbits[sMaxFeeId][sMaxLinkId];
  uint32_t mLastOrbitSeen[sMaxFeeId][sMaxLinkId];

  // 2D Histograms, using Elec view (where x and y uniquely identify each pad based on its Elec info (fee, link, de)
  TH2F* mHistogramNHitsElec;                                  // Histogram of Number of hits (Elec view)
  TH2F* mHistogramNorbitsElec;                                // Histogram of Number of orbits (Elec view)
  std::shared_ptr<MergeableTH2Ratio> mHistogramOccupancyElec; // Mergeable object, Occupancy histogram (Elec view)

  std::shared_ptr<GlobalHistogramRatio> mHistogramOccupancyST12;  // Mergeable object, Occupancy histogram (global XY view)
  std::shared_ptr<GlobalHistogramRatio> mHistogramOccupancyST345; // Mergeable object, Occupancy histogram (global XY view)

  // on-cycle occupancy plots
  std::shared_ptr<GlobalHistogramRatio> mHistogramOccupancyPrevCycleST12;
  std::shared_ptr<GlobalHistogramRatio> mHistogramOccupancyPrevCycleST345;
  std::shared_ptr<GlobalHistogramRatio> mHistogramOccupancyOnCycleST12;
  std::shared_ptr<GlobalHistogramRatio> mHistogramOccupancyOnCycleST345;

  std::shared_ptr<TH2F> mHistogramDigitsOrbitInTF;
  std::shared_ptr<TH2F> mHistogramDigitsOrbitInTFDE;

  std::shared_ptr<TH2F> mHistogramDigitsOrbitInTFDEPrevCycle;
  std::shared_ptr<TH2F> mHistogramDigitsOrbitInTFDEOnCycle;
  std::shared_ptr<TH2F> mHistogramDigitsBcInOrbit;
  std::shared_ptr<TH2F> mHistogramAmplitudeVsSamples;

  std::map<int, std::shared_ptr<TH1F>> mHistogramADCamplitudeDE;              // Histogram of ADC distribution per DE
  std::map<int, std::shared_ptr<DetectorHistogram>> mHistogramNhitsDE[2];     // Histogram of Number of hits (XY view)
  std::map<int, std::shared_ptr<DetectorHistogram>> mHistogramNorbitsDE[2];   // Histogram of Number of orbits (XY view)
  std::map<int, std::shared_ptr<MergeableTH2Ratio>> mHistogramOccupancyDE[2]; // Mergeable object, Occupancy histogram (XY view)

  // TH1 of the Mean Occupancy on each DE
  std::shared_ptr<TH1F> mHistogramMeanOccupancyPerDE;
  std::shared_ptr<TH1F> mHistogramMeanOccupancyOnCyclePerDE;

  std::vector<TH1*> mAllHistograms;
};

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_MUONCHAMBERS_PHYSICSDATAPROCESSOR_H
