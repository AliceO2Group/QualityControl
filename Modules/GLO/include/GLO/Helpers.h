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

#ifndef QC_MODULE_GLO_HELPERS_H
#define QC_MODULE_GLO_HELPERS_H

#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/CustomParameters.h"
#include "QualityControl/Activity.h"
#include "Common/Utils.h"

#include <CommonConstants/PhysicsConstants.h>

#include <memory>

#include <TF1.h>
#include <TH1.h>
#include <TFitResult.h>
#include <TFitResultPtr.h>

namespace o2::quality_control_modules::glo::helpers
{

// Simple class to fit K0s mass
struct K0sFitter {
  enum Parameters : uint8_t {
    // Background
    Pol0 = 0,
    Pol1 = 1,
    Pol2 = 2,
    // K0s
    Amplitude = 3,
    Mass = 4,
    Sigma = 5,
  };
  static constexpr auto mMassK0s{ o2::constants::physics::MassK0Short };
  double mBackgroundRangeLeft{ 0.45 };
  double mBackgroundRangeRight{ 0.54 };
  struct FitBackground {
    static constexpr int mNPar{ 3 };
    double mRejLeft{ 0.48 };
    double mRejRight{ 0.51 };
    double operator()(double* x, double* p) const
    {
      if (x[0] > mRejLeft && x[0] < mRejRight) {
        TF1::RejectPoint();
        return 0;
      }
      return p[0] + (p[1] * x[0]) + (p[2] * x[0] * x[0]);
    }
  } mFitBackground;
  std::unique_ptr<TF1> mBackground;
  std::unique_ptr<TF1> mSignalAndBackground;

  void init(o2::quality_control::core::CustomParameters const& pars)
  {
    mFitBackground.mRejLeft = common::getFromConfig(pars, "k0sBackgroundRejLeft", 0.48);
    mFitBackground.mRejRight = common::getFromConfig(pars, "k0sBackgroundRejRight", 0.51);
    mBackgroundRangeLeft = common::getFromConfig(pars, "k0sBackgroundRangeLeft", 0.45);
    mBackgroundRangeRight = common::getFromConfig(pars, "k0sBackgroundRangeRight", 0.54);
    initImpl();
  }

  void init(o2::quality_control::core::CustomParameters const& pars, o2::quality_control::core::Activity const& activity)
  {
    mFitBackground.mRejLeft = common::getFromExtendedConfig(activity, pars, "k0sBackgroundRejLeft", 0.48);
    mFitBackground.mRejRight = common::getFromExtendedConfig(activity, pars, "k0sBackgroundRejRight", 0.51);
    mBackgroundRangeLeft = common::getFromExtendedConfig(activity, pars, "k0sBackgroundRangeLeft", 0.45);
    mBackgroundRangeRight = common::getFromExtendedConfig(activity, pars, "k0sBackgroundRangeRight", 0.54);
    initImpl();
  }

  void initImpl()
  {
    mBackground.reset(new TF1("gloFitK0sMassBackground", mFitBackground, mBackgroundRangeLeft, mBackgroundRangeRight, mFitBackground.mNPar));
    mSignalAndBackground.reset(new TF1("gloFitK0sMassSignal", "[0] + [1] * x + [2] * x * x + gaus(3)", mBackgroundRangeLeft, mBackgroundRangeRight));
  }

  bool fit(TH1* h, bool add = false)
  {
    if (!h || h->GetEntries() == 0) {
      ILOG(Warning, Devel) << "Cannot fit empty histogram: "
                           << (h ? h->GetName() : "<null>") << ENDM;
      return false;
    }

    // --- First: background-only fit
    auto bgResult = h->Fit(mBackground.get(), "RNQS");
    if (bgResult.Get() == nullptr || bgResult->Status() != 0) {
      ILOG(Warning, Devel) << "Failed k0s background fit for histogram: "
                           << h->GetName()
                           << " (status=" << (bgResult ? bgResult->Status() : -1) << ")"
                           << ENDM;
      return false;
    }

    // --- Initialize signal+background from background fit
    mSignalAndBackground->SetParameter(Parameters::Pol0, mBackground->GetParameter(Parameters::Pol0));
    mSignalAndBackground->SetParameter(Parameters::Pol1, mBackground->GetParameter(Parameters::Pol1));
    mSignalAndBackground->SetParameter(Parameters::Pol2, mBackground->GetParameter(Parameters::Pol2));
    mSignalAndBackground->SetParameter(Parameters::Amplitude, h->GetMaximum() - mBackground->Eval(mMassK0s));
    mSignalAndBackground->SetParameter(Parameters::Mass, mMassK0s);
    mSignalAndBackground->SetParameter(Parameters::Sigma, 0.005);
    mSignalAndBackground->SetParLimits(Parameters::Sigma, 1e-6, 1.0);

    // --- Fit signal+background
    const std::string fitOpt = add ? "RMQS" : "RMQS0";
    auto sbResult = h->Fit(mSignalAndBackground.get(), fitOpt.c_str());
    if (sbResult.Get() == nullptr || sbResult->Status() != 0) {
      ILOG(Warning, Devel) << "Failed k0s signal+background fit for histogram: "
                           << h->GetName()
                           << " (status=" << (sbResult ? sbResult->Status() : -1) << ")"
                           << ENDM;
      return false;
    }
    return true;
  }

  auto getMass() const noexcept
  {
    return mSignalAndBackground->GetParameter(4);
  }

  auto getSigma() const noexcept
  {
    return mSignalAndBackground->GetParameter(5);
  }

  auto getUncertainty() const noexcept
  {
    return std::abs(mMassK0s - getMass()) / getSigma();
  }

  auto getRelativeError() const noexcept
  {
    return std::abs(mMassK0s - getMass()) / mMassK0s;
  }
};

} // namespace o2::quality_control_modules::glo::helpers

#endif
