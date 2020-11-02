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
/// \file   BasicDigitQcTask.h
/// \author Milosz Filus
/// Example of QC Task for FT0 detector

#ifndef QC_MODULE_FT0_FT0BASICDIGITQCTASK_H
#define QC_MODULE_FT0_FT0BASICDIGITQCTASK_H

#include "QualityControl/TaskInterface.h"
#include <memory>
#include "TH1.h"
#include "TH2.h"
#include "TTree.h"
#include "TFile.h"
#include "TMultiGraph.h"
#include "Rtypes.h"

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::ft0
{

class BasicDigitQcTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  BasicDigitQcTask() = default;
  /// Destructor
  ~BasicDigitQcTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  // Object which will be published
  std::unique_ptr<TH1F> mChargeHistogram;
  std::unique_ptr<TH1F> mTimeHistogram;
  std::unique_ptr<TH2F> mAmplitudeAndTime;
  std::unique_ptr<TTree> mTTree;
};

} // namespace o2::quality_control_modules::ft0

#endif // QC_MODULE_FT0_FT0BasicDigitQcTask_H
