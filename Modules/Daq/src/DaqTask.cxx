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
using namespace o2::header;

namespace o2::quality_control_modules::daq
{

DaqTask::DaqTask()
  : TaskInterface(),
    mInputRecordPayloadSize(nullptr),
    mNumberInputs(nullptr),
    mInputSize(nullptr)
{
}

DaqTask::~DaqTask()
{
  delete mInputRecordPayloadSize;
  delete mNumberInputs;
  delete mInputSize;
}

// TODO remove this function once the equivalent is available in O2 DAQID
bool isDetIdValid(DAQID::ID id)
{
  return (id < DAQID::MAXDAQ+1) && (DAQID::DAQtoO2(id) != gDataOriginInvalid);
}

void DaqTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "Initializiation of DaqTask" << ENDM;

  // blocks plots
  mInputRecordPayloadSize = new TH1F("inputRecordSize", "Total payload size per InputRecord;bytes", 128, 0, 2047);
  mInputRecordPayloadSize->SetCanExtend(TH1::kXaxis);
  getObjectsManager()->startPublishing(mInputRecordPayloadSize);
  mNumberInputs = new TH1F("numberInputs", "Number of inputs per InputRecords", 100, 1, 100);
  getObjectsManager()->startPublishing(mNumberInputs);
  mInputSize = new TH1F("payloadSizeInputs", "Payload size of the inputs;bytes", 128, 0, 2047);
  mInputSize->SetCanExtend(TH1::kXaxis);
  getObjectsManager()->startPublishing(mInputSize);
  mNumberRDHs = new TH1F("numberRDHs", "Number of RDHs per InputRecord", 100, 1, 100);
  mNumberRDHs->SetCanExtend(TH1::kXaxis);
  getObjectsManager()->startPublishing(mNumberRDHs);

  // initialize a map for the subsystems (id, name)
  for(int i = DAQID::MINDAQ ; i < DAQID::MAXDAQ+1 ; i++) {
    DataOrigin origin = DAQID::DAQtoO2(i);
    if(origin != gDataOriginInvalid) {
      mSystems[i] = origin.str;
    }
  }
  mSystems[DAQID::INVALID] = "UNKNOWN"; // to store RDH info for unknown detectors

  // subsystems plots
  for(const auto& system : mSystems)  {
    string name = system.second + "/sumRdhSizesPerInputRecord";
    string title = "Sum of RDH sizes per InputRecord for " + system.second + ";bytes";
    mSubSystemsTotalSizes[system.first] = new TH1F(name.c_str(), title.c_str(), 128, 0, 2047);
    mSubSystemsTotalSizes[system.first]->SetCanExtend(TH1::kXaxis);
    getObjectsManager()->startPublishing(mSubSystemsTotalSizes[system.first]);

    name = system.second + "/RdhSizes";
    title = "RDH sizes for " + system.second + ";bytes";
    mSubSystemsRdhSizes[system.first] = new TH1F(name.c_str(), title.c_str(), 128, 0, 2047);
    mSubSystemsRdhSizes[system.first]->SetCanExtend(TH1::kXaxis);
    getObjectsManager()->startPublishing(mSubSystemsRdhSizes[system.first]);
  }
}

void DaqTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity: " << activity.mId << ENDM;
  mInputRecordPayloadSize->Reset();
  mNumberInputs->Reset();
  mInputSize->Reset();
  // TODO all plots
}

void DaqTask::startOfCycle() {
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}
//int i = 0 ;
void DaqTask::monitorData(o2::framework::ProcessingContext& ctx)
{
//  if(i < 2) {
  monitorBlocks(ctx.inputs());
  monitorRDHs(ctx.inputs());
//  i++;
//  }
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
  // todo probably all plots
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
      mInputSize->Fill(size);
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
  mInputRecordPayloadSize->Fill(totalPayloadSize);
  // for the number of subblocks, never use inputRecord.size() because it would just return
  // the number of inputs declared, not the ones valid and ready now.
  // When released in O2, use InputRecord::countValidInputs()
  size_t numberValidBlocks = std::distance(inputRecord.begin(), inputRecord.end());
  mNumberInputs->Fill(numberValidBlocks);
}

void printPage(const DPLRawParser::Iterator<const DataRef>& data)
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

void DaqTask::monitorRDHs(o2::framework::InputRecord& inputRecord)
{
  // Now, use the DPLRawParser to get information about the Pages and RDHs stored there
  o2::framework::DPLRawParser parser(inputRecord);
  size_t rdhCounter = 0;
  size_t totalSize = 0;
  DAQID::ID rdhSource = DAQID::INVALID;
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

    // RDH plots
    // DAQID::DAQtoO2(rdh.sourceID).str
//    uint8_t sourceId = RDHUtils::getSourceID(rdh);
//    cout << "detector : " << unsigned(sourceId) << endl;
//    cout << DAQID::DAQtoO2(sourceId).str << endl;
//    cout << mSystems.at(sourceId) << endl;
    rdhSource = RDHUtils::getSourceID(rdh);
    if(!isDetIdValid(rdhSource)) {
      rdhSource = DAQID::INVALID;
      ILOG(Warning, Support) << "Unknown detector id: " << unsigned(rdhSource) << ENDM;
    }
    totalSize += RDHUtils::getMemorySize(rdh);
    rdhCounter++;
    cout << "b4 " << endl;
    mSubSystemsRdhSizes.at(rdhSource)->Fill(RDHUtils::getMemorySize(rdh));
    cout << "after" << endl;
//    mSystemSizes[sourceId].Fill(); // should I fill with the page's payload size ? the page size ? the rdh payload declared ?
  }

  cout << "total rdh size: " << totalSize << endl;
  cout << "detector : " << unsigned(rdhSource) << endl;

  mSubSystemsTotalSizes.at(rdhSource)->Fill(totalSize);
    // TODO why is the payload size reported by the dataref.header->print() different than the one from the sum
  //      of the RDH memory size + dataref header size ? a few hundreds bytes difference.
  mNumberRDHs->Fill(rdhCounter);
}

} // namespace o2::quality_control_modules::daq
