///
/// \file   DaqTask.cxx
/// \author Barthelemy von Haller
///

#include "Daq/DaqTask.h"

#include <TH1.h>
#include <TCanvas.h>
#include <TDatime.h>
#include <TStyle.h>
#include <TGraph.h>
#include "QualityControl/QcInfoLogger.h"

using namespace std;

namespace AliceO2 {
namespace QualityControlModules {
namespace Daq {

DaqTask::DaqTask()
  : TaskInterface(),
    mPayloadSize(nullptr),
    mIds(nullptr),
    fNPoints(0),
    mNumberSubblocks(nullptr),
    mSubPayloadSize(nullptr)
{
}

DaqTask::~DaqTask()
{
  delete mPayloadSize;
  delete mIds;
  delete mNumberSubblocks;
  delete mSubPayloadSize;
}

void DaqTask::initialize()
{
  QcInfoLogger::GetInstance() << "initialize DaqTask" << AliceO2::InfoLogger::InfoLogger::endm;

  mPayloadSize = new TH1F("payloadSize", "Payload size of blocks;bytes", 2048, 0, 2047);
  mPayloadSize->SetCanExtend(TH1::kXaxis);
  getObjectsManager()->startPublishing(mPayloadSize);
  getObjectsManager()->addCheck(mPayloadSize, "checkNonEmpty", "AliceO2::QualityControlModules::Common::NonEmpty",
                                "QcCommon");
  mNumberSubblocks = new TH1F("numberSubBlocks", "Number of subblocks", 100, 1, 100);
  getObjectsManager()->startPublishing(mNumberSubblocks);
  mSubPayloadSize = new TH1F("PayloadSizeSubBlocks", "Payload size of subblocks;bytes", 2048, 0, 2047);
  mSubPayloadSize->SetCanExtend(TH1::kXaxis);
  getObjectsManager()->startPublishing(mSubPayloadSize);

  mIds = new TGraph();
  mIds->SetName("IDs");
  mIds->GetXaxis()->SetTimeDisplay(1);
  mIds->GetXaxis()->SetNdivisions(-503);
  mIds->GetXaxis()->SetTimeFormat("%Y-%m-%d %H:%M:%S");
  mIds->GetXaxis()->SetTimeOffset(0, "gmt");
  mIds->SetMarkerStyle(20);
  mIds->GetXaxis()->SetLabelSize(0.02);
  mIds->GetYaxis()->SetLabelSize(0.02);
  getObjectsManager()->startPublishing(mIds);
  getObjectsManager()->addCheck(mIds, "checkIncreasingIDs", "AliceO2::QualityControlModules::Daq::EverIncreasingGraph",
                                "QcDaq");
  getObjectsManager()->addCheck(mIds, "checkNonEmpty", "AliceO2::QualityControlModules::Common::NonEmpty", "QcCommon");
}

void DaqTask::startOfActivity(Activity &activity)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
  mPayloadSize->Reset();
  mNumberSubblocks->Reset();
  mSubPayloadSize->Reset();
  mIds->Set(0);
  fNPoints = 0;
}

void DaqTask::startOfCycle()
{
  QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void DaqTask::monitorDataBlock(DataSetReference dataSet)
{
  if (dataSet->size() <= 0) {
    cout << "Empty vector!" << endl;
    return;
  }

  uint32_t totalPayloadSize = 0;
  for (auto b : *dataSet) {
    uint32_t size = b->getData()->header.dataSize / 8;
    mSubPayloadSize->Fill(size);
    totalPayloadSize += size;
  }

  mPayloadSize->Fill(totalPayloadSize);
  mNumberSubblocks->Fill(dataSet->size());

  if (dataSet->at(0) == nullptr) {
    cout << "Container pointer invalid" << endl;
    return;
  }
  TDatime now;
  if ((now.Get() - mTimeLastRecord) >= 1) {
    mIds->SetPoint(fNPoints, now.Convert(), dataSet->at(0)->getData()->header.id);
    fNPoints++;
    mTimeLastRecord = now.Get();
  }
}

void DaqTask::endOfCycle()
{
  QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void DaqTask::endOfActivity(Activity &activity)
{
  QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void DaqTask::reset()
{
  QcInfoLogger::GetInstance() << "Reset" << AliceO2::InfoLogger::InfoLogger::endm;
}

}
}
}
