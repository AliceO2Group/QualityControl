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
/// \file   JunkDetection.h
/// \author Thomas Klemenz
///

#ifndef QC_MODULE_TPC_JUNKDETECTION_H
#define QC_MODULE_TPC_JUNKDETECTION_H

// QC includes
#include "QualityControl/TaskInterface.h"

// root includes
#include "TH2.h"

class TCanvas;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::tpc
{

/// \brief TPC JunkDetection QC Task
/// \author Thomas Klemenz
class JunkDetection final : public TaskInterface
{
 public:
  /// \brief Constructor
  JunkDetection() = default;
  /// Destructor
  ~JunkDetection()
  {
    for (auto hist : mJDHistos) {
      delete hist;
    }
  }

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;
  /// fills the output canvas with the relevant info
  TCanvas* makeCanvas(const TObjArray* data, TCanvas* outputCanvas);

 private:
  bool mIsMergeable = false;
  std::unique_ptr<TCanvas> mJDCanv;
  std::vector<TH2F*> mJDHistos{};
};

} // namespace o2::quality_control_modules::tpc

#endif // QC_MODULE_TPC_JUNKDETECTION_H
