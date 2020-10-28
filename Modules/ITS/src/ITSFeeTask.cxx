// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   ITSFeeTask.cxx
/// \author Jian Liu
/// \author Liang Zhang
///

#include "ITS/ITSFeeTask.h"
#include "QualityControl/QcInfoLogger.h"

#include <DPLUtils/RawParser.h>
#include <DPLUtils/DPLRawParser.h>

using namespace o2::framework;
using namespace o2::header;

namespace o2::quality_control_modules::its
{

ITSFeeTask::ITSFeeTask()
  : TaskInterface()
{
}

ITSFeeTask::~ITSFeeTask()
{
  delete mTFInfo;
  delete mErrorFlag;
  delete mErrorFlagVsFeeId;
  //delete mInfoCanvas;
}

void ITSFeeTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  QcInfoLogger::GetInstance() << "Initializing the ITSFeeTask" << AliceO2::InfoLogger::InfoLogger::endm;
  getEnableLayers();
  getRunNumber();
  int barrel = 0;
  if (mEnableLayers[0] or mEnableLayers[1] or mEnableLayers[2]) {
    barrel += 1;
  }
  if (mEnableLayers[3] or mEnableLayers[4] or mEnableLayers[5] or mEnableLayers[6]) {
    barrel += 2;
  }
  createErrorTFPlots(barrel);
  setPlotsFormat();
}

void ITSFeeTask::createErrorTFPlots(int barrel)
{
  mErrorFlag = new TH1I("ErrorTF/ErrorFlag", "FEE Decoding Errors", mNTrigger, 0.5, mNTrigger + 0.5);
  mErrorFlag->SetMinimum(0);
  mErrorFlag->SetFillColor(kBlue);
  getObjectsManager()->startPublishing(mErrorFlag); //mErrorFlag

  mTFInfo = new TH1I("ErrorTF/TFInfo", "TF vs count", 15000, 0, 15000);
  getObjectsManager()->startPublishing(mTFInfo); //mTFInfo

  if (barrel == 0) { //No Need to create
    QcInfoLogger::GetInstance() << "No Error/TF Plots need to create, Please check you config file of QC" << AliceO2::InfoLogger::InfoLogger::endm;
  } else if (barrel == 1) { //create for IB
    mErrorFlagVsFeeId = new TH2I("ErrorTF/ErrorFlagVsFeeid", "Error count vs Error id and Fee id", 3 * StaveBoundary[3], 0, 3 * StaveBoundary[3], mNTrigger, 0.5, mNTrigger + 0.5);
    mErrorFlagVsFeeId->SetMinimum(0);
    mErrorFlagVsFeeId->SetStats(0);
    getObjectsManager()->startPublishing(mErrorFlagVsFeeId);
  } else if (barrel == 2) { //create for OB
    mErrorFlagVsFeeId = new TH2I("ErrorTF/ErrorFlagVsFeeid", "Error count vs Error id and Fee id", 3 * (StaveBoundary[7] - StaveBoundary[3]), 0, 3 * (StaveBoundary[7] - StaveBoundary[3]), mNTrigger, 0.5, mNTrigger + 0.5);
    mErrorFlagVsFeeId->SetMinimum(0);
    mErrorFlagVsFeeId->SetStats(0); //create for OB
    getObjectsManager()->startPublishing(mErrorFlagVsFeeId);
  }
}

void ITSFeeTask::setAxisTitle(TH1* object, const char* xTitle, const char* yTitle)
{
  object->GetXaxis()->SetTitle(xTitle);
  object->GetYaxis()->SetTitle(yTitle);
}

