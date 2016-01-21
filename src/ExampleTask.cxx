///
/// \file   ExampleTask.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/ExampleTask.h"
#include "QualityControl/QcInfoLogger.h"
#include <TH1.h>

#include <iostream>
#include <TCanvas.h>
#include <DataFormat/DataBlock.h>

//using namespace std;

namespace AliceO2 {
namespace QualityControl {
namespace ExampleModule {

ExampleTask::ExampleTask(std::string name, ObjectsManager *objectsManager)
  : TaskInterface(name, objectsManager), mHisto1(nullptr), mHisto2(nullptr)
{
}

ExampleTask::~ExampleTask()
{
}

void ExampleTask::initialize()
{
  QcInfoLogger::GetInstance() << "initialize" << AliceO2::InfoLogger::InfoLogger::endm;
  mHisto1 = new TH1F("first", "first", 2048, 0, 2047);
  mHisto2 = new TH1F("second", "second", 100, -10, 10);
  getObjectsManager()->startPublishing("my object", mHisto1);
  getObjectsManager()->startPublishing("my second object", mHisto2);
}

void ExampleTask::startOfActivity(Activity &activity)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
  mHisto1->Reset();
}

void ExampleTask::startOfCycle()
{
  QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void ExampleTask::monitorDataBlock(DataBlock &block)
{
  uint32_t payloadSizeBytes = block.header.dataSize / 8;
  QcInfoLogger::GetInstance() << "Payload size " << payloadSizeBytes << AliceO2::InfoLogger::InfoLogger::endm;
  mHisto1->Fill(payloadSizeBytes);
  mHisto2->FillRandom("gaus", 10);




//  QcInfoLogger::GetInstance() << "ExampleTask monitorDataBlock (5x1s)" << AliceO2::InfoLogger::InfoLogger::endm;
//  sleep(2);
//  QcInfoLogger::GetInstance() << "   ." << AliceO2::InfoLogger::InfoLogger::endm;
//  sleep(1);
//  QcInfoLogger::GetInstance() << "   ." << AliceO2::InfoLogger::InfoLogger::endm;
//  sleep(1);
//  QcInfoLogger::GetInstance() << "   ." << AliceO2::InfoLogger::InfoLogger::endm;
//  sleep(1);
//  QcInfoLogger::GetInstance() << "   ." << AliceO2::InfoLogger::InfoLogger::endm;
//  sleep(1);
//  QcInfoLogger::GetInstance() << "   ." << AliceO2::InfoLogger::InfoLogger::endm;
}

void ExampleTask::endOfCycle()
{
  QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void ExampleTask::endOfActivity(Activity &activity)
{
  QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void ExampleTask::Reset()
{
  QcInfoLogger::GetInstance() << "Reset" << AliceO2::InfoLogger::InfoLogger::endm;
}

}
}
}
