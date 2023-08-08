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
/// \file   CountersQcTask.h
/// \author Marek Bombara
///

#ifndef QC_MODULE_CTP_CTPCOUNTERSQCTASK_H
#define QC_MODULE_CTP_CTPCOUNTERSQCTASK_H

#include "QualityControl/TaskInterface.h"
#include "DataFormatsCTP/Configuration.h"
#include "DataFormatsCTP/RunManager.h"
#include "TH1.h"

class TH1F;
class TH1D;
class TCanvas;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::ctp
{

struct runCTP2QC {
  int mRunNumber;
  std::vector<int> mRunClasses;
  int mPositionInCounters;
};

class CTPCountersTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  CTPCountersTask() = default;
  /// Destructor
  ~CTPCountersTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

  // setters
  void SetIsFirstCycle(bool isFirstCycle = true) { mIsFirstCycle = isFirstCycle; }
  void SetFirstTimeStamp(double firstTimeStamp = 0) { mFirstTimeStamp = firstTimeStamp; }
  void SetPreviousTimeStamp(double previousTimeStamp = 0) { mPreviousTimeStamp = previousTimeStamp; }
  void SetRateHisto(TH1D* h, double ofs)
  {
    h->GetXaxis()->SetTimeDisplay(1);
    h->GetXaxis()->SetTimeOffset(ofs);
    h->GetXaxis()->SetTimeFormat("%H:%M");
    h->GetXaxis()->SetNdivisions(808);
  };

  // getters
  bool IsFirstCycle() { return mIsFirstCycle; }
  double GetFirstTimeStamp() { return mFirstTimeStamp; }
  double GetPreviousTimeStamp() { return mPreviousTimeStamp; }

 private:
  bool mIsFirstCycle = true;
  double mFirstTimeStamp = 0;
  double mPreviousTimeStamp = 0;
  std::vector<double> mTime;
  std::vector<double> mPreviousTrgInput;
  std::vector<double> mPreviousTrgClass;
  std::vector<int> mPreviousRunNumbers;
  runCTP2QC mNewRun = { 0 };
  std::vector<double> mTimes[48];
  std::vector<double> mClassTimes[64];
  std::vector<double> mInputRates[48];
  std::vector<double> mClassRates[64];
  TH1D* mInputCountsHist = nullptr;
  TH1D* mDummyCountsHist = nullptr;
  TH1D* mInputRateHist = nullptr;
  TH1D* mClassCountsHist = nullptr;
  TCanvas* mTCanvasInputs = nullptr;
  std::array<TH1D*, 48> mHistInputRate = { nullptr };
  TCanvas* mTCanvasClasses = nullptr;
  std::array<TH1D*, 64> mHistClassRate = { nullptr };
  TCanvas* mTCanvasTotalCountsClasses = nullptr;
  std::array<TH1D*, 6> mHistClassTotalCounts = { nullptr };
  std::array<TCanvas*, 16> mTCanvasClassRates = { nullptr };
};

class CTPQcRunManager : public o2::ctp::CTPRunManager
{
 public:
  /// \brief Constructor
  CTPQcRunManager() = default;
  /// Destructor
  ~CTPQcRunManager(){};
};

} // namespace o2::quality_control_modules::ctp

#endif // QC_MODULE_CTP_CTPCOUNTERSQCTASK_H
