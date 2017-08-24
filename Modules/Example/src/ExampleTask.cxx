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

namespace AliceO2 {
namespace QualityControlModules {
namespace Example {

ExampleTask::ExampleTask()
  : TaskInterface()
{
  for (int i = 0; i < 25; i++) {
    mHistos[i] = nullptr;
  }
}

ExampleTask::~ExampleTask()
{
  for (int i = 0; i < 25; i++) {
    if (mHistos[i]) {
      delete mHistos[i];
    }
  }
}

void ExampleTask::initialize()
{
  QcInfoLogger::GetInstance() << "initialize ExampleTask" << AliceO2::InfoLogger::InfoLogger::endm;

  for (int i = 0; i < 25; i++) {
    stringstream name;
    name << "array-" << i;
    mHistos[i] = new TH1F(name.str().c_str(), name.str().c_str(), 100, 0, 99);
    getObjectsManager()->startPublishing(mHistos[i], name.str());
  }

  // Extendable axis
  mHistos[0]->SetCanExtend(TH1::kXaxis);

  // Add checks (first by name, then by reference)
  getObjectsManager()->addCheck("array-0", "checkMeanIsAbove",
                                "AliceO2::QualityControlModules::Common::MeanIsAbove", "QcCommon");
  getObjectsManager()->addCheck(mHistos[0], "checkNonEmpty", "AliceO2::QualityControlModules::Common::NonEmpty",
                                "QcCommon");
  getObjectsManager()->addCheck(mHistos[0], "checkFromExample", "AliceO2::QualityControlModules::Example::FakeCheck",
                                "QcExample");
}

void ExampleTask::startOfActivity(Activity &activity)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
  for (int i = 0; i < 25; i++) {
    mHistos[i]->Reset();
  }
}

void ExampleTask::startOfCycle()
{
  QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void ExampleTask::monitorDataBlock(DataSetReference dataSet)
{
  mHistos[0]->Fill(dataSet->at(0)->getData()->header.dataSize);
  for (int i = 1; i < 25; i++) {
    mHistos[i]->FillRandom("gaus", 1);
  }
}

void ExampleTask::endOfCycle()
{
  QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void ExampleTask::endOfActivity(Activity &activity)
{
  QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void ExampleTask::reset()
{
  QcInfoLogger::GetInstance() << "Reset" << AliceO2::InfoLogger::InfoLogger::endm;
}

}
}
}
