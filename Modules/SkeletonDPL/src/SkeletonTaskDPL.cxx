///
/// \file   SkeletonTask.cxx
/// \author Barthelemy von Haller
///

#include <TH1.h>
#include <TCanvas.h>

#include "SkeletonDPL/SkeletonTaskDPL.h"
#include "QualityControl/QcInfoLogger.h"

using namespace std;

namespace o2 {
namespace quality_control_modules {
namespace skeleton_dpl {

SkeletonTaskDPL::SkeletonTaskDPL()
  : TaskInterfaceDPL(), mHistogram(nullptr)
{
  mHistogram = nullptr;
}

SkeletonTaskDPL::~SkeletonTaskDPL()
{
}

void SkeletonTaskDPL::initialize(o2::framework::InitContext& ctx)
{
  QcInfoLogger::GetInstance() << "initialize SkeletonTaskDPL" << AliceO2::InfoLogger::InfoLogger::endm;

  mHistogram = new TH1F("example", "example", 20, 0, 10000);
  getObjectsManager()->startPublishing(mHistogram);
//  getObjectsManager()->addCheck(mHistogram, "checkFromSkeleton",
//                                "o2::quality_control_modules::skeleton_dpl::_SkeletonCheck",
//                                "QcSkeletonDpl");
}

void SkeletonTaskDPL::startOfActivity(Activity &activity)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
  mHistogram->Reset();
}

void SkeletonTaskDPL::startOfCycle()
{
  QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void SkeletonTaskDPL::monitorDataBlock(o2::framework::ProcessingContext& ctx)
{
  QcInfoLogger::GetInstance() << "monitorDataBlock" << AliceO2::InfoLogger::InfoLogger::endm;

  // exemplary ways of accessing inputs:
  // 1. in a loop
  for (auto && input : ctx.inputs()){
    const auto* header = o2::header::get<header::DataHeader*>(input.header);
    mHistogram->Fill(header->payloadSize);

    // const char* payload = input.payload;
  }

  // 2. get payload of a specific input, which is a char array:
  // auto payload = ctx.inputs().get("binding").payload;
  //
  // 3. get payload of a specific input, which is a structure array:
  // const auto* header = o2::header::get<DataHeader*>(ctx.inputs().get("binding").header);
  // auto structures = reinterpret_cast<StructureType*>(ctx.inputs().get("binding").payload);
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

void SkeletonTaskDPL::endOfCycle()
{
  QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void SkeletonTaskDPL::endOfActivity(Activity &activity)
{
  QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void SkeletonTaskDPL::reset()
{
  QcInfoLogger::GetInstance() << "Reset" << AliceO2::InfoLogger::InfoLogger::endm;
}

}
}
}
