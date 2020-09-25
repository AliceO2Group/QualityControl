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
#include <Headers/DAQID.h>
#include <map>

class TH1F;
class TGraph;
class TObjString;
class TCanvas;
class TPaveText;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::daq
{

//class MyDAQID{
// public:
//  // the following constexpr should probably go to DAQID.h or DataHeader.h in O2.
//  static constexpr o2::header::DAQID::ID NSYSTEMS = 19;
//};

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
  void printInputPayload(const header::DataHeader* header, const char* payload);
  void monitorBlocks(o2::framework::InputRecord& inputRecord);
  void monitorRDHs(o2::framework::InputRecord& inputRecord);

  // ** general information

  std::map<o2::header::DAQID::ID, std::string> mSystems;

  // ** objects we publish **

  // Message related
  // Block = the whole InputRecord, i.e. the thing we receive and analyse in monitorData(...)
  // SubBlock = a single input of the InputRecord
  TH1F* mInputRecordPayloadSize;  // filled with the sum of the payload size of all the inputs of an inputrecord
  TH1F* mNumberInputs; // filled with the number of inputs in each InputRecord we encounter
  TH1F* mInputSize; // filled with the number of inputs in each InputRecord we encounter
  TH1F* mNumberRDHs; // filled with the number of RDHs found in each InputRecord we encounter

  // Per link information

  // Per detector information
  std::map<o2::header::DAQID::ID, TH1F*> mSubSystemsTotalSizes; // filled with the sum of RDH memory sizes per InputRecord
  std::map<o2::header::DAQID::ID, TH1F*> mSubSystemsRdhSizes; // filled with the RDH memory sizes for each RDH
  // todo : for the next one we need to know the number of links per detector.
  std::map<o2::header::DAQID::ID, TH1F*> mSubSystemsRdhHits; // hits per link split by detector
};

} // namespace o2::quality_control_modules::daq

#endif // QC_MODULE_DAQ_DAQTASK_H
