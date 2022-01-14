// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
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
/// \file   RawDataQcTask.cxx
/// \author Marek Bombara
///

#include <TCanvas.h>
#include <TH1.h>

#include "QualityControl/QcInfoLogger.h"
#include "CTP/RawDataQcTask.h"
#include "DetectorsRaw/RDHUtils.h"
#include "Headers/RAWDataHeader.h"
#include "DPLUtils/DPLRawParser.h"
#include "DataFormatsCTP/Digits.h"
//#include "Detectors/CTP/RawToDigitConverterSpec.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>

namespace o2::quality_control_modules::ctp
{

RawDataQcTask::~RawDataQcTask()
{
  delete mHistogram;
  delete mHistogram2;
}

void RawDataQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize RawDataQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - myOwnKey: " << param->second << ENDM;
  }

  mHistogram = new TH1F("example", "example", 20, 0, 30000);
  mHistogram2 = new TH1F("example3", "example3", 20, 0, 20);
  getObjectsManager()->startPublishing(mHistogram);
  getObjectsManager()->startPublishing(mHistogram2);
  getObjectsManager()->startPublishing(new TH1F("example2", "example2", 20, 0, 30000));
  try {
    getObjectsManager()->addMetadata(mHistogram->GetName(), "custom", "34");
    getObjectsManager()->addMetadata(mHistogram2->GetName(), "custom", "34");
  } catch (...) {
    // some methods can throw exceptions, it is indicated in their doxygen.
    // In case it is recoverable, it is recommended to catch them and do something meaningful.
    // Here we don't care that the metadata was not added and just log the event.
    ILOG(Warning, Support) << "Metadata could not be added to " << mHistogram->GetName() << ENDM;
    ILOG(Warning, Support) << "Metadata could not be added to " << mHistogram2->GetName() << ENDM;
  }
}

void RawDataQcTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity " << activity.mId << ENDM;
  mHistogram->Reset();
  mHistogram2->Reset();
}

void RawDataQcTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void RawDataQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // In this function you can access data inputs specified in the JSON config file, for example:
  //   "query": "random:ITS/RAWDATA/0"
  // which is correspondingly <binding>:<dataOrigin>/<dataDescription>/<subSpecification
  // One can also access conditions from CCDB, via separate API (see point 3)

  // Use Framework/DataRefUtils.h or Framework/InputRecord.h to access and unpack inputs (both are documented)
  // One can find additional examples at:
  // https://github.com/AliceO2Group/AliceO2/blob/dev/Framework/Core/README.md#using-inputs---the-inputrecord-api

  // get the input
  o2::framework::DPLRawParser parser(ctx.inputs());
  LOG(info) << "hello =========================";
  // loop over input
  o2::ctp::gbtword80_t remnant = 0;
  uint32_t size_gbt = 0;

  for (auto it = parser.begin(), end = parser.end(); it != end; ++it) {
    // get the header
    auto const* rdh = it.get_if<o2::header::RAWDataHeader>();
    //o2::raw::RDHUtils::printRDH(rdh);
    auto triggerBC = o2::raw::RDHUtils::getTriggerBC(rdh);
    //LOG(info) << "trigger BC = " << triggerBC;
    auto triggerOrbit = o2::raw::RDHUtils::getTriggerOrbit(rdh);
    //LOG(info) << "trigger orbit = " << triggerOrbit;
    mHistogram->Fill(triggerOrbit);
    mHistogram2->Fill(triggerBC);

    uint32_t payloadCTP;
    auto feeID = o2::raw::RDHUtils::getFEEID(rdh); // 0 = IR, 1 = TCR
    auto linkCRU = (feeID & 0xf00) >> 8;
    if (linkCRU == o2::ctp::GBTLinkIDIntRec) {
      payloadCTP = o2::ctp::NIntRecPayload;
    } else if (linkCRU == o2::ctp::GBTLinkIDClassRec) {
      payloadCTP = o2::ctp::NClassPayload;
    } else {
      LOG(error) << "Unxpected  CTP CRU link:" << linkCRU;
    }
    LOG(info) << "RDH FEEid: " << feeID << " CTP CRU link:" << linkCRU << " Orbit:" << triggerOrbit << " payloadCTP = " << payloadCTP;

    gsl::span<const uint8_t> payload(it.data(), it.size());
    o2::ctp::gbtword80_t gbtWord = 0;
    int wordCount = 0;
    std::vector<o2::ctp::gbtword80_t> diglets;

    for (auto payloadWord : payload) 
    {
      //LOG(info) << wordCount << " payload:" <<  int(payloadWord);
      //std::cout << wordCount << " payload: 0x" << std::hex << payloadWord << std::endl;
      if (wordCount == 15) {
        wordCount = 0;
      } else if (wordCount > 9) {
        wordCount++;
      } else if (wordCount == 9) {
        for (int i = 0; i < 8; i++) {
          gbtWord[wordCount * 8 + i] = bool(int(payloadWord) & (1 << i));
        }
        wordCount++;
        LOG(info) << " gbtword:" << gbtWord;
        //makeGBTWordInverse(diglets, gbtWord, remnant, size_gbt, payloadCTP);
        gbtWord = 0;
      } else {
        for (int i = 0; i < 8; i++) {
            gbtWord[wordCount * 8 + i] = bool(int(payloadWord) & (1 << i));
            //gbtWord[(9-wordCount) * 8 + i] = bool(int(payloadWord) & (1 << i));
        }
        wordCount++;
      }
    }

  }
  // Some examples:
/*
  // 1. In a loop
  for (auto&& input : framework::InputRecordWalker(ctx.inputs())) {
    // get message header
    const auto* header = header::get<header::DataHeader*>(input.header);
    // get payload of a specific input, which is a char array.
    // const char* payload = input.payload;

    // for the sake of an example, let's fill the histogram with payload sizes
    mHistogram->Fill(header->payloadSize);
    mHistogram2->Fill(4.);
  }
*/
  // 2. Using get("<binding>")

  // get the payload of a specific input, which is a char array. "random" is the binding specified in the config file.
  //   auto payload = ctx.inputs().get("random").payload;

  // get payload of a specific input, which is a structure array:
  //  const auto* header = header::get<header::DataHeader*>(ctx.inputs().get("random").header);
  //  struct s {int a; double b;};
  //  auto array = ctx.inputs().get<s*>("random");
  //  for (int j = 0; j < header->payloadSize / sizeof(s); ++j) {
  //    int i = array.get()[j].a;
  //  }

  // get payload of a specific input, which is a root object
  //   auto h = ctx.inputs().get<TH1F*>("histos");
  //   Double_t stats[4];
  //   h->GetStats(stats);
  //   auto s = ctx.inputs().get<TObjString*>("string");
  //   LOG(info) << "String is " << s->GetString().Data();

  // 3. Access CCDB. If it is enough to retrieve it once, do it in initialize().
  // Remember to delete the object when the pointer goes out of scope or it is no longer needed.
  //   TObject* condition = TaskInterface::retrieveCondition("QcTask/example"); // put a valid condition path here
  //   if (condition) {
  //     LOG(info) << "Retrieved " << condition->ClassName();
  //     delete condition;
  //   }
}

void RawDataQcTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void RawDataQcTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void RawDataQcTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  mHistogram->Reset();
  mHistogram2->Reset();
}

} // namespace o2::quality_control_modules::ctp
