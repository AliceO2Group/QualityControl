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
/// \file   TOFTaskCompressedCounter.h
/// \author Nicolo' Jacazio
///

#ifndef QC_MODULE_TOF_TOFTASKCOMPRESSEDCOUNTER_H
#define QC_MODULE_TOF_TOFTASKCOMPRESSEDCOUNTER_H

// QC includes
#include "QualityControl/TaskInterface.h"
#include "TOF/TOFCounter.h"

class TH1F;
class TH2F;
class TH1I;
class TH2I;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::tof
{

/// \brief TOF Quality Control DPL Task for TOF Compressed data
/// \author Nicolo' Jacazio
class TOFTaskCompressedCounter /*final*/
  : public TaskInterface       // todo add back the "final" when doxygen is fixed
{
 public:
  /// \brief Constructor
  TOFTaskCompressedCounter();
  /// Destructor
  ~TOFTaskCompressedCounter() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  // Histograms
  std::shared_ptr<TH1F> mRDHCounterCrate0; /// TOF raw hit multiplicity per event
  TOFCounter mCounter; /// Decoder for TOF Compressed data useful for the Task
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_TOFTASKCOMPRESSEDCOUNTER_H
