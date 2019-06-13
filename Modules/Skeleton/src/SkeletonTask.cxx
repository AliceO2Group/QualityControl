///
/// \file   SkeletonTask.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#include <TCanvas.h>
#include <TH1.h>

#include "QualityControl/QcInfoLogger.h"
#include "Skeleton/SkeletonTask.h"

namespace o2
{
namespace quality_control_modules
{
namespace skeleton
{

SkeletonTask::SkeletonTask() : TaskInterface(), mHistogram(nullptr) { mHistogram = nullptr; }

SkeletonTask::~SkeletonTask() {
  if (mHistogram) {
    delete mHistogram;
  }
}

void SkeletonTask::initialize(o2::framework::InitContext& ctx)
{
  QcInfoLogger::GetInstance() << "initialize SkeletonTask" << AliceO2::InfoLogger::InfoLogger::endm;

  mHistogram = new TH1F("example", "example", 20, 0, 30000);
  getObjectsManager()->startPublishing(mHistogram);
  getObjectsManager()->addCheck(mHistogram, "checkFromSkeleton", "o2::quality_control_modules::skeleton::SkeletonCheck",
                                "QcSkeleton");
}

void SkeletonTask::startOfActivity(Activity& activity)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
  mHistogram->Reset();
}

void SkeletonTask::startOfCycle()
{
  QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void SkeletonTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // In this function you can access data inputs specified in the JSON config file, for example:
  //  {
  //    "binding": "random",
  //    "dataOrigin": "ITS",
  //    "dataDescription": "RAWDATA"
  //  }

  // Use Framework/DataRefUtils.h or Framework/InputRecord.h to access and unpack inputs (both are documented)
  // One can find additional examples at:
  // https://github.com/AliceO2Group/AliceO2/blob/dev/Framework/Core/README.md#using-inputs---the-inputrecord-api

  // Some examples:

  // 1. In a loop
  for (auto&& input : ctx.inputs()) {
    // get message header
	 QcInfoLogger::GetInstance() << "BEEN HERE BRO" << AliceO2::InfoLogger::InfoLogger::endm;

	  const auto* header = header::get<header::DataHeader*>(input.header);
    // get payload of a specific input, which is a char array.
//    const char* payload = input.payload;

    // for the sake of an example, let's fill the histogram with payload sizes
    mHistogram->Fill(header->payloadSize);
  }

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
//   LOG(INFO) << "String is " << s->GetString().Data();
}

void SkeletonTask::endOfCycle()
{
  QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void SkeletonTask::endOfActivity(Activity& activity)
{
  QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void SkeletonTask::reset()
{
  // clean all the monitor objects here

  QcInfoLogger::GetInstance() << "Resetting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;
  mHistogram->Reset();
}

} // namespace skeleton
} // namespace quality_control_modules
} // namespace o2
  
