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
/// \file   DaqTask.h
/// \author Barthelemy von Haller
///

#ifndef QC_MODULE_DAQ_DAQTASK_H
#define QC_MODULE_DAQ_DAQTASK_H

#include "QualityControl/TaskInterface.h"

class TH1F;
class TGraph;
class TObjString;
class TCanvas;
class TPaveText;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::daq
{

/// \brief Dataflow task
/// It does only look at the header and plots sizes (e.g. payload).
/// It also can print the headers and the payloads by setting printHeaders to "1"
/// and printPayload to "hex" or "bin" in the config file under "taskParameters".
/// \author Barthelemy von Haller
class DaqTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  DaqTask();
  /// Destructor
  ~DaqTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  TH1F* mPayloadSize;
  TGraph* mIds;
  int mNPoints;
  TH1F* mNumberSubblocks;
  TH1F* mSubPayloadSize;
  //  UInt_t mTimeLastRecord;
};

} // namespace o2::quality_control_modules::daq

#endif // QC_MODULE_DAQ_DAQTASK_H
