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
/// \file   CalibQcTask.h
/// \author Dmitri Peresunko
/// \author Dmitry Blau
///

#ifndef QC_MODULE_PHOS_QCCALIBTASK_H
#define QC_MODULE_PHOS_QCCALIBTASK_H

#include "QualityControl/TaskInterface.h"
#include <array>

class TH2F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::phos
{

/// \brief PHOS Quality Control DPL Task for calibration monitoring
class CalibQcTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  CalibQcTask() = default;
  /// Destructor
  ~CalibQcTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 protected:
 private:
  int mMode = 0; ///< Mode of operation: 0: badMap, 1: Pedestals, 2: LED
  static constexpr short NHIST2D = 9;
  enum histos2D { kChangeHGM1,
                  kChangeHGM2,
                  kChangeHGM3,
                  kChangeHGM4,
                  kChangeLGM1,
                  kChangeLGM2,
                  kChangeLGM3,
                  kChangeLGM4,
                  kL1phase
  };
  std::array<TH2F*, NHIST2D> mHist2D = { nullptr }; ///< Array of 2D histograms
};

} // namespace o2::quality_control_modules::phos

#endif // QC_MODULE_PHOS_QCCALIBTASK_H
