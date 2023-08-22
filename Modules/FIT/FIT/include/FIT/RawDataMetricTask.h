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
/// \file   RawDataMetricTask.h
/// \author Artur Furs afurs@cern.ch
/// QC task for RawDataMetric QC proccessing at FIT detectors

#ifndef QC_MODULE_FIT_RAWDATAMETRICTASK_H
#define QC_MODULE_FIT_RAWDATAMETRICTASK_H

#include <memory>
#include <set>

#include <FITCommon/HelperLUT.h>

#include "Headers/DataHeader.h"
#include "QualityControl/TaskInterface.h"
#include <TH1.h>
#include <TH2.h>
using namespace o2::quality_control::core;

namespace o2::quality_control_modules::fit
{

class RawDataMetricTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  RawDataMetricTask() = default;
  /// Destructor
  ~RawDataMetricTask() override;
  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  // Objects which will be published
  std::string mDetName{ "" };  // detector name
  unsigned int mBinPosUnknown; // position for uknown FEE;
  std::unique_ptr<TH2F> mHistRawDataMetrics;
  std::map<helperLUT::FEEID_t, unsigned int> mMapFEE2binPos{};     //(epID,linkID)->bin position, alphabet sorted by module name
  static const std::set<o2::header::DataOrigin> sSetOfAllowedDets; // set of allowed detectors: FDD, FT, FV0
};

} // namespace o2::quality_control_modules::fit

#endif // QC_MODULE_FIT_RAWDATAMETRICTASK_H
