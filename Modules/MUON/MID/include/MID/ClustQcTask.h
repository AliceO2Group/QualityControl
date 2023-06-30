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
/// \file   ClustQcTask.h
/// \author Valerie Ramillien

#ifndef QC_MODULE_MID_MIDCLUSTQCTASK_H
#define QC_MODULE_MID_MIDCLUSTQCTASK_H

#include "QualityControl/TaskInterface.h"
#include "QualityControl/QcInfoLogger.h"
#include "MIDBase/Mapping.h"

class TH1F;
class TH2F;
class TProfile;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::mid
{

/// \brief Count number of digits per detector elements

class ClustQcTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  ClustQcTask() = default;
  /// Destructor
  ~ClustQcTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  std::shared_ptr<TH1F> mNbClusterTF{ nullptr };

  std::shared_ptr<TH2F> mClusterMap11{ nullptr };
  std::shared_ptr<TH2F> mClusterMap12{ nullptr };
  std::shared_ptr<TH2F> mClusterMap21{ nullptr };
  std::shared_ptr<TH2F> mClusterMap22{ nullptr };

  std::shared_ptr<TH1F> mMultClust11{ nullptr };
  std::shared_ptr<TH1F> mMultClust12{ nullptr };
  std::shared_ptr<TH1F> mMultClust21{ nullptr };
  std::shared_ptr<TH1F> mMultClust22{ nullptr };

  std::shared_ptr<TProfile> mClustBCCounts{ nullptr };

  std::shared_ptr<TH1F> mClustResX{ nullptr };
  std::shared_ptr<TH1F> mClustResY{ nullptr };
  std::shared_ptr<TH2F> mClustResXDetId{ nullptr };
  std::shared_ptr<TH2F> mClustResYDetId{ nullptr };

  ///////////////////////////
  int mROF = 0;
  o2::mid::Mapping mMapping; ///< Mapping
};

} // namespace o2::quality_control_modules::mid

#endif // QC_MODULE_MID_MIDCLUSTQCTASK_H
