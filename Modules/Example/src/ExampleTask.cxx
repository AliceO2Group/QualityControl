///
/// \file   ExampleTask.cxx
/// \author Barthelemy von Haller
///

#include "Example/ExampleTask.h"
#include "QualityControl/QcInfoLogger.h"
#include <TH1.h>
#include <TCanvas.h>
#include <Common/DataBlock.h>

using namespace std;

namespace o2 {
namespace quality_control_modules {
namespace example {

ExampleTask::ExampleTask()
  : TaskInterface()
{
  for (auto &mHisto : mHistos) {
    mHisto = nullptr;
  }
  mNumberCycles = 0;
}

ExampleTask::~ExampleTask()
{
  for (auto &mHisto : mHistos) {
    if (mHisto) {
      delete mHisto;
    }
  }
}

void ExampleTask::initialize()
{
  QcInfoLogger::GetInstance() << "initialize ExampleTask" << AliceO2::InfoLogger::InfoLogger::endm;

  for (int i = 0; i < 24; i++) {
    publishHisto(i);
  }

  // Extendable axis
  mHistos[0]->SetCanExtend(TH1::kXaxis);

  // Add checks (first by name, then by reference)
  getObjectsManager()->addCheck("array-0", "checkMeanIsAbove",
                                "o2::quality_control_modules::common::MeanIsAbove", "QcCommon");
  getObjectsManager()->addCheck(mHistos[0], "checkNonEmpty", "o2::quality_control_modules::common::NonEmpty",
                                "QcCommon");
  getObjectsManager()->addCheck(mHistos[0], "checkFromExample", "o2::quality_control_modules::example::FakeCheck",
                                "QcExample");
}

void ExampleTask::publishHisto(int i)
{
  stringstream name;
  name << "array-" << i;
  mHistos[i] = new TH1F(name.str().c_str(), name.str().c_str(), 100, 0, 99);
  getObjectsManager()->startPublishing(mHistos[i], name.str());
}

void ExampleTask::startOfActivity(Activity &activity)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
  for (auto &mHisto : mHistos) {
    if (mHisto) {
      mHisto->Reset();
    }
  }
}

void ExampleTask::startOfCycle()
{
  QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void ExampleTask::monitorDataBlock(DataSetReference dataSet)
{
  mHistos[0]->Fill(dataSet->at(0)->getData()->header.dataSize);
  for (auto &mHisto : mHistos) {
    if (mHisto) {
      mHisto->FillRandom("gaus", 1);
    }
  }
}

void ExampleTask::endOfCycle()
{
  QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
  mNumberCycles++;

  // Add one more object just to show that we can do it
  if(mNumberCycles == 3) {
    publishHisto(24);
  }
}

void ExampleTask::endOfActivity(Activity &activity)
{
  QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void ExampleTask::reset()
{
  QcInfoLogger::GetInstance() << "Reset" << AliceO2::InfoLogger::InfoLogger::endm;
}

} // namespace example
} // namespace quality_control_modules
} // namespace o2
