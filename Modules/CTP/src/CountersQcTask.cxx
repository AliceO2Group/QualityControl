// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   CountersQcTask.cxx
/// \author Marek Bombara
///

#include <TCanvas.h>
#include <TPad.h>
#include <TH1.h>

#include "QualityControl/QcInfoLogger.h"
#include "CTP/CountersQcTask.h"
#include "DetectorsRaw/RDHUtils.h"
#include "Headers/RAWDataHeader.h"
#include "DPLUtils/DPLRawParser.h"
#include "DataFormatsCTP/Digits.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/DataRefUtils.h>
#include "CommonUtils/StringUtils.h"

namespace o2::quality_control_modules::ctp
{

CTPCountersTask::~CTPCountersTask()
{
  delete mHistogram;
  delete mInputCountsHist;
  delete mInputRateHist;
}

void CTPCountersTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Info, Support) << "initialize CountersQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  // if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
  //  ILOG(Info, Devel) << "Custom parameter - myOwnKey: " << param->second << ENDM;
  //}

  mHistogram = new TH1F("example", "example", 20, 0, 300);
  getObjectsManager()->startPublishing(mHistogram);

  mInputCountsHist = new TH1D("InputCountsFix", "Trigger Input Counts", 48, 0, 48);
  // gPad->SetLogy(mInputCountsHist->GetEntries()>0);
  getObjectsManager()->startPublishing(mInputCountsHist);

  mInputRateHist = new TH1D("InputRate", "Trigger Input Rate", 1, 0, 1);
  getObjectsManager()->startPublishing(mInputRateHist);

  {
    mTCanvasInputs = new TCanvas("inputsRates", "inputsRates", 2000, 2500);
    mTCanvasInputs->Clear();
    mTCanvasInputs->Divide(6, 8);

    for (size_t i = 0; i < 48; i++) {
      auto name = std::string("Rate_of_inp") + std::to_string(i);
      mHistInputRate[i] = new TH1D(name.c_str(), name.c_str(), 1, 0, 1);
      mHistInputRate[i]->GetXaxis()->SetTitle("1bin = 10sec");
      mHistInputRate[i]->GetYaxis()->SetTitle("Rate[Hz]");
      mTCanvasInputs->cd(i + 1);
      mHistInputRate[i]->Draw();
      mHistInputRate[i]->SetBit(TObject::kCanDelete);
    }
    getObjectsManager()->startPublishing(mTCanvasInputs);
  }

  {
    mTCanvasInputsTest = new TCanvas("inputsRatesTest", "inputsRatesTest", 1000, 1500);
    mTCanvasInputsTest->Clear();
    mTCanvasInputsTest->Divide(2, 2);

    for (size_t i = 0; i < 4; i++) {
      auto name = std::string("Rate_of_inp") + std::to_string(i);
      mHistInputRateTest[i] = new TH1D(name.c_str(), name.c_str(), 1, 0, 1);
      mHistInputRateTest[i]->GetXaxis()->SetTitle("1bin = 10sec");
      mHistInputRateTest[i]->GetYaxis()->SetTitle("Rate[Hz]");
      mTCanvasInputsTest->cd(i + 1);
      mHistInputRateTest[i]->Draw();
      mHistInputRateTest[i]->SetBit(TObject::kCanDelete);
    }
    getObjectsManager()->startPublishing(mTCanvasInputsTest);
  }
}

void CTPCountersTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity " << activity.mId << ENDM;
  mHistogram->Reset();
  mInputCountsHist->Reset();
  mInputRateHist->Reset();
  if (mTCanvasInputs) {
    for (const auto member : mHistInputRate) {
      if (member) {
        member->Reset();
      }
    }
  }
  if (mTCanvasInputsTest) {
    for (const auto member : mHistInputRateTest) {
      if (member) {
        member->Reset();
      }
    }
  }
}

void CTPCountersTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void CTPCountersTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // get the input
  o2::framework::InputRecord& inputs = ctx.inputs();
  o2::framework::DataRef ref = inputs.getByPos(1);
  // printf("Address of ref is %p\n", (void *)ref);
  // const char* ph = ref.header;
  // if (!ph) {LOG(info) << "no header pointer ";}
  const char* pp = ref.payload;
  if (!pp) {
    LOG(info) << "no payload pointer ";
  }
  // LOG(info) << "header pointer adress: " << ref.header;
  // LOG(info) << "payload pointer adress: " << pp;
  std::string spp = std::string(pp);
  // LOG(info) << "a teraz toto: " << spp;
  std::vector<double> counter;
  std::vector<std::string> tokens = o2::utils::Str::tokenize(spp, ' ');
  size_t ntokens = tokens.size();
  for (int i = 1; i < tokens.size(); i++) {
    // check if the received data are numbers
    std::string sCheck = tokens[i];
    bool notNumber = false;
    for (int j = 0; j < sCheck.length(); j++) {
      if (!isdigit(sCheck[j]))
        notNumber = true;
    }
    if (notNumber)
      continue;
    double tempCounter = std::stod(tokens[i]);
    counter.push_back(tempCounter);
  }

  double timeStamp = std::stod(tokens[0]);
  // LOG(info) << "Time stamp = " << timeStamp;
  // for(int i=0; i < counter.size(); i++) {
  // std::cout << "i = " << i << " " << counter.at(i) << std::endl;
  //}
  std::vector<double> trgInput;
  for (int i = 0; i < counter.size(); i++) {
    if ((i >= 599) && (i < 647)) {
      trgInput.push_back(counter[i]);
    }
    // std::cout << counter.at(i) << std::endl;
  }
  // filling input histograms
  for (int i = 0; i < trgInput.size(); i++) {
    double trgInputDouble = trgInput[i];
    mInputCountsHist->SetBinContent(i, trgInputDouble);
    // std::cout << trgInput[i] << " ";
  }
  mHistogram->Fill(5);
  double recentInput = 0.;
  double previousInput = 0.;
  double countDiff = 0.;
  double recentInputs[48] = { 0. };
  double previousInputs[48] = { 0. };
  double countDiffs[48] = { 0. };

  bool firstCycle = GetIsFirstCycle();
  if (firstCycle) {
    SetIsFirstCycle(false);
    SetFirstTimeStamp(timeStamp);
    SetPreviousTimeStamp(timeStamp);
    SetPreviousInput(trgInput[1]);
    for (int i = 0; i < 48; i++)
      mPreviousTrgInput.push_back(trgInput[i]);
    mTime.push_back(0.);
    mInputRate.push_back(0.);
    for (int i = 0; i < 48; i++)
      mTimes[i].push_back(0.);
    for (int i = 0; i < 48; i++)
      mInputRates[i].push_back(0.);
  } else {

    // get time from recent epoch timestamp
    std::time_t recentTime = timeStamp;
    struct tm* tmr = localtime(&recentTime);
    int rh = tmr->tm_hour;
    int rm = tmr->tm_min;
    int rs = tmr->tm_sec;
    // get time from previous epoch timestamp
    std::time_t previousTime = GetPreviousTimeStamp();
    struct tm* tmp = localtime(&previousTime);
    int ph = tmp->tm_hour;
    int pm = tmp->tm_min;
    int ps = tmp->tm_sec;

    // printf("Recent time: %02d:%02d:%02d\n", rh, rm, rs);
    // printf("Previous time: %02d:%02d:%02d\n", ph, pm, ps);

    // calculate rate
    recentInput = trgInput[1];
    previousInput = GetPreviousInput();
    countDiff = recentInput - previousInput; // should not be negative - integration values
    // std::cout << "recentinp: " << recentInput << " prevInp: " << previousInput << " diff: " << countDiff << std::endl;
    //  counters are 32 bit integers - when 40 MHz - very often overflow 2^32, in that case:
    for (int i = 0; i < 48; i++) {
      recentInputs[i] = trgInput[i];
      previousInputs[i] = mPreviousTrgInput[i];
      countDiffs[i] = recentInputs[i] - previousInputs[i]; // should not be negative - integration values
    }

    if (recentInput > previousInput) {
      int lastTimeIndex = mTime.size();
      double timeBefore = mTime[lastTimeIndex - 1];
      double timeNow = timeBefore + 10;
      mTime.push_back(timeNow);
      mInputRate.push_back(countDiff);
    }
    for (int i = 0; i < 48; i++) {
      if (recentInputs[i] > previousInputs[i]) {
        int lastTimeIndexs = mTimes[i].size();
        double timeBefores = mTimes[i][lastTimeIndexs - 1];
        double timeNows = timeBefores + 10;
        mTimes[i].push_back(timeNows);
        mInputRates[i].push_back(countDiffs[i]);
      }
    }
  }
  SetIsFirstCycle(false);
  SetPreviousTimeStamp(timeStamp);
  SetPreviousInput(trgInput.at(1));
  mPreviousTrgInput.clear();
  for (int i = 0; i < 48; i++) {
    mPreviousTrgInput.push_back(trgInput[i]);
    // std::cout << "i = " << i << " trgInput = " << trgInput[i] << " prevrate = " << mPreviousTrgInput[i] << std::endl;
  }

  // update rate histo
  std::time_t firstTime = GetFirstTimeStamp();
  struct tm* tmf = localtime(&firstTime);
  int fh = tmf->tm_hour;
  int fm = tmf->tm_min;
  int fs = tmf->tm_sec;
  if (!firstCycle) {
    // time in seconds
    int nBinsTime = mTime.size();
    double xMinTime = mTime[0];
    double xMaxTime = mTime[nBinsTime - 1];
    // std::cout << "nb = " << nBinsTime-1 << " xmin =" << xMinTime << " xmax =" << xMaxTime << std::endl;
    mInputRateHist->SetBins(nBinsTime - 1, xMinTime, xMaxTime);
    for (int i = 0; i < 48; i++) {
      mHistInputRate[i]->SetBins(nBinsTime - 1, xMinTime, xMaxTime);
      for (int j = 1; j < nBinsTime; j++) {
        double tempInpRate = mInputRates[i][j];
        // std::cout << "i = " << i << " j = " << j << " rate = " << tempInpRate << std::endl;
        mHistInputRate[i]->SetBinContent(j, tempInpRate);
      }
    }
    for (int i = 1; i < nBinsTime; i++) {
      // std::cout << "i = " << i << " rate = " << mInputRate[i] << std::endl;
      mInputRateHist->SetBinContent(i, mInputRate[i]);
    }
  }
}

void CTPCountersTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void CTPCountersTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void CTPCountersTask::reset()
{
  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  mHistogram->Reset();
  mInputCountsHist->Reset();
  mInputRateHist->Reset();
  if (mTCanvasInputs) {
    for (const auto member : mHistInputRate) {
      if (member) {
        member->Reset();
      }
    }
  }
  if (mTCanvasInputsTest) {
    for (const auto member : mHistInputRateTest) {
      if (member) {
        member->Reset();
      }
    }
  }
}

} // namespace o2::quality_control_modules::ctp
