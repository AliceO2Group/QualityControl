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
/// \file   RawDataQcTask.h
/// \author Marek Bombara
/// \author Lucia Anna Tarasovicova
/// \author Jan Musinsky
/// \date   2026-02-17
///

#ifndef QC_MODULE_CTP_CTPRAWDATAQCTASK_H
#define QC_MODULE_CTP_CTPRAWDATAQCTASK_H

#include "QualityControl/TaskInterface.h"
#include "CTPReconstruction/RawDataDecoder.h"
#include "Common/TH1Ratio.h"
#include <memory>

class TH1D;

using namespace o2::quality_control::core;
using namespace o2::quality_control_modules::common;
namespace o2::quality_control_modules::ctp
{

/// \brief Task for reading the CTP inputs and CTP classes
class CTPRawDataReaderTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  CTPRawDataReaderTask() = default;
  /// Destructor
  ~CTPRawDataReaderTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;
  void splitSortInputs();
  void readLHCFillingScheme();

 private:
  o2::ctp::RawDataDecoder mDecoder;                              // ctp raw data decoder
  std::unique_ptr<TH1DRatio> mHistoInputs = nullptr;             // histogram with ctp inputs
  std::unique_ptr<TH1DRatio> mHistoClasses = nullptr;            // histogram with ctp classes
  std::unique_ptr<TH1DRatio> mHistoInputRatios = nullptr;        // histogram with ctp input ratios to MB
  std::unique_ptr<TH1DRatio> mHistoClassRatios = nullptr;        // histogram with ctp class ratios to MB
  std::unique_ptr<TH1D> mHistoBCMinBias1 = nullptr;              // histogram of BC positions to check LHC filling scheme
  std::unique_ptr<TH1D> mHistoBCMinBias2 = nullptr;              // histogram of BC positions to check LHC filling scheme
  std::unique_ptr<TH1D> mHistoDecodeError = nullptr;             // histogram of erros from decoder
  std::array<TH1D*, o2::ctp::CTP_NINPUTS> mHisInputs = {};       ///< Array of input histograms, all BCs
  std::array<TH1D*, o2::ctp::CTP_NINPUTS> mHisInputsYesLHC = {}; ///< Array of input histograms, LHC BCs
  std::array<TH1D*, o2::ctp::CTP_NINPUTS> mHisInputsNotLHC = {}; ///< Array of input histograms, not LHC BCs
  std::array<std::size_t, o2::ctp::CTP_NINPUTS> shiftBC = {};    ///< Array of shifts for the BCs for each input
  int mRunNumber;
  int indexMB1 = -1;
  int indexMB2 = -1;
  int mShiftInput1 = 0;
  int mShiftInput2 = 0;
  static const int ninps = o2::ctp::CTP_NINPUTS + 1;
  static const int nclasses = o2::ctp::CTP_NCLASSES + 1;
  double mScaleInput1 = 1;
  double mScaleInput2 = 1;
  long int mTimestamp;
  std::string classNames[nclasses];
  int mIndexMBclass = -1; // index for the MB ctp class, which is used as scaling for the ratios
  bool mConsistCheck = 0;
  bool mReadCTPconfigInMonitorData = 0;
  const o2::ctp::CTPConfiguration* mCTPconfig = nullptr;
  std::string mMBclassName;
  std::array<uint64_t, o2::ctp::CTP_NCLASSES> mClassErrorsA;
  bool mPerformConsistencyCheck = false;
  std::bitset<o2::constants::lhc::LHCMaxBunches> mLHCBCs; /// LHC filling scheme
  bool lhcDataFileFound = true;
};

} // namespace o2::quality_control_modules::ctp

#endif // QC_MODULE_CTP_CTPRAWDATAQCTASK_H
