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
/// \file   MIPCheck.h
/// \author andreas.molander@cern.ch
///

#ifndef QC_MODULE_FIT_FITMIPCHECK_H
#define QC_MODULE_FIT_FITMIPCHECK_H

#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::fit
{

/// \brief  QC check on MIP peaks in channel amplitude spectra. Essentially it is a check on
///         parameters of Gaussian fits of the MIP peaks.
/// \author andreas.molander@cern.ch
class MIPCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  MIPCheck() = default;
  ~MIPCheck() override = default;

  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  void startOfActivity(const Activity& activity) override;

 private:
  std::shared_ptr<Activity> mActivity;

  std::string mNameObjectToCheck = ""; //< Name of the MO to check

  /// Number of MIP peaks to fit. The resulting fit funcion is a sum of `mNPeaksToFit` Gaussians.
  int mNPeaksToFit = 2;

  /// Initial fit paramemters for the gaussian means.
  /// If omitted, the 1 MIP peak will default to the amplitude at the ampltude histogram max value.
  /// Other peaks  will default to multiples of the initial 1 MIP peak mean (2 MIP peak mean = 2 * 1 MIP peak mean, and so on).
  /// The values are used as initial guesses and are not fixed.
  std::vector<float> mGausParamsMeans;

  /// The initial fit parameters for the MIP peak sigmas. The values are used as initial guesses and are not fixed.
  std::vector<float> mGausParamsSigmas;

  float mFitRangeLow = 11.0; //< Lower limit of the fit

  float mFitRangeHigh = 35.0; //< Upper limit of the fit

  /// Lower warning thresholds for the MIP peak means.
  /// The first element is for the first peak and so on.
  std::vector<float> mMeanWarningsLow;

  /// Upper warning thresholds for the MIP peak means.
  /// The first element is for the first peak and so on.
  std::vector<float> mMeanWarningsHigh;

  /// Lower error thresholds for the MIP peak means.
  /// The first element is for the first peak and so on.
  std::vector<float> mMeanErrorsLow;

  /// Upper error thresholds for the MIP peak means.
  /// The first element is for the first peak and so on.
  std::vector<float> mMeanErrorsHigh;

  /// Sigma warning thresholds.
  /// The first element is for the first peak and so on.
  std::vector<float> mSigmaWarnings;

  /// Sigma error thresholds.
  /// The first element is for the first peak and so on.
  std::vector<float> mSigmaErrors;

  /// Whether to draw the threhold lines.
  /// The first element is for the first peak and so on.
  std::vector<bool> mDrawMeanWarningsLow;
  std::vector<bool> mDrawMeanWarningsHigh;
  std::vector<bool> mDrawMeanErrorsLow;
  std::vector<bool> mDrawMeanErrorsHigh;
  std::vector<bool> mDrawSigmaWarnings;
  std::vector<bool> mDrawSigmaErrors;

  std::vector<double> mVecLabelPos{ 0.15, 0.2, 0.85, 0.45 }; //< Position of the check label

  ClassDefOverride(MIPCheck, 3);
};

} // namespace o2::quality_control_modules::fit

#endif // QC_MODULE_FIT_FITMIPCHECK_H
