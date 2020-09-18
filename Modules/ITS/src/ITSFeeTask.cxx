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
///

#include "ITS/ITSFeeTask.h"
#include "QualityControl/QcInfoLogger.h"

#include <ITSMFTReconstruction/GBTWord.h>

#include <DPLUtils/RawParser.h>
#include <DPLUtils/DPLRawParser.h>
#include <TCanvas.h>
#include <TDatime.h>
#include <TGraph.h>
#include <TH1.h>
#include <TPaveText.h>
#include <TPaveStats.h>

#include <time.h>

using namespace std;

namespace o2::quality_control_modules::its
{

ITSFeeTask::ITSFeeTask()
  : TaskInterface()
{
  o2::base::GeometryManager::loadGeometry();
  mGeom = o2::its::GeometryTGeo::Instance();
}

ITSFeeTask::~ITSFeeTask()
{
  delete mTFInfo;
  delete mTriggerPlots;
  delete mTriggerVsFeeid;
  //delete mInfoCanvas;

  delete mGeom;
}

void ITSFeeTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  QcInfoLogger::GetInstance() << "initialize ITSFeeTask" << AliceO2::InfoLogger::InfoLogger::endm;
  getEnableLayers();
  int barrel = 0;
  if (mEnableLayers[0] or mEnableLayers[1] or mEnableLayers[2]) {
    barrel += 1;
  }
  if (mEnableLayers[3] or mEnableLayers[4] or mEnableLayers[5] or mEnableLayers[6]) {
    barrel += 2;
  }
  createGeneralPlots(barrel);
  setPlotsFormat();
}

void ITSFeeTask::createErrorTriggerPlots()
{

  mTriggerPlots = new TH1D("General/TriggerPlots", "Decoding Triggers", mNTrigger, 0.5, mNTrigger + 0.5);
  mTriggerPlots->SetMinimum(0);
  mTriggerPlots->SetFillColor(kBlue);
  getObjectsManager()->startPublishing(mTriggerPlots); //mTriggerPlots
}

void ITSFeeTask::createGeneralPlots(int barrel = 0)
{


  createErrorTriggerPlots();

  mTFInfo = new TH1F("General/TFInfo", "TF vs count", 15000, 0, 15000);
  getObjectsManager()->startPublishing(mTFInfo); //mTFInfo

  if (barrel == 0) { //No Need to create
    QcInfoLogger::GetInstance() << "No General Plots need to create, Please check you config file of QC" << AliceO2::InfoLogger::InfoLogger::endm;
  } else if (barrel == 1) { //create for IB
    mTriggerVsFeeid = new TH2I("General/TriggerVsFeeid", "Trigger count vs Trigger id and Fee id", 3 * StaveBoundary[3], 0, 3 * StaveBoundary[3], mNTrigger, 0.5, mNTrigger + 0.5);
    mTriggerVsFeeid->SetMinimum(0);
    mTriggerVsFeeid->SetStats(0);
    getObjectsManager()->startPublishing(mTriggerVsFeeid);
  } else if (barrel == 2) { //create for OB		TODO:create the Error/Trigger vs feeid plots
    mTriggerVsFeeid = new TH2I("General/TriggerVsFeeid", "Trigger count vs Trigger id and Fee id", 3 * StaveBoundary[3], 0, 3 * StaveBoundary[3], mNTrigger, 0.5, mNTrigger + 0.5);
    mTriggerVsFeeid->SetMinimum(0);
    mTriggerVsFeeid->SetStats(0); //create for OB		TODO: now I am just copy the code from IB to make it will not crash
    getObjectsManager()->startPublishing(mTriggerVsFeeid);

  } else if (barrel == 3) { //create for All	TODO:create the Error/Trigger vs feeid plots
  }
}


void ITSFeeTask::setAxisTitle(TH1* object, const char* xTitle, const char* yTitle)
{
  object->GetXaxis()->SetTitle(xTitle);
  object->GetYaxis()->SetTitle(yTitle);
}

