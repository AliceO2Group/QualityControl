///
/// \file   SkeletonTask.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#include <TCanvas.h>
#include <TH1.h>

#include "QualityControl/QcInfoLogger.h"
#include "Skeleton/SkeletonTask.h"

using namespace std;

namespace o2
{
namespace quality_control_modules
{
namespace skeleton
{

SkeletonTask::SkeletonTask() : TaskInterface(), mHistogram(nullptr) { mHistogram = nullptr; }

SkeletonTask::~SkeletonTask() {}

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
  // todo: update API examples or refer to DPL README.md

  //  QcInfoLogger::GetInstance() << "monitorData" << AliceO2::InfoLogger::InfoLogger::endm;

  // exemplary ways of accessing inputs (incoming data), that were specified in the .ini file - e.g.:
  //  [readoutInput]
  //  inputName=readout
  //  dataOrigin=ITS
  //  dataDescription=RAWDATA

  // 1. in a loop
  for (auto&& input : ctx.inputs()) {
    const auto* header = o2::header::get<header::DataHeader*>(input.header);
    mHistogram->Fill(header->payloadSize);

    // const char* payload = input.payload;
  }

  // 2. get payload of a specific input, which is a char array. Change <inputName> to the previously specified binding
  // (e.g. readout).
  // auto payload = ctx.inputs().get("<inputName>").payload;
  //
  // 3. get payload of a specific input, which is a structure array:
  // const auto* header = o2::header::get<DataHeader*>(ctx.inputs().get("<inputName>").header);
  // auto structures = reinterpret_cast<StructureType*>(ctx.inputs().get("<inputName>").payload);
  // for (int j = 0; j < header->payloadSize / sizeof(StructureType); ++j) {
  //   someProcessing(structures[j].someField);
  // }

  // 4. get payload of a specific input, which is a root object
  // auto h = ctx.inputs().get<TH1F>("histos");
  // Double_t stats[4];
  // h->GetStats(stats);
  // auto s = ctx.inputs().get<TObjString>("string");
  // LOG(INFO) << "String is " << s->GetString().Data();
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

  QcInfoLogger::GetInstance() << "Reseting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;
  mHistogram->Reset();
}

} // namespace skeleton
} // namespace quality_control_modules
} // namespace o2
