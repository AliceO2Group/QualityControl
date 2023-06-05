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
/// \file   PhysicsTask.h
/// \author Sergey Evdokimov
///

#ifndef QC_MODULE_CPV_CPVPHYSICSTASK_H
#define QC_MODULE_CPV_CPVPHYSICSTASK_H

#include "QualityControl/TaskInterface.h"
#include "PHOS/TH1Fraction.h"
#include "PHOS/TH2Fraction.h"
#include <Mergers/MergeInterface.h>
#include <memory>
#include <array>
#include <gsl/span>
#include <CPVBase/Geometry.h>
#include <TH2F.h>

class TH1F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::cpv
{

// this is 2D histogram which is not additive: when merge() is being called, it is just updated with new incoming value rather than merging
// update or not is decided by cycle counter: if this->mCycleNumber < incoming.mCycleNumber then *this = incoming
class IntensiveTH2F : public TH2F, public o2::mergers::MergeInterface
{
 public:
  /// \brief Constructor.
  IntensiveTH2F(const char* name, const char* title, int nbinsx, double xlow, double xup, int nbinsy, double ylow, double yup)
    : TH2F(name, title, nbinsx, xlow, xup, nbinsy, ylow, yup)
  {
  }
  IntensiveTH2F() = default;
  /// \brief Default destructor
  ~IntensiveTH2F() override = default;

  const char* GetName() const override
  {
    return TH2F::GetName();
  }

  void setCycleNumber(uint32_t cycleNumber) { mCycleNumber = cycleNumber; }
  uint32_t getCylceNumber() { return mCycleNumber; }

  void merge(MergeInterface* const other) override
  {
    if (IntensiveTH2F* cast = dynamic_cast<IntensiveTH2F*>(other); cast != nullptr) {
      if (mCycleNumber < cast->mCycleNumber) {
        mCycleNumber = cast->mCycleNumber;
        for (int iX = 1; iX <= GetNbinsX(); iX++) {
          for (int iY = 1; iY <= GetNbinsY(); iY++) {
            SetBinContent(iX, iY, cast->GetBinContent(iX, iY));
          }
        }
      }
    } else {
      LOG(warn) << "IntensiveTH2F::merge() : problem occured while dynamic_cast<IntensiveTH2F*>(other) for "
                << GetName() << ". Result is nullptr. Do nothing this time.";
    }
  }

 private:
  std::string mTreatMeAs = "TH2F"; // the name of the class this object should be considered as when drawing in QCG.
  uint32_t mCycleNumber = 0;       // cycle number of last udate

  ClassDefOverride(IntensiveTH2F, 1);
};

/// \brief Task for CPV Physics monitoring
/// \author Sergey Evdokimov
using Geometry = o2::cpv::Geometry;
using TH1Fraction = o2::quality_control_modules::phos::TH1Fraction;
using TH2Fraction = o2::quality_control_modules::phos::TH2Fraction;
class PhysicsTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  PhysicsTask() = default;
  /// Destructor
  ~PhysicsTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  void initHistograms();
  void resetHistograms();

  static constexpr short kNHist1D = 29;
  enum Histos1D { H1DInputPayloadSize,
                  H1DNInputs,
                  H1DNValidInputs,
                  H1DRawErrors,
                  H1DNDigitsPerInput,
                  H1DNClustersPerInput,
                  H1DNCalibDigitsPerInput,
                  H1DDigitIds,
                  H1DCalibDigitIds,
                  H1DDigitsInEventM2,
                  H1DDigitsInEventM3,
                  H1DDigitsInEventM4,
                  H1DDigitsInEventM2M3M4,
                  H1DDigitEnergyM2,
                  H1DDigitEnergyM3,
                  H1DDigitEnergyM4,
                  H1DCalibDigitEnergyM2,
                  H1DCalibDigitEnergyM3,
                  H1DCalibDigitEnergyM4,
                  H1DClustersInEventM2,
                  H1DClustersInEventM3,
                  H1DClustersInEventM4,
                  H1DClustersInEventM2M3M4,
                  H1DClusterTotEnergyM2,
                  H1DClusterTotEnergyM3,
                  H1DClusterTotEnergyM4,
                  H1DNDigitsInClusterM2,
                  H1DNDigitsInClusterM3,
                  H1DNDigitsInClusterM4
  };

  static constexpr short kNHist2D = 9;
  enum Histos2D { H2DDigitMapM2,
                  H2DDigitMapM3,
                  H2DDigitMapM4,
                  H2DCalibDigitMapM2,
                  H2DCalibDigitMapM3,
                  H2DCalibDigitMapM4,
                  H2DClusterMapM2,
                  H2DClusterMapM3,
                  H2DClusterMapM4
  };
  static constexpr short kNIntensiveHist2D = 12;
  enum IntensiveHistos2D { H2DPedestalValueM2,
                           H2DPedestalValueM3,
                           H2DPedestalValueM4,
                           H2DPedestalSigmaM2,
                           H2DPedestalSigmaM3,
                           H2DPedestalSigmaM4,
                           H2DBadChannelMapM2,
                           H2DBadChannelMapM3,
                           H2DBadChannelMapM4,
                           H2DGainsM2,
                           H2DGainsM3,
                           H2DGainsM4
  };
  static constexpr short kNfractions1D = 1;
  enum fractions1D { F1DErrorTypeOccurance };

  static constexpr short kNfractions2D = 3;
  enum fractions2D { F2DDigitFreqM2,
                     F2DDigitFreqM3,
                     F2DDigitFreqM4
  };

  static constexpr short kNModules = 3;
  static constexpr short kNChannels = 23040;
  int mNEventsTotal = 0;
  int mCcdbCheckIntervalInMinutes = 1;
  uint32_t mCycleNumber = 0;
  bool mJustWasReset = false;
  bool mIsAsyncMode = false;
  std::array<TH1F*, kNHist1D> mHist1D = { nullptr };                            ///< Array of 1D histograms
  std::array<TH2F*, kNHist2D> mHist2D = { nullptr };                            ///< Array of 2D histograms
  std::array<IntensiveTH2F*, kNIntensiveHist2D> mIntensiveHist2D = { nullptr }; ///< Array of IntensiveTH2F histograms
  std::array<TH1Fraction*, kNfractions1D> mFractions1D = { nullptr };           ///< Array of TH1Fraction histograms
  std::array<TH2Fraction*, kNfractions2D> mFractions2D = { nullptr };           ///< Array of TH1Fraction histograms
};

} // namespace o2::quality_control_modules::cpv

#endif // QC_MODULE_CPV_CPVPHYSICSTASK_H
