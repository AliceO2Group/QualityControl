// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   RawDataQcTask.h
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#ifndef QC_MODULE_MID_MIDRAWDATAQCTASK_H
#define QC_MODULE_MID_MIDRAWDATAQCTASK_H

#include "QualityControl/TaskInterface.h"

class TH1F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::mid
{

/// \brief Example Quality Control DPL Task
/// It is final because there is no reason to derive from it. Just remove it if needed.
/// \author Barthelemy von Haller
/// \author Piotr Konopka
class RawDataQcTask /*final*/ : public TaskInterface // todo add back the "final" when doxygen is fixed
{
 public:
  /// \brief Constructor
  RawDataQcTask() = default;
  /// Destructor
  ~RawDataQcTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  TH1F* mHistogram = nullptr;
};

} // namespace o2::quality_control_modules::mid

#endif // QC_MODULE_MID_MIDRAWDATAQCTASK_H
