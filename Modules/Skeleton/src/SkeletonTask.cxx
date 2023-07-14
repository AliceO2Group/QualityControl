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
/// \file   SkeletonTask.cxx
/// \author My Name
///

#include <TCanvas.h>
#include <TH1.h>

#include "QualityControl/QcInfoLogger.h"
#include "Skeleton/SkeletonTask.h"
#include <Framework/InputRecordWalker.h>
#include <Framework/DataRefUtils.h>

namespace o2::quality_control_modules::skeleton
{

SkeletonTask::~SkeletonTask()
{
  delete mHistogram;
}

void SkeletonTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Debug, Devel) << "initialize SkeletonTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  ILOG(Debug, Support) << "Debug" << ENDM;                 // QcInfoLogger is used. FairMQ logs will go to there as well.
  ILOG(Info, Support) << "Info" << ENDM;                   // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - myOwnKey: " << param->second << ENDM;
  }

  mHistogram = new TH1F("example", "example", 20, 0, 30000);
  // this will make the two histograms published at the end of each cycle. no need to use the method in monitorData
  getObjectsManager()->startPublishing(mHistogram);
  getObjectsManager()->startPublishing(new TH1F("example2", "example2", 20, 0, 30000));
  try {
    getObjectsManager()->addMetadata(mHistogram->GetName(), "custom", "34");
  } catch (...) {
    // some methods can throw exceptions, it is indicated in their doxygen.
    // In case it is recoverable, it is recommended to catch them and do something meaningful.
    // Here we don't care that the metadata was not added and just log the event.
    ILOG(Warning, Support) << "Metadata could not be added to " << mHistogram->GetName() << ENDM;
  }
}

void SkeletonTask::startOfActivity(const Activity& activity)
{
  // THIS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
  mHistogram->Reset();

  // Example: retrieve custom parameters
  std::string parameter;
  // first we try for the current activity. It returns an optional.
  if (auto param = mCustomParameters.atOptional("myOwnKey", activity)) {
    parameter = param.value(); // we got a value
  } else {
    // we did not get a value. We ask for the "default" runtype and beamtype and we specify a default return value.
    parameter = mCustomParameters.atOrDefaultValue("myOwnKey", "some default");
  }
}

void SkeletonTask::startOfCycle()
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void SkeletonTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // THIS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.

  // In this function you can access data inputs specified in the JSON config file, for example:
  //   "query": "random:ITS/RAWDATA/0"
  // which is correspondingly <binding>:<dataOrigin>/<dataDescription>/<subSpecification
  // One can also access conditions from CCDB, via separate API (see point 3)

  // Use Framework/DataRefUtils.h or Framework/InputRecord.h to access and unpack inputs (both are documented)
  // One can find additional examples at:
  // https://github.com/AliceO2Group/AliceO2/blob/dev/Framework/Core/README.md#using-inputs---the-inputrecord-api

  // Some examples:

  // 1. In a loop
  for (auto&& input : framework::InputRecordWalker(ctx.inputs())) {
    // get message header
    const auto* header = o2::framework::DataRefUtils::getHeader<header::DataHeader*>(input);
    auto payloadSize = o2::framework::DataRefUtils::getPayloadSize(input);
    // get payload of a specific input, which is a char array.
    // const char* payload = input.payload;

    // for the sake of an example, let's fill the histogram with payload sizes
    mHistogram->Fill(payloadSize);
  }

  // 2. Using get("<binding>")

  // get the payload of a specific input, which is a char array. "random" is the binding specified in the config file.
  //   auto payload = ctx.inputs().get("random").payload;

  // get payload of a specific input, which is a structure array:
  //   auto input = ctx.inputs().get("random");
  //   auto payload = input.payload;

  // get payload of a specific input, which is a structure array:
  //  const auto* header = DataRefUtils::getHeader<header::DataHeader*>(input);
  //  auto payloadSize = DataRefUtils::getPayloadSize(input);
  //  struct s {int a; double b;};
  //  auto array = ctx.inputs().get<s*>(input);
  //  for (int j = 0; j < payloadSize / sizeof(s); ++j) {
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
  //     TObject* condition = TaskInterface::retrieveCondition("GRP/Calib/LHCClockPhase"); // put a valid condition or calibration object path here
  //     if (condition) {
  //       LOG(info) << "Retrieved " << condition->ClassName();
  //       delete condition;
  //     }
}

void SkeletonTask::endOfCycle()
{
  // THIS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void SkeletonTask::endOfActivity(const Activity& /*activity*/)
{
  // THIS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void SkeletonTask::reset()
{
  // THIS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.

  // clean all the monitor objects here

  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  mHistogram->Reset();
}

} // namespace o2::quality_control_modules::skeleton
