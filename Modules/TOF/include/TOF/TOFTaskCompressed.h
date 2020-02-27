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
/// \file   TOFTaskCompressed.h
/// \author Nicolo' Jacazio
///

#ifndef QC_MODULE_TOF_TOFTASKCOMPRESSED_H
#define QC_MODULE_TOF_TOFTASKCOMPRESSED_H

// QC includes
#include "QualityControl/TaskInterface.h"
#include "TOF/TOFDecoderCompressed.h"

class TH1F;
class TH2F;
class TH1I;
class TH2I;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::tof
{

/// \brief TOF Quality Control DPL Task for TOF Compressed data
/// \author Nicolo' Jacazio
class TOFTaskCompressed /*final*/
  : public TaskInterface // todo add back the "final" when doxygen is fixed
{
 public:
  /// \brief Constructor
  TOFTaskCompressed();
  /// Destructor
  ~TOFTaskCompressed() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  TOFDecoderCompressed mDecoder;         /// Decoder for TOF Compressed data useful for the Task
  std::shared_ptr<TH1F> mHits;           /// Number of TOF hits
  std::shared_ptr<TH1F> mTime;           /// Time
  std::shared_ptr<TH1F> mTimeBC;         /// Time in Bunch Crossing
  std::shared_ptr<TH1F> mTOT;            /// Time-Over-Threshold
  std::shared_ptr<TH1F> mIndexE;         /// Index in electronic
  std::shared_ptr<TH2F> mSlotEnableMask; /// Enabled slot
  std::shared_ptr<TH2F> mDiagnostic;     /// Diagnostic histogram

};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_TOFTASKCOMPRESSED_H
