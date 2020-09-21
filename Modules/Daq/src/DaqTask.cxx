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
/// \file   DaqTask.cxx
/// \author Barthelemy von Haller
///

#include "Daq/DaqTask.h"

#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/stringUtils.h"
#include <TGraph.h>
#include <TH1.h>

using namespace std;

namespace o2::quality_control_modules::daq
{

DaqTask::DaqTask()
  : TaskInterface(),
    mPayloadSize(nullptr),
    mIds(nullptr),
    mNPoints(0),
    mNumberSubblocks(nullptr),
    mSubPayloadSize(nullptr)
{
}

DaqTask::~DaqTask()
{
  delete mPayloadSize;
  delete mIds;
  delete mNumberSubblocks;
  delete mSubPayloadSize;
}

void DaqTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  QcInfoLogger::GetInstance() << "initialize DaqTask" << AliceO2::InfoLogger::InfoLogger::endm;

  mPayloadSize = new TH1F("payloadSize", "Payload size of blocks;bytes", 2048, 0, 2047);
  mPayloadSize->SetCanExtend(TH1::kXaxis);
  getObjectsManager()->startPublishing(mPayloadSize);

  mNumberSubblocks = new TH1F("numberSubBlocks", "Number of subblocks", 100, 1, 100);
  getObjectsManager()->startPublishing(mNumberSubblocks);
  mSubPayloadSize = new TH1F("PayloadSizeSubBlocks", "Payload size of subblocks;bytes", 2048, 0, 2047);
  mSubPayloadSize->SetCanExtend(TH1::kXaxis);
  getObjectsManager()->startPublishing(mSubPayloadSize);

  mIds = new TGraph();
  mIds->SetName("IDs");
  mIds->GetXaxis()->SetTimeDisplay(1);
  mIds->GetXaxis()->SetNdivisions(-503);
  mIds->GetXaxis()->SetTimeFormat("%Y-%m-%d %H:%M:%S");
  mIds->GetXaxis()->SetTimeOffset(0, "gmt");
  mIds->SetMarkerStyle(20);
  mIds->GetXaxis()->SetLabelSize(0.02);
  mIds->GetYaxis()->SetLabelSize(0.02);
  getObjectsManager()->startPublishing(mIds);
}

void DaqTask::startOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
  mPayloadSize->Reset();
  mNumberSubblocks->Reset();
  mSubPayloadSize->Reset();
  mIds->Set(0);
  mNPoints = 0;
}

void DaqTask::startOfCycle() { QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm; }

void DaqTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // what does it mean to have several inputs ? is it that we defined several in the config file ?
  // If I am connected to the readout can I ever receive several inputs ?
  // TODO maybe we need a special daq task for the readout because we have to look into the weird data structure.

  // in a loop
  uint32_t totalPayloadSize = 0;
  for (auto&& input : ctx.inputs()) {
    if (input.header != nullptr) {
      const auto* header = o2::header::get<header::DataHeader*>(input.header);
      const char* payload = input.payload;

      // payload size
      uint32_t size = header->payloadSize;
      mSubPayloadSize->Fill(size);
      totalPayloadSize += size;

      // printing
      if (mCustomParameters.count("printHeaders") > 0 && mCustomParameters["printHeaders"] == "true") {
        header->print();
      }
      if (mCustomParameters.count("printPayload") > 0) {
        std::vector<std::string> representation;
        if (mCustomParameters["printPayload"] == "hex") {
          representation = getHexRepresentation((unsigned char*)payload, header->payloadSize);
        } else if (mCustomParameters["printPayload"] == "bin") {
          representation = getBinRepresentation((unsigned char*)payload, header->payloadSize);
        }

        for (size_t i = 0; i < representation.size();) {
          ILOG(Info) << std::setw(4) << i << " : ";
          for (size_t col = 0; col < 4; col++) {
            for (size_t word = 0; word < 2; word++) {
              if (i + col * 2 + word < representation.size()) {
                ILOG(Info) << representation[i + col * 2 + word];
              } else {
                ILOG(Info) << "   ";
              }
            }
            ILOG(Info) << " | ";
          }
          ILOG(Info) << ENDM;
          i = i + 8;
        }
      }
    }
  }

  mPayloadSize->Fill(totalPayloadSize);
  mNumberSubblocks->Fill(ctx.inputs().size());

  // TODO if data has an id (like event id), we should plot it
  //  TDatime now;
  //  if ((now.Get() - mTimeLastRecord) >= 1) {
  //    mIds->SetPoint(mNPoints, now.Convert(), ctx.inputs().begin());
  //    mNPoints++;
  //    mTimeLastRecord = now.Get();
  //  }
}

void DaqTask::endOfCycle() { QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm; }

void DaqTask::endOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void DaqTask::reset() { QcInfoLogger::GetInstance() << "Reset" << AliceO2::InfoLogger::InfoLogger::endm; }

} // namespace o2::quality_control_modules::daq
