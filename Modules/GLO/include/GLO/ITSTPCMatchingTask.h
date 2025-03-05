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
/// \file   ITSTPCMatchingTask.h
/// \author Chiara Zampolli
///

#ifndef QC_MODULE_GLO_ITSTPCMATCHINGTASK_H
#define QC_MODULE_GLO_ITSTPCMATCHINGTASK_H

#include "QualityControl/TaskInterface.h"
#include "GLOQC/MatchITSTPCQC.h"
#include "Common/TH1Ratio.h"

#include <CommonConstants/PhysicsConstants.h>

#include <memory>

#include <TH3F.h>
#include <TF1.h>
#include <TGraphErrors.h>

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::glo
{

/// \brief ITS-TPC Matching QC task
/// \author Chiara Zampolli
class ITSTPCMatchingTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  ITSTPCMatchingTask() = default;
  /// Destructor
  ~ITSTPCMatchingTask() override = default;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  o2::gloqc::MatchITSTPCQC mMatchITSTPCQC;

  unsigned int mNCycle{ 0 };

  bool mIsSync{ false };
  bool mIsPbPb{ false };

  bool mDoMTCRatios{ false };
  std::unique_ptr<common::TH1FRatio> mEffPt;
  std::unique_ptr<common::TH1FRatio> mEffEta;
  std::unique_ptr<common::TH1FRatio> mEffPhi;

  bool mDoPtTrending{ false };
  float mTrendingBinPt{ 1.0 };
  std::unique_ptr<TGraph> mTrendingPt;

  bool mDoK0s{ false };
  static constexpr auto mMassK0s{ o2::constants::physics::MassK0Short };
  std::unique_ptr<TH3F> mK0sCycle;
  std::unique_ptr<TH3F> mK0sIntegral;
  bool mDoK0sMassTrending{ false };
  std::unique_ptr<TGraphErrors> mK0sMassTrend;
  bool fitK0sMass(TH1* h);
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
};

} // namespace o2::quality_control_modules::glo

#endif // QC_MODULE_GLO_ITSTPCMATCHINGTASK_H
