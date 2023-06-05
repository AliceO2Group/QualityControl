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
/// \file       HmpidTask.h
/// \author     Antonio Franco, Giacomo Volpe, Antonio Paz
/// \brief      Class for quality control of HMPID detectors
/// \version    0.2.4
/// \date       19/05/2022
///

#ifndef QC_MODULE_HMPID_HMPIDHMPIDTASK_H
#define QC_MODULE_HMPID_HMPIDHMPIDTASK_H

#include "QualityControl/TaskInterface.h"
#include "HMPIDReconstruction/HmpidDecoder2.h"

class TH1F;
class TH2F;
class TProfile;
class TProfile2D;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::hmpid
{

/// \brief Example Quality Control DPL Task
/// \author My Name
class HmpidTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  HmpidTask() = default;
  /// Destructor
  ~HmpidTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  static const Int_t numCham = 7;
  TH1F* hPedestalMean = nullptr;
  TH1F* hPedestalSigma = nullptr;
  TProfile* hBusyTime = nullptr;
  TProfile* hEventSize = nullptr;
  TProfile* hEventNumber = nullptr;
  TH2F* hModuleMap[numCham] = { nullptr };
  o2::hmpid::HmpidDecoder2* mDecoder = nullptr;
  // TH2F *fHmpBigMap = nullptr;
  TH2F* fHmpHvSectorQ = nullptr;
  TProfile2D* fHmpBigMap_profile = nullptr;
  // TProfile2D *fHmpHvSectorQ_profile = nullptr;
  // TH2F *fHmpHvSectorQ_profile_temp = nullptr;
  TProfile* fHmpPadOccPrf = nullptr;
};

} // namespace o2::quality_control_modules::hmpid

#endif // QC_MODULE_HMPID_HMPIDHMPIDTASK_H
