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
/// \author Barthelemy von Haller
/// \author Piotr Konopka
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_MUONCHAMBERS_DIGITSTASK_H
#define QC_MODULE_MUONCHAMBERS_DIGITSTASK_H

#include "QualityControl/TaskInterface.h"
#include "MCHRawElecMap/Mapper.h"
#ifdef HAVE_DIGIT_IN_DATAFORMATS
#include "DataFormatsMCH/Digit.h"
#else
#include "MCHBase/Digit.h"
#endif
#include "MCHDigitFiltering/DigitFilter.h"
#include "MUONCommon/MergeableTH2Ratio.h"
#include "MCH/Helpers.h"

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
class DigitsTask /*final*/ : public TaskInterface // todo add back the "final" when doxygen is fixed
{
 public:
  /// \brief Constructor
  DigitsTask();
  /// Destructor
  ~DigitsTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  void storeOrbit(const uint64_t& orb);
  void addDefaultOrbitsInTF();
  void plotDigit(const o2::mch::Digit& digit);
  void updateOrbits();
  void resetOrbits();

  template <typename T>
  void publishObject(T* histo, std::string drawOption, bool statBox, bool isExpert)
  {
    histo->SetOption(drawOption.c_str());
    if (!statBox) {
      histo->SetStats(0);
    }
    mAllHistograms.push_back(histo);
    if (mDiagnostic || (isExpert == false)) {
      getObjectsManager()->startPublishing(histo);
      getObjectsManager()->setDefaultDrawOptions(histo, drawOption);
    }
  }

  bool mDiagnostic{ false }; // publish extra diagnostics plots

  o2::mch::raw::Elec2DetMapper mElec2DetMapper;
  o2::mch::raw::FeeLink2SolarMapper mFeeLink2SolarMapper;

  o2::mch::DigitFilter mIsSignalDigit;

  uint32_t mNOrbits[FecId::sFeeNum][FecId::sLinkNum];
  uint32_t mLastOrbitSeen[FecId::sFeeNum][FecId::sLinkNum];
  int mNOrbitsPerTF{ -1 };

  // 2D Histograms, using Elec view (where x and y uniquely identify each pad based on its Elec info (fee, link, de)
  std::unique_ptr<MergeableTH2Ratio> mHistogramOccupancyElec;       // Occupancy histogram (Elec view)
  std::unique_ptr<MergeableTH2Ratio> mHistogramSignalOccupancyElec; // Occupancy histogram (Elec view) for signal-like digits

  // TH2F* mHistNum;
  // TH2F* mHistDen;

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