void ITSFeeTask::setPlotsFormat()
{
  if (mErrorFlag) {
    setAxisTitle(mErrorFlag, "Error Flag ID", "Counts");
    for (int i = 0; i < mNTrigger; i++) {
      mErrorFlag->GetXaxis()->SetBinLabel(i + 1, mErrorType[i]);
    }
  }
  if (mTFInfo) {
    setAxisTitle(mTFInfo, "TF ID", "Counts");
  }
  if (mErrorFlagVsFeeId) {
    setAxisTitle(mErrorFlagVsFeeId, "FeeID", "Error Flag ID");
    for (int i = 0; i < mNTrigger; i++) {
      mErrorFlagVsFeeId->GetYaxis()->SetBinLabel(i + 1, mErrorType[i]);
    }
  }
}

void ITSFeeTask::startOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void ITSFeeTask::startOfCycle() { QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm; }

void ITSFeeTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  std::chrono::time_point<std::chrono::high_resolution_clock> end;
  int difference;
  start = std::chrono::high_resolution_clock::now();

  DPLRawParser parser(ctx.inputs());
  int staveStart = 0;
  for (int i = 0; !mEnableLayers[i]; i++) {
    staveStart = StaveBoundary[i + 1];
  }
  for (auto it = parser.begin(), end = parser.end(); it != end; ++it) {
    auto const* rdh = it.get_if<o2::header::RAWDataHeaderV6>();
    int istave = (int)(rdh->feeId & 0x00ff);
    int ilink = (int)((rdh->feeId & 0x0f00) >> 8);
    istave += staveStart;

    if ((int)(rdh->stop) && it.size()) { //looking into the Stop RDH
      ILOG(Info) << "Stopbit : " << (int)rdh->stop << ", Payload size: " << it.size() << ENDM;
      auto const* ddw = reinterpret_cast<const GBTDdw*>(it.data());
      LOGF(INFO, "DDW header 0x: %02x", ddw->ddwBits.data8[9]);
    }
    for (int i = 0; i < 13; i++) { //filling trigger types to debug the program for the moment
      if (((uint32_t)(rdh->triggerType) >> i & 1) == 1) {
        mErrorFlag->Fill(i + 1);
        mErrorFlagVsFeeId->Fill((istave * 3) + ilink, i + 1);
      }
    }
  }

  mTimeFrameId = ctx.inputs().get<int>("G");

  mTFInfo->Fill(mTimeFrameId);
  end = std::chrono::high_resolution_clock::now();
  difference = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  ILOG(Info) << "Processing time: " << difference << ", and TF ID == " << mTimeFrameId << ENDM;
}

void ITSFeeTask::getEnableLayers()
{
  std::ifstream configFile("Config/ConfigFakeRateFee.dat");
  for (int ilayer = 0; ilayer < NLayer; ilayer++) {
    configFile >> mEnableLayers[ilayer];
    if (mEnableLayers[ilayer]) {
      ILOG(Info) << "Enable layer : " << ilayer << ENDM;
    }
  }
  configFile >> mRunNumberPath;
}

void ITSFeeTask::getRunNumber()
{
  std::ifstream runNumberFile(mRunNumberPath);
  if (runNumberFile) {
    mRunNumber = "";
    runNumberFile >> mRunNumber;
    ILOG(Info) << "runNumber : " << mRunNumber << ENDM;
  }
}

void ITSFeeTask::endOfCycle()
{
  getObjectsManager()->addMetadata(mTFInfo->GetName(), "Run", mRunNumber);
  getObjectsManager()->addMetadata(mErrorFlagVsFeeId->GetName(), "Run", mRunNumber);
  getObjectsManager()->addMetadata(mErrorFlag->GetName(), "Run", mRunNumber);
  ILOG(Info) << "endOfCycle" << ENDM;
}

void ITSFeeTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "endOfActivity" << ENDM;
}

void ITSFeeTask::resetGeneralPlots()
{
  mTFInfo->Reset();
  mErrorFlagVsFeeId->Reset();
  mErrorFlag->Reset();
}
void ITSFeeTask::reset()
{
  resetGeneralPlots();
  ILOG(Info) << "Reset" << ENDM;
}

} // namespace o2::quality_control_modules::its
