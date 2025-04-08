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
/// \file   DigitsTask.h
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_MUONCHAMBERS_DIGITSTASK_H
#define QC_MODULE_MUONCHAMBERS_DIGITSTASK_H

#include "QualityControl/TaskInterface.h"
#ifdef HAVE_DIGIT_IN_DATAFORMATS
#include "DataFormatsMCH/Digit.h"
#else
#include "MCHBase/Digit.h"
#endif
#include "MCHDigitFiltering/DigitFilter.h"
#include "Common/TH1Ratio.h"
#include "Common/TH2Ratio.h"

class TH1F;
class TH2F;

using namespace o2::quality_control::core;
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
class DigitsTask /*final*/ : public TaskInterface // todo add back the "final" when doxygen is fixed
{
 public:
  /// \brief Constructor
  DigitsTask() = default;
  /// Destructor
  ~DigitsTask() override = default;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  void plotDigit(const o2::mch::Digit& digit);
  void updateOrbits();
  void resetOrbits();

  template <typename T>
  void publishObject(T* histo, std::string drawOption, bool statBox, bool isExpert);

  bool mEnable1DRateMaps{ true };  // whether to publish 1D maps of channel rates
  bool mEnable2DRateMaps{ false }; // whether to publish 2D maps of channel rates

  bool mFullHistos{ false }; // publish extra diagnostics plots

  o2::mch::DigitFilter mIsSignalDigit;

  uint32_t mNOrbits{ 0 };

  // 2D Histograms, using Elec view (where x and y uniquely identify each pad based on its Elec info (fee, link, de)
  std::unique_ptr<TH2FRatio> mHistogramOccupancyElec;       // Occupancy histogram (Elec view)
  std::unique_ptr<TH2FRatio> mHistogramSignalOccupancyElec; // Occupancy histogram (Elec view) for signal-like digits

  // 1D rate histograms using Elec view, where each x bin corresponds to the unique ID of a DualSAMPA board
  std::unique_ptr<TH1DRatio> mHistogramRatePerDualSampa;
  std::unique_ptr<TH1DRatio> mHistogramRateSignalPerDualSampa;

  std::unique_ptr<TH2F> mHistogramDigitsOrbitElec;
  std::unique_ptr<TH2F> mHistogramDigitsSignalOrbitElec;

  std::unique_ptr<TH2F> mHistogramDigitsBcInOrbit;
  std::unique_ptr<TH2F> mHistogramAmplitudeVsSamples;

  std::map<int, std::unique_ptr<TH1F>> mHistogramADCamplitudeDE; // Histogram of ADC distribution per DE

  std::vector<TH1*> mAllHistograms;
};

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_MUONCHAMBERS_DIGITSTASK_H
