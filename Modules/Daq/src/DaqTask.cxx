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
#include <DPLUtils/DPLRawParser.h>
#include <DetectorsRaw/RDHUtils.h>
#include <Framework/InputRecord.h>

using namespace std;
using namespace o2::raw;
using namespace o2::framework;

namespace o2::quality_control_modules::daq
{

DaqTask::DaqTask()
  : TaskInterface(),
    mBlockSize(nullptr),
    mNumberSubBlocks(nullptr),
    mSubBlockSize(nullptr)
{
}

DaqTask::~DaqTask()
{
  delete mBlockSize;
  delete mNumberSubBlocks;
  delete mSubBlockSize;
}

void DaqTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize DaqTask" << ENDM;

  mBlockSize = new TH1F("payloadSize", "Payload size of blocks;bytes", 2048, 0, 2047);
  mBlockSize->SetCanExtend(TH1::kXaxis);
  getObjectsManager()->startPublishing(mBlockSize);

  mNumberSubBlocks = new TH1F("numberSubBlocks", "Number of subblocks", 100, 1, 100);
  getObjectsManager()->startPublishing(mNumberSubBlocks);
  mSubBlockSize = new TH1F("PayloadSizeSubBlocks", "Payload size of subblocks;bytes", 2048, 0, 2047);
  mSubBlockSize->SetCanExtend(TH1::kXaxis);
  getObjectsManager()->startPublishing(mSubBlockSize);
}

void DaqTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity: " << activity.mId << ENDM;
  mBlockSize->Reset();
  mNumberSubBlocks->Reset();
  mSubBlockSize->Reset();
}

void DaqTask::startOfCycle() {
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void DaqTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  monitorBlocks(ctx.inputs());
  monitorRDHs(ctx.inputs());
}

void DaqTask::endOfCycle() {
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void DaqTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void DaqTask::reset() {
  ILOG(Info, Support) << "Reset" << ENDM;
}

void DaqTask::printInputPayload(const header::DataHeader* header, const char* payload)
{
  std::vector<std::string> representation;
  if (mCustomParameters["printInputPayload"] == "hex") {
    representation = getHexRepresentation((unsigned char*)payload, header->payloadSize);
  } else if (mCustomParameters["printInputPayload"] == "bin") {
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

void DaqTask::monitorBlocks(InputRecord& inputRecord)
{
  uint32_t totalPayloadSize = 0;
  for (auto&& input : inputRecord) {
    if (input.header != nullptr) {
      const auto* header = o2::header::get<header::DataHeader*>(input.header);
      const char* payload = input.payload;

      // payload size
      uint32_t size = header->payloadSize;
      mSubBlockSize->Fill(size);
      totalPayloadSize += size;

      // printing
      if (mCustomParameters.count("printInputHeader") > 0 && mCustomParameters["printInputHeader"] == "true") {
        header->print();
      }
      if (mCustomParameters.count("printInputPayload") > 0) {
        printInputPayload(header, payload);
      }
    } else {
      ILOG(Warning, Support) << "Received an input with an empty header" << ENDM;
    }
  }
  mBlockSize->Fill(totalPayloadSize);
  mNumberSubBlocks->Fill(inputRecord.size());
}

void DaqTask::monitorRDHs(o2::framework::InputRecord& inputRecord)
{
  // Now, use the DPLRawParser to get information about the Pages and RDHs stored there
  o2::framework::DPLRawParser parser(inputRecord);
  for (auto it = parser.begin(), end = parser.end(); it != end; ++it) {
    // TODO for some reason this does not work
    //    ILOG(Info, Ops) << "Header: " << ENDM;
    //    const auto* header = o2::header::get<header::DataHeader*>(it.o2DataHeader());
    //    header->print();
    //    it.o2DataHeader()->print();

    // print page
    if (mCustomParameters.count("printPageInfo") > 0 && mCustomParameters["printPageInfo"] == "true") {
      printPage(it);
    }

    // retrieving RDH
    const auto& rdh = reinterpret_cast<const o2::header::RDHAny*>(it.raw());
    if (!rdh) {
      ILOG(Info, Ops) << "Cannot parse data to RAW data header" << ENDM;
      continue;
    }

    // print RDH
    if (mCustomParameters.count("printRDH") > 0 && mCustomParameters["printRDH"] == "true") {
      ILOG(Info, Ops) << "RDH: " << ENDM;
      RDHUtils::printRDH(rdh);
    }
  }
}

void DaqTask::printPage(const DPLRawParser::Iterator<const DataRef>& data)
{
  // retrieving the raw pointer of the page
  auto const* raw = data.raw();
  // retrieving payload pointer of the page
  auto const* rawPayload = data.data();
  // size of payload
  size_t rawPayloadSize = data.size();
  // offset of payload in the raw page
  size_t offset = data.offset();

  ILOG(Info, Ops) << "Page: " << ENDM;
  ILOG(Info, Ops) << "    payloadSize: " << rawPayloadSize << ENDM;
  ILOG(Info, Ops) << "    raw pointer of the page:           " << (void*)raw << ENDM;
  ILOG(Info, Ops) << "    payload pointer of the page:       " << (void*)rawPayload << ENDM;
  ILOG(Info, Ops) << "    offset of payload in the raw page: " << (void*)offset << ENDM;
}

} // namespace o2::quality_control_modules::daq
