///
/// \file   SkeletonTask.cxx
/// \author Barthelemy von Haller
///

#include "Skeleton/SkeletonTask.h"
#include "QualityControl/QcInfoLogger.h"
#include <TH1.h>
#include <TCanvas.h>

using namespace std;

namespace o2 {
namespace quality_control_modules {
namespace skeleton {

SkeletonTask::SkeletonTask()
  : TaskInterface(), mHistogram(nullptr)
{
  mHistogram = nullptr;
}

SkeletonTask::~SkeletonTask()
{
}

void SkeletonTask::initialize()
{
  QcInfoLogger::GetInstance() << "initialize SkeletonTask" << AliceO2::InfoLogger::InfoLogger::endm;

  mHistogram = new TH1F("example", "example", 100, 0, 99);
  getObjectsManager()->startPublishing(mHistogram);
  getObjectsManager()->addCheck(mHistogram, "checkFromSkeleton",
                                "o2::quality_control_modules::skeleton::SkeletonCheck",
                                "QcSkeleton");
}

void SkeletonTask::startOfActivity(Activity &activity)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void SkeletonTask::startOfCycle()
{
  QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void SkeletonTask::monitorDataBlock(DataSetReference block)
{
  mHistogram->FillRandom("gaus", 1);
}

void SkeletonTask::endOfCycle()
{
  QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void SkeletonTask::endOfActivity(Activity &activity)
{
  QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void SkeletonTask::reset()
{
  QcInfoLogger::GetInstance() << "Reset" << AliceO2::InfoLogger::InfoLogger::endm;
}

} // namespace skeleton
} // namespace quality_control_modules
} // namespace o2
