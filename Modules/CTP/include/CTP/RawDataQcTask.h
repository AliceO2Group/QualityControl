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

 private:
  o2::ctp::RawDataDecoder mDecoder;                       // ctp raw data decoder
  std::unique_ptr<TH1DRatio> mHistoInputs = nullptr;      // histogram with ctp inputs
  std::unique_ptr<TH1DRatio> mHistoClasses = nullptr;     // histogram with ctp classes
  std::unique_ptr<TH1DRatio> mHistoInputRatios = nullptr; // histogram with ctp input ratios to MB
  std::unique_ptr<TH1DRatio> mHistoClassRatios = nullptr; // histogram with ctp class ratios to MB
  std::unique_ptr<TH1D> mHistoBCMinBias1 = nullptr;       // histogram of BC positions to check LHC filling scheme
  std::unique_ptr<TH1D> mHistoBCMinBias2 = nullptr;       // histogram of BC positions to check LHC filling scheme
  int mRunNumber;
  int indexMB1 = -1;
  int indexMB2 = -1;
  static const int ninps = o2::ctp::CTP_NINPUTS + 1;
  static const int nclasses = o2::ctp::CTP_NCLASSES + 1;
  long int mTimestamp;
  std::string classNames[nclasses];
  int mIndexMBclass = -1; // index for the MB ctp class, which is used as scaling for the ratios
};

} // namespace o2::quality_control_modules::ctp

#endif // QC_MODULE_CTP_CTPRAWDATAQCTASK_H
