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

// QC
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/stringUtils.h"
// ROOT
#include <TH1.h>
// O2
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
  : TaskInterface()
{
}

DaqTask::~DaqTask()
{
  delete mInputRecordPayloadSize;
  delete mNumberInputs;
  delete mInputSize;
  delete mNumberRDHs;
}

// TODO remove this function once the equivalent is available in O2 DAQID
bool isDetIdValid(DAQID::ID id)
{
  return (id < DAQID::MAXDAQ + 1) && (DAQID::DAQtoO2(id) != gDataOriginInvalid);
}

void DaqTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "Initializiation of DaqTask" << ENDM;

  // General plots, related mostly to the payload size (InputRecord, Inputs) and the numbers of RDHs and Inputs in an InputRecord.
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
  for (int i = DAQID::MINDAQ; i < DAQID::MAXDAQ + 1; i++) {
    DataOrigin origin = DAQID::DAQtoO2(i);
    if (origin != gDataOriginInvalid) {
      mSystems[i] = origin.str;
    }
  }
  mSystems[DAQID::INVALID] = "UNKNOWN"; // to store RDH info for unknown detectors

  // subsystems plots: distribution of rdh size, distribution of the sum of rdh in each message.
  for (const auto& system : mSystems) {
    string name = system.second + "/sumRdhSizesPerInputRecord";
    string title = "Sum of RDH sizes per InputRecord for " + system.second + ";bytes";
    mSubSystemsTotalSizes[system.first] = new TH1F(name.c_str(), title.c_str(), 128, 0, 2047);
    mSubSystemsTotalSizes[system.first]->SetCanExtend(TH1::kXaxis);

    name = system.second + "/RdhSizes";
    title = "RDH sizes for " + system.second + ";bytes";
    mSubSystemsRdhSizes[system.first] = new TH1F(name.c_str(), title.c_str(), 128, 0, 2047);
    mSubSystemsRdhSizes[system.first]->SetCanExtend(TH1::kXaxis);
  }
}

void DaqTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity: " << activity.mId << ENDM;
  reset();
}

void DaqTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void DaqTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  monitorInputRecord(ctx.inputs());
  monitorRDHs(ctx.inputs());
}

void DaqTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;

  // TODO make this optional once we are able to know the run number and the detectors included.
  //      It might still be necessary in test runs without a proper run number.
  for(auto toBeAdded : mToBePublished) {
    if(!getObjectsManager()->isBeingPublished(mSubSystemsTotalSizes[toBeAdded]->GetName())) {
      getObjectsManager()->startPublishing(mSubSystemsTotalSizes[toBeAdded]);
    }
    if(!getObjectsManager()->isBeingPublished(mSubSystemsRdhSizes[toBeAdded]->GetName())) {
      getObjectsManager()->startPublishing(mSubSystemsRdhSizes[toBeAdded]);
    }
  }
}

void DaqTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;

  for (const auto& system : mSystems) {
    if(!getObjectsManager()->isBeingPublished(mSubSystemsTotalSizes[system.first]->GetName())) {
      getObjectsManager()->stopPublishing(mSubSystemsTotalSizes[system.first]);
    }
    if(!getObjectsManager()->isBeingPublished(mSubSystemsRdhSizes[system.first]->GetName())) {
      getObjectsManager()->stopPublishing(mSubSystemsRdhSizes[system.first]);
    }
  }
}

void DaqTask::reset()
{
  ILOG(Info, Support) << "Reset" << ENDM;

  // TODO if the number of plots grows we should probably have a container with pointers/references to all of them.
  //      then we can just iterate over.

  mInputRecordPayloadSize->Reset();
  mNumberInputs->Reset();
  mInputSize->Reset();
  mNumberRDHs->Reset();

  for (const auto& system : mSystems) {
    mSubSystemsRdhSizes.at(system.first)->Reset();
    mSubSystemsTotalSizes.at(system.first)->Reset();
  }
}

void DaqTask::printInputPayload(const header::DataHeader* header, const char* payload)
{
  std::vector<std::string> representation;
  if (mCustomParameters["printInputPayload"] == "hex") {
    representation = getHexRepresentation((unsigned char*)payload, header->payloadSize);
  } else if (mCustomParameters["printInputPayload"] == "bin") {
    representation = getBinRepresentation((unsigned char*)payload, header->payloadSize);
  }
  size_t limit = std::numeric_limits<size_t>::max();
  if (mCustomParameters.count("printInputPayloadLimit") > 0) {
    limit = std::stoi(mCustomParameters["printInputPayloadLimit"]);
  }

  for (size_t i = 0; i < representation.size();) {
    ILOG(Info, Ops) << std::setw(4) << i << " : ";
    for (size_t col = 0; col < 4; col++) {
      for (size_t word = 0; word < 2; word++) {
        if (i + col * 2 + word < representation.size()) {
          ILOG(Info, Ops) << representation[i + col * 2 + word];
        } else {
          ILOG(Info, Ops) << "   ";
        }
      }
      ILOG(Info, Ops) << " | ";
    }
    ILOG(Info, Ops) << ENDM;
    i = i + 8;
    // limit output but we don't troncate the lines.
    if (i > limit) {
      return;
    }
  }
}

void DaqTask::monitorInputRecord(InputRecord& inputRecord)
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
  mNumberInputs->Fill(inputRecord.countValidInputs());
}

void printPage(const DPLRawParser::Iterator<const DataRef>& data)
{
  auto const* raw = data.raw();         // retrieving the raw pointer of the page
  auto const* rawPayload = data.data(); // retrieving payload pointer of the page
  size_t rawPayloadSize = data.size();  // size of payload
  size_t offset = data.offset();        // offset of payload in the raw page

  ILOG(Info, Ops) << "Page: " << ENDM;
  ILOG(Info, Ops) << "    payloadSize: " << rawPayloadSize << ENDM;
  ILOG(Info, Ops) << "    raw pointer of the page:           " << (void*)raw << ENDM;
  ILOG(Info, Ops) << "    payload pointer of the page:       " << (void*)rawPayload << ENDM;
  ILOG(Info, Ops) << "    offset of payload in the raw page: " << (void*)offset << ENDM;
}

void DaqTask::monitorRDHs(o2::framework::InputRecord& inputRecord)
{
  // Use the DPLRawParser to get information about the Pages and RDHs stored in the inputRecord
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
    rdhSource = RDHUtils::getSourceID(rdh);
    if (!isDetIdValid(rdhSource)) {
      rdhSource = DAQID::INVALID;
    }
    totalSize += RDHUtils::getMemorySize(rdh);
    rdhCounter++;
    mSubSystemsRdhSizes.at(rdhSource)->Fill(RDHUtils::getMemorySize(rdh));
  }

  mSubSystemsTotalSizes.at(rdhSource)->Fill(totalSize);

  // TODO make this optional once we are able to know the run number and the detectors included.
  mToBePublished.insert(rdhSource);

  // TODO why is the payload size reported by the dataref.header->print() different than the one from the sum
  //      of the RDH memory size + dataref header size ? a few hundreds bytes difference.
  mNumberRDHs->Fill(rdhCounter);
}

} // namespace o2::quality_control_modules::daq
