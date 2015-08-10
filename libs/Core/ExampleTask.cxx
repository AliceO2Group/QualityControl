//
// Created by flpprotodev on 7/10/15.
//

#include "ExampleTask.h"
#include "QcInfoLogger.h"
#include <TH1.h>

#include <iostream>
#include <TCanvas.h>
#include <DataBlock.h>

using namespace std;

namespace AliceO2 {
namespace QualityControl {
namespace ExampleModule {

ExampleTask::ExampleTask(std::string name, Publisher *publisher) : TaskInterface(name, publisher)
{
}

ExampleTask::~ExampleTask()
{
}

void ExampleTask::initialize()
{
  QcInfoLogger::GetInstance() << "initialize" << AliceO2::InfoLogger::InfoLogger::endm;
  mHisto1 = new TH1F("first", "asdf", 2048, 0, 2047);
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
//  mHisto1->FillRandom("gaus", 10);
  uint32_t payloadSizeBytes =  block.header.dataSize/8;
  QcInfoLogger::GetInstance() << "Payload size " << payloadSizeBytes << AliceO2::InfoLogger::InfoLogger::endm;
  mHisto1->Fill(payloadSizeBytes);
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
  // just to see it
  TCanvas canvas;
  mHisto1->Draw();
  canvas.SaveAs("test.jpg");
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