void ITSFeeTask::setPlotsFormat()
{
  //set general plots format
  if (mTriggerPlots) {
    setAxisTitle(mTriggerPlots, "Trigger ID", "Counts");
    for (int i = 0; i < mNTrigger; i++) {
      mTriggerPlots->GetXaxis()->SetBinLabel(i + 1, mTriggerType[i]);
    }
  }
  if (mTFInfo) {
    setAxisTitle(mTFInfo, "TF ID", "Counts");
  }
  if (mTriggerVsFeeid) {
    setAxisTitle(mTriggerVsFeeid, "FeeID", "Trigger ID");
    for (int i = 0; i < mNTrigger; i++) {
      mTriggerVsFeeid->GetYaxis()->SetBinLabel(i + 1, mTriggerType[i]);
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
  // in a loop
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  std::chrono::time_point<std::chrono::high_resolution_clock> end;
  int difference;
  start = std::chrono::high_resolution_clock::now();



  //Fill the Trigger plots
  DPLRawParser parser(ctx.inputs());
  int StaveStart = 0;
  for (int i = 0; !mEnableLayers[i]; i++) {
    StaveStart = StaveBoundary[i + 1];
  }
  for (auto it = parser.begin(), end = parser.end(); it != end; ++it) {
    auto const* rdh = it.get_if<o2::header::RAWDataHeaderV6>();
    int istave = (int)(rdh->feeId & 0x00ff);
    int ilink = (int)((rdh->feeId & 0x0f00) >> 8);
    istave += StaveStart;

    int stopbit = (int)(rdh->stop);
    std::cout << "stopbit is: " << stopbit  << std::endl;
        
    auto const* ddw = reinterpret_cast<const o2::itsmft::GBTDataTrailer*>(rdh)+4;
    ddw->printX();    

    for (int i = 0; i < 13; i++) {
      if (((uint32_t)(rdh->triggerType) >> i & 1) == 1) {
        mTriggerPlots->Fill(i + 1);
        mTriggerVsFeeid->Fill((istave * 3) + ilink, i + 1);
      }
    }
  }
  //Fill the Trigger plots end



  //get the Time Frame ID
  mTimeFrameId = ctx.inputs().get<int>("G");
  //get the Time Frame ID end

  mTFInfo->Fill(mTimeFrameId);
  end = std::chrono::high_resolution_clock::now();
  difference = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  ILOG(Info) << "time until thread all end is " << difference << ", and TF ID == " << mTimeFrameId << ENDM;
}

void ITSFeeTask::getEnableLayers()
{
  std::ifstream configFile("Config/ConfigFakeRateFee.dat");
  for (int ilayer = 0; ilayer < NLayer; ilayer++) {
    configFile >> mEnableLayers[ilayer];
    if (mEnableLayers[ilayer]) {
      ILOG(Info) << "enable layer : " << ilayer << ENDM;
    }
  }
  configFile >> mRunNumberPath;
}

void ITSFeeTask::endOfCycle()
{
  std::ifstream runNumberFile("/home/its/QC_Online/workdir/infiles/RunNumber.dat"); //catching ITS run number in commissioning
  if (runNumberFile) {
    string runNumber;
    runNumberFile >> runNumber;
    ILOG(Info) << "runNumber : " << runNumber << ENDM;
    if (runNumber == mRunNumber) {
      goto pass;
    }
    getObjectsManager()->addMetadata(mTFInfo->GetName(), "Run", runNumber);
    getObjectsManager()->addMetadata(mTriggerVsFeeid->GetName(), "Run", runNumber);
    getObjectsManager()->addMetadata(mTriggerPlots->GetName(), "Run", runNumber);
    mRunNumber = runNumber;
  pass:;
  }
  ILOG(Info) << "endOfCycle" << ENDM;
}

void ITSFeeTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "endOfActivity" << ENDM;
}

void ITSFeeTask::resetGeneralPlots()
{
  mTFInfo->Reset();
  mTriggerVsFeeid->Reset();
  mTriggerPlots->Reset();
}
void ITSFeeTask::reset()
{
  resetGeneralPlots();
  ILOG(Info) << "Reset" << ENDM;
}

} // namespace o2::quality_control_modules::its
