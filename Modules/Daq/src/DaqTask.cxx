// Copyright 2019-2020 CERN and  bopyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
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
// O2
#include <DPLUtils/DPLRawParser.h>
#include <DetectorsRaw/RDHUtils.h>
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <Headers/DataHeaderHelpers.h>
#include <stdexcept>
#include "Common/Utils.h"

using namespace std;
using namespace o2::raw;
using namespace o2::framework;
using namespace o2::header;

namespace o2::quality_control_modules::daq
{

int DaqTask::getIntParam(const std::string paramName, int defaultValue)
{
  return common::getFromConfig<int>(mCustomParameters, paramName, defaultValue);
}

void DaqTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initializiation of DaqTask" << ENDM;

  mInputRecordPayloadSize = std::make_unique<TH1F>("inputRecordSize", "Total payload size per InputRecord;bytes",
                                                   getIntParam("inputRecordSizeBins", 128),
                                                   getIntParam("inputRecordSizeMin", 0),
                                                   getIntParam("inputRecordSizeMax", 2047));
  getObjectsManager()->startPublishing(mInputRecordPayloadSize.get(), PublicationPolicy::Forever);

  mInputSize = std::make_unique<TH1F>("payloadSizeInputs", "Payload size of the inputs;bytes",
                                      getIntParam("payloadSizeInputsBins", 128),
                                      getIntParam("payloadSizeInputsMin", 0),
                                      getIntParam("payloadSizeInputsMax", 2047));
  getObjectsManager()->startPublishing(mInputSize.get(), PublicationPolicy::Forever);

  mNumberRDHs = std::make_unique<TH1F>("numberRdhs", "Number of RDHs per InputRecord",
                                       getIntParam("numberRDHsBins", 100),
                                       getIntParam("numberRDHsMin", 1),
                                       getIntParam("numberRDHsMax", 100));
  getObjectsManager()->startPublishing(mNumberRDHs.get(), PublicationPolicy::Forever);

  mSumRDHSizesPerInputRecord = std::make_unique<TH1F>("sumRdhSizesPerInputRecord", "Sum of RDH sizes per InputRecord;bytes",
                                                      getIntParam("sumRdhSizesPerInputRecordBins", 128),
                                                      getIntParam("sumRdhSizesPerInputRecordMin", 0),
                                                      getIntParam("sumRdhSizesPerInputRecordMax", 2047));
  getObjectsManager()->startPublishing(mSumRDHSizesPerInputRecord.get(), PublicationPolicy::Forever);

  mSumRDHSizesPerRDH = std::make_unique<TH1F>("RdhSizes", "RDH sizes; bytes",
                                              getIntParam("RdhSizesBins", 128),
                                              getIntParam("RdhSizesMin", 0),
                                              getIntParam("RdhSizesMax", 2047));
  getObjectsManager()->startPublishing(mSumRDHSizesPerRDH.get(), PublicationPolicy::Forever);

  mRDHSizesPerCRUIds = std::make_unique<TH2F>("RdhPayloadSizePerCRUid", "RDH payload size per CRU",
                                              getIntParam("RdhPayloadSizeBins", 128),
                                              getIntParam("RdhPayloadSizeMin", 0),
                                              getIntParam("RdhPayloadSizeMax", 2047),
                                              getIntParam("CRUidBins", 500),
                                              getIntParam("CRUidMin", 0),
                                              getIntParam("CRUidMax", 500));
  mRDHSizesPerCRUIds->GetXaxis()->SetTitle("bytes");
  mRDHSizesPerCRUIds->GetYaxis()->SetTitle("CRU Id");
  getObjectsManager()->startPublishing(mRDHSizesPerCRUIds.get(), PublicationPolicy::Forever);
}

void DaqTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity: " << activity.mId << ENDM;
  reset();
}

void DaqTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void DaqTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  monitorInputRecord(ctx.inputs());
  monitorRDHs(ctx.inputs());
}

void DaqTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void DaqTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;

  getObjectsManager()->stopPublishing(mInputRecordPayloadSize.get());
  getObjectsManager()->stopPublishing(mInputSize.get());
  getObjectsManager()->stopPublishing(mNumberRDHs.get());
  getObjectsManager()->stopPublishing(mSumRDHSizesPerInputRecord.get());
  getObjectsManager()->stopPublishing(mSumRDHSizesPerRDH.get());
  getObjectsManager()->stopPublishing(mRDHSizesPerCRUIds.get());
}

void DaqTask::reset()
{
  ILOG(Info, Support) << "Reset" << ENDM;

  // TODO if the number of plots grows we should probably have a container with pointers/references to all of them.
  //      then we can just iterate over.

  mInputRecordPayloadSize->Reset();
  mInputSize->Reset();
  mNumberRDHs->Reset();
  mSumRDHSizesPerRDH->Reset();
  mSumRDHSizesPerInputRecord->Reset();
}

void DaqTask::printInputPayload(const header::DataHeader* header, const char* payload, size_t payloadSize)
{
  std::vector<std::string> representation;
  if (mCustomParameters["printInputPayload"] == "hex") {
    representation = getHexRepresentation((unsigned char*)payload, payloadSize);
  } else if (mCustomParameters["printInputPayload"] == "bin") {
    representation = getBinRepresentation((unsigned char*)payload, payloadSize);
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
  for (const auto& input : InputRecordWalker(inputRecord)) {
    if (input.header != nullptr) {
      const auto* header = DataRefUtils::getHeader<header::DataHeader*>(input);
      const char* payload = input.payload;

      // payload size
      auto size = DataRefUtils::getPayloadSize(input);
      mInputSize->Fill(size);
      totalPayloadSize += size;

      // printing
      if (mCustomParameters.count("printInputHeader") > 0 && mCustomParameters["printInputHeader"] == "true") {
        std::cout << fmt::format("{}", *header) << std::endl;
      }
      if (mCustomParameters.count("printInputPayload") > 0) {
        printInputPayload(header, payload, size);
      }
    } else {
      ILOG(Warning, Support) << "Received an input with an empty header" << ENDM;
    }
  }
  mInputRecordPayloadSize->Fill(totalPayloadSize);
}

template <class T>
void printPage(const T& data)
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
  size_t totalSize = 0;
  size_t rdhCounter = 0;
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
    try {
      const auto RDHSize = RDHUtils::getMemorySize(rdh) - RDHUtils::getHeaderSize(rdh);
      mSumRDHSizesPerRDH->Fill(RDHSize);
      totalSize += RDHSize;
      rdhCounter++;

      mRDHSizesPerCRUIds->Fill(RDHSize, RDHUtils::getCRUID(rdh));

    } catch (std::runtime_error& e) {
      ILOG(Error, Devel) << "Caught an exception when accessing the rdh fields: \n"
                         << e.what() << ENDM;
    }
  }

  mSumRDHSizesPerInputRecord->Fill(totalSize);
  mNumberRDHs->Fill(rdhCounter);
}

} // namespace o2::quality_control_modules::daq
