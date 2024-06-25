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

/// \brief Task for reading the CTP inputs
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
  o2::ctp::RawDataDecoder mDecoder;
  std::unique_ptr<TH1DRatio> mHistoInputs = nullptr;
  std::unique_ptr<TH1DRatio> mHistoClasses = nullptr;
  std::unique_ptr<TH1DRatio> mHistoInputRatios = nullptr;
  std::unique_ptr<TH1DRatio> mHistoClassRatios = nullptr;
  std::unique_ptr<TH1D> mHistoMTVXBC = nullptr;
  int mRunNumber;
  long int mTimestamp;
  int mIndexMBclass = -1;
  // const char* ctpinputs[o2::ctp::CTP_NINPUTS + 1] = { " T0A", " T0C", " TVX", " TSC", " TCE", " VBA", " VOR", " VIR", " VNC", " VCH", "11", "12", " UCE", "DMC", " USC", " UVX", " U0C", " U0A", "COS", "LAS", "EMC", " PH0", "23", "24", "ZED", "ZNC", "PHL", "PHH", "PHM", "30", "31", "32", "33", "34", "35", "36", "EJ1", "EJ2", "EG1", "EG2", "DJ1", "DG1", "DJ2", "DG2", "45", "46", "47", "48", "49" };
};

} // namespace o2::quality_control_modules::ctp

#endif // QC_MODULE_CTP_CTPRAWDATAQCTASK_H
