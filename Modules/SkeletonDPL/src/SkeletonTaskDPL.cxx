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

  for (auto && input : ctx.inputs()){
    const auto* header = o2::header::get<header::DataHeader*>(input.header);
    mHistogram->Fill(header->payloadSize);
  }
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
