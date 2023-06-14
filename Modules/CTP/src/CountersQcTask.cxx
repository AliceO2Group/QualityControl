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
#include "DataFormatsCTP/Configuration.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/DataRefUtils.h>
#include "CommonUtils/StringUtils.h"

namespace o2::quality_control_modules::ctp
{

CTPCountersTask::~CTPCountersTask()
{
  delete mDummyCountsHist;
  delete mInputCountsHist;
}

void CTPCountersTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize CountersQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  mInputCountsHist = new TH1D("TriggerInputCounts", "Total Trigger Input Counts", 48, 0, 48);
  //  gPad->SetLogy(mInputCountsHist->GetEntries()>0);
  getObjectsManager()->startPublishing(mInputCountsHist);

  {
    mTCanvasInputs = new TCanvas("inputsRates", "inputsRates", 2000, 2500);
    mTCanvasInputs->Clear();
    mTCanvasInputs->Divide(6, 8);

    for (size_t i = 0; i < 48; i++) {
      auto name = std::string("Rate_of_inp") + std::to_string(i);
      mHistInputRate[i] = new TH1D(name.c_str(), name.c_str(), 1, 0, 1);
      mHistInputRate[i]->GetXaxis()->SetTitle("Time");
      mHistInputRate[i]->GetYaxis()->SetTitle("Rate[Hz]");
      mTCanvasInputs->cd(i + 1);
      mHistInputRate[i]->Draw();
      mHistInputRate[i]->SetBit(TObject::kCanDelete);
    }
    getObjectsManager()->startPublishing(mTCanvasInputs);
  }

  {
    mTCanvasClasses = new TCanvas("classesRates", "classesRates", 2500, 2500);
    mTCanvasClasses->Clear();
    mTCanvasClasses->Divide(8, 8);

    for (size_t i = 0; i < 64; i++) {
      auto name = std::string("Rate_of_class") + std::to_string(i);
      mHistClassRate[i] = new TH1D(name.c_str(), name.c_str(), 1, 0, 1);
      mHistClassRate[i]->GetXaxis()->SetTitle("Time");
      mHistClassRate[i]->GetYaxis()->SetTitle("Rate[Hz]");
      mTCanvasClasses->cd(i + 1);
      mHistClassRate[i]->Draw();
      mHistClassRate[i]->SetBit(TObject::kCanDelete);
    }
    getObjectsManager()->startPublishing(mTCanvasClasses);
  }

  {
    for (int j = 0; j < 16; j++) {
      // auto name = std::string("Class rates in Run ") + std::to_string(mActiveRun[j]);
      auto name = std::string("Class_rates_in_Run_position_in_payload:") + std::to_string(j);
      mTCanvasClassRates[j] = new TCanvas(name.c_str(), name.c_str(), 2500, 2500);
      mTCanvasClassRates[j]->Clear();
      /*mTCanvasClassRates[j]->Divide(mXPad[j], mXPad[j]);

      for (size_t i = 0; i < mNumberOfClasses[j]; i++) {
        int k = mActiveClassInRun[i];
        auto name = std::string("Rate_of_class") + std::to_string(k);
        //mHistClassRate[k] = new TH1D(name.c_str(), name.c_str(), 1, 0, 1);
        //mHistClassRate[k]->GetXaxis()->SetTitle("Time");
        //mHistClassRate[k]->GetYaxis()->SetTitle("Rate[Hz]");
        mTCanvasClassRates[j]->cd(i + 1);
        mHistClassRate[k]->Draw();
        mHistClassRate[k]->SetBit(TObject::kCanDelete);
      }*/
      getObjectsManager()->startPublishing(mTCanvasClassRates[j]);
    }
  }

  {
    mTCanvasTotalCountsClasses = new TCanvas("TotalCountsClasses", "Total Counts Classes", 2000, 500);
    mTCanvasTotalCountsClasses->Clear();
    mTCanvasTotalCountsClasses->Divide(3, 2);

    for (size_t i = 0; i < 6; i++) {
      auto name = std::string("Rate_of_classnew") + std::to_string(i);
      if (i == 0) {
        name = std::string("Trigger Class LMb Total Time Integrated Counts");
      }
      if (i == 1) {
        name = std::string("Trigger Class L0b Total Time Integrated Counts");
      }
      if (i == 2) {
        name = std::string("Trigger Class L1b Total Time Integrated Counts");
      }
      if (i == 3) {
        name = std::string("Trigger Class LMa Total Time Integrated Counts");
      }
      if (i == 4) {
        name = std::string("Trigger Class L0a Total Time Integrated Counts");
      }
      if (i == 5) {
        name = std::string("Trigger Class L1a Total Time Integrated Counts");
      }
      mHistClassTotalCounts[i] = new TH1D(name.c_str(), name.c_str(), 64, 0, 64);
      mHistClassTotalCounts[i]->GetXaxis()->SetTitle("Class");
      mHistClassTotalCounts[i]->GetYaxis()->SetTitle("Total counts for run");
      mTCanvasTotalCountsClasses->cd(i + 1);
      mHistClassTotalCounts[i]->Draw();
      mHistClassTotalCounts[i]->SetBit(TObject::kCanDelete);
    }
    getObjectsManager()->startPublishing(mTCanvasTotalCountsClasses);
  }
}

void CTPCountersTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "Start of all activitites " << ENDM;
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
  mInputCountsHist->Reset();
  // mInputRateHist->Reset();
  // mClassCountsHist->Reset();
  if (mTCanvasInputs) {
    for (const auto member : mHistInputRate) {
      if (member) {
        member->Reset();
      }
    }
  }
  if (mTCanvasClasses) {
    for (const auto member : mHistClassRate) {
      if (member) {
        member->Reset();
      }
    }
  }
  if (mTCanvasTotalCountsClasses) {
    for (const auto member : mHistClassTotalCounts) {
      if (member) {
        member->Reset();
      }
    }
  }
  /*
  for (int j = 0; j < 16; j++) {
    if (mTCanvasClassRates[j]) {
      for (const auto member : mHistClassRate) {
        if (member) {
          member->Reset();
        }
      }
    }
  }
  */
}

void CTPCountersTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void CTPCountersTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // dummy publishing
  // mDummyCountsHist = new TH1D("DummyCounts", "Dummy counts", 1, 0, 1);
  // getObjectsManager()->startPublishing(mDummyCountsHist);
  //  get the input
  // std::cout << "before data reading ";
  o2::framework::InputRecord& inputs = ctx.inputs();
  o2::framework::DataRef ref = inputs.get("readout");

  const char* pp = ref.payload;
  if (!pp) {
    LOG(info) << "no payload pointer ";
  }
  // pyaload has 4 formats:
  // ctpconfig with rcfg, sox with counters+rcfg, pcp with counters and eox with counters
  std::string spp = std::string(pp);
  // LOG(info) << "the payload message: " << spp;
  // std::cout << "the cout payload message: " << spp;
  std::vector<double> counter;
  // fill the tokens with counters or rcfg
  std::vector<std::string> tokens = o2::utils::Str::tokenize(spp, ' ');

  // ctpconfig - add classes to newly loaded run
  if (tokens[0] == "ctpconfig") {
    LOG(info) << "CTP run configuration:";
    // we have to get a rid of the first substring - ctpconfig to gain rcfg message
    std::string subspp = "ctpconfig ";
    std::string::size_type posInSpp = spp.find(subspp);
    if (posInSpp != std::string::npos)
      spp.erase(posInSpp, subspp.length());
    std::string ctpConf = spp;
    // LOG(info) << "Rcfg message:";
    LOG(info) << ctpConf;
    // get Trigger Class Mask for the run from the CTP configuration
    o2::ctp::CTPConfiguration activeConf;
    activeConf.loadConfigurationRun3(ctpConf);
    activeConf.printStream(std::cout);
    uint64_t runClassMask = activeConf.getTriggerClassMask();
    LOG(info) << "Class Mask Qc: " << runClassMask;
    std::vector<int> runClassList = activeConf.getTriggerClassList();
    std::cout << "size of runClassList: " << runClassList.size() << std::endl;
    for (auto i : runClassList) {
      std::cout << " print class list: " << i << " ";
    }
    uint32_t runNumber = activeConf.getRunNumber();
    // runCTPQC mNewRun;
    mNewRun.mRunNumber = runNumber;
    mNewRun.mRunClasses = runClassList;
  }

  if (tokens[0] == "sox") {
    for (int i = 2; i < 1040; i++) {
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
    for (int i = 0; i < 16; i++) {
      int isNewRun = counter[i] - mPreviousRunNumbers[i];
      if (isNewRun != 0) {
        std::cout << "we have a new run!" << std::endl;
        mNewRun.mPositionInCounters = i;
        // auto name = std::string("Class Rates For Run ") + std::to_string(mNewRun.mRunNumber);
        // mTCanvasClassRates[i] = new TCanvas(name.c_str(), name.c_str(), 2500, 2500);
        // mTCanvasClassRates[i]->Clear();
        // mTCanvasClassRates[i]->SetTitle(name.c_str());
        int numberOfClasses = mNewRun.mRunClasses.size();
        double numOfCl = numberOfClasses;
        double xpad = std::ceil(sqrt(numOfCl));
        mTCanvasClassRates[i]->Divide(xpad, xpad);

        for (size_t j = 0; j < numberOfClasses; j++) {
          int k = mNewRun.mRunClasses[j];
          auto name = std::string("Run ") + std::to_string(mNewRun.mRunNumber) + std::string(" Rate_of_class") + std::to_string(k);
          mTCanvasClassRates[i]->cd(j + 1);
          mHistClassRate[k]->SetTitle(name.c_str());
          mHistClassRate[k]->Draw();
          mHistClassRate[k]->SetBit(TObject::kCanDelete);
        }
        // std::cout << "before publishing" << std::endl;
        // LOG(info) << "before publishing with LOG";
        // getObjectsManager()->startPublishing(mTCanvasClassRates[i]);
        // std::cout << "after publishing" << std::endl;
      }
      mPreviousRunNumbers.clear();
      mPreviousRunNumbers.push_back(counter[i]);
      // LOG(info) << "Recent Runs = " << runNumbers[i] << " ";
    }
    mPreviousRunNumbers.clear();
    for (int i = 0; i < 16; i++) {
      mPreviousRunNumbers.push_back(counter[i]);
      // LOG(info) << "Recent Runs = " << runNumbers[i] << " ";
    }
  }

  if (tokens[0] == "eox") {
    for (int i = 0; i < 16; i++) {
      for (int i = 2; i < 1040; i++) {
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
      int wasNewRun = counter[i] - mPreviousRunNumbers[i];
      if (wasNewRun != 0) {
        // std::cout << "before stopping publishing" << std::endl;
        // getObjectsManager()->stopPublishing(mTCanvasClassRates[i]);
        // std::cout << "after stopping publishing and before deleting" << std::endl;
        // delete mTCanvasClassRates[i];
        // std::cout << "after deleting" << std::endl;
      }
    }
  }

  if (tokens[0] == "pcp") {
    // for (int i = 2; i < tokens.size(); i++) {
    for (int i = 2; i < 1040; i++) {
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

    LOG(info) << "The topic is = " << tokens[0];

    double timeStamp = std::stod(tokens[1]);
    // LOG(info) << "Time stamp = " << timeStamp;
    for (int i = 0; i < counter.size(); i++) {
      // std::cout << "i = " << i << " " << counter.at(i) << std::endl;
    }
    int RunNumber = counter[0];
    std::vector<int> runNumbers;
    mPreviousRunNumbers.clear();
    for (int i = 0; i < 16; i++) {
      runNumbers.push_back(counter[i]);
      mPreviousRunNumbers.push_back(counter[i]);
      // LOG(info) << "Recent Runs = " << runNumbers[i] << " ";
    }

    // 48 trigger inputs
    std::vector<double> trgInput;
    // 64 trigger classes after L1
    std::vector<double> trgClass;
    std::vector<double> trgClassMb;
    std::vector<double> trgClassMa;
    std::vector<double> trgClass0b;
    std::vector<double> trgClass0a;
    std::vector<double> trgClass1b;
    std::vector<double> trgClass1a;
    for (int i = 0; i < counter.size(); i++) {
      if ((i >= 599) && (i < 647))
        trgInput.push_back(counter[i]);
      if ((i >= 967) && (i < 1031))
        trgClass.push_back(counter[i]);

      if ((i >= 647) && (i < 711))
        trgClassMb.push_back(counter[i]);
      if ((i >= 711) && (i < 775))
        trgClassMa.push_back(counter[i]);
      if ((i >= 775) && (i < 839))
        trgClass0b.push_back(counter[i]);
      if ((i >= 839) && (i < 903))
        trgClass0a.push_back(counter[i]);
      if ((i >= 903) && (i < 967))
        trgClass1b.push_back(counter[i]);
      if ((i >= 967) && (i < 1031))
        trgClass1a.push_back(counter[i]);
      // std::cout << counter.at(i) << std::endl;
    }
    // filling input histograms
    for (int i = 0; i < trgInput.size(); i++) {
      double trgInputDouble = trgInput[i];
      mInputCountsHist->SetBinContent(i, trgInputDouble);
      // std::cout << trgInput[i] << " ";
    }
    for (int i = 0; i < trgClassMb.size(); i++) {
      double trgClassDouble = trgClassMb[i];
      mHistClassTotalCounts[0]->SetBinContent(i, trgClassDouble);
    }
    for (int i = 0; i < trgClassMa.size(); i++) {
      double trgClassDouble = trgClassMa[i];
      mHistClassTotalCounts[3]->SetBinContent(i, trgClassDouble);
    }
    for (int i = 0; i < trgClass0b.size(); i++) {
      double trgClassDouble = trgClass0b[i];
      mHistClassTotalCounts[1]->SetBinContent(i, trgClassDouble);
    }
    for (int i = 0; i < trgClass0a.size(); i++) {
      double trgClassDouble = trgClass0a[i];
      mHistClassTotalCounts[4]->SetBinContent(i, trgClassDouble);
    }
    for (int i = 0; i < trgClass1b.size(); i++) {
      double trgClassDouble = trgClass1b[i];
      mHistClassTotalCounts[2]->SetBinContent(i, trgClassDouble);
    }
    for (int i = 0; i < trgClass1a.size(); i++) {
      double trgClassDouble = trgClass1a[i];
      mHistClassTotalCounts[5]->SetBinContent(i, trgClassDouble);
    }

    double recentInput = 0.;
    double previousInput = 0.;
    double countDiff = 0.;
    double recentInputs[48] = { 0. };
    double previousInputs[48] = { 0. };
    double countInputDiffs[48] = { 0. };
    double recentClasses[64] = { 0. };
    double previousClasses[64] = { 0. };
    double countClassDiffs[64] = { 0. };
    // int previousRunNumbers[16] = {0};

    bool firstCycle = IsFirstCycle();
    if (firstCycle) {
      SetIsFirstCycle(false);
      SetFirstTimeStamp(timeStamp);
      SetPreviousTimeStamp(timeStamp);
      // SetPreviousInput(trgInput[1]);
      for (int i = 0; i < 48; i++)
        mPreviousTrgInput.push_back(trgInput[i]);
      // mTime.push_back(0.);
      mTime.push_back(timeStamp);
      // mInputRate.push_back(0.);
      for (int i = 0; i < 48; i++)
        mTimes[i].push_back(0.);
      for (int i = 0; i < 48; i++)
        mInputRates[i].push_back(0.);
      // class part
      // SetPreviousClass(trgClass[1]);
      for (int i = 0; i < 64; i++)
        mPreviousTrgClass.push_back(trgClass[i]);
      for (int i = 0; i < 64; i++)
        mClassRates[i].push_back(0.);
      for (int i = 0; i < 16; i++)
        mPreviousRunNumbers.push_back(runNumbers[i]);
    } else {

      for (int i = 0; i < 48; i++) {
        recentInputs[i] = trgInput[i];
        previousInputs[i] = mPreviousTrgInput[i];
        countInputDiffs[i] = recentInputs[i] - previousInputs[i]; // should not be negative - integration values
        mInputRates[i].push_back(countInputDiffs[i]);
      }

      for (int i = 0; i < 64; i++) {
        recentClasses[i] = trgClass[i];
        previousClasses[i] = mPreviousTrgClass[i];
        countClassDiffs[i] = recentClasses[i] - previousClasses[i]; // should not be negative - integration values
        mClassRates[i].push_back(countClassDiffs[i]);
      }

      if (recentInputs[0] > previousInputs[0]) {
        mTime.push_back(timeStamp);
      }
    }
    SetIsFirstCycle(false);
    SetPreviousTimeStamp(timeStamp);
    mPreviousTrgInput.clear();
    for (int i = 0; i < 48; i++) {
      mPreviousTrgInput.push_back(trgInput[i]);
      // std::cout << "i = " << i << " trgInput = " << trgInput[i] << " prevrate = " << mPreviousTrgInput[i] << std::endl;
    }
    mPreviousTrgClass.clear();
    for (int i = 0; i < 64; i++) {
      mPreviousTrgClass.push_back(trgClass[i]);
      // std::cout << "i = " << i << " trgInput = " << trgInput[i] << " prevrate = " << mPreviousTrgInput[i] << std::endl;
    }
    mPreviousRunNumbers.clear();
    for (int i = 0; i < 16; i++) {
      mPreviousRunNumbers.push_back(runNumbers[i]);
      // std::cout << "i = " << i << " trgInput = " << trgInput[i] << " prevrate = " << mPreviousTrgInput[i] << std::endl;
    }
    if (!firstCycle) {
      // time in seconds
      int nBinsTime = mTime.size();
      double xMinTime = mTime[0];
      double xMaxTime = mTime[nBinsTime - 1];
      // std::cout << "nb = " << nBinsTime-1 << " xmin =" << xMinTime << " xmax =" << xMaxTime << std::endl;
      for (int i = 0; i < 48; i++) {
        mHistInputRate[i]->SetBins(nBinsTime - 1, 0, xMaxTime - xMinTime);
        for (int j = 1; j < nBinsTime; j++) {
          double tempInpRate = mInputRates[i][j];
          // std::cout << "i = " << i << " j = " << j << " rate = " << tempInpRate << " nbins = " << nBinsTime << std::endl;
          mHistInputRate[i]->SetBinContent(j, tempInpRate);
          SetRateHisto(mHistInputRate[i], xMinTime);
        }
      }
      // std::cout << "class rate histos.." << std::endl;
      for (int i = 0; i < 64; i++) {
        mHistClassRate[i]->SetBins(nBinsTime - 1, 0, xMaxTime - xMinTime);
        for (int j = 1; j < nBinsTime; j++) {
          double tempClassRate = mClassRates[i][j];
          // std::cout << "i = " << i << " j = " << j  << " rate = " << tempClassRate << std::endl;
          mHistClassRate[i]->SetBinContent(j, tempClassRate);
          SetRateHisto(mHistClassRate[i], xMinTime);
        }
      }
    }
  }
}

void CTPCountersTask::endOfCycle()
{
  std::cout << "End of Cycle" << std::endl;
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void CTPCountersTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void CTPCountersTask::reset()
{
  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  mInputCountsHist->Reset();
  // mDummyCountsHist->Reset();
  // mClassCountsHist->Reset();
  if (mTCanvasInputs) {
    for (const auto member : mHistInputRate) {
      if (member) {
        member->Reset();
      }
    }
  }
  if (mTCanvasClasses) {
    for (const auto member : mHistClassRate) {
      if (member) {
        member->Reset();
      }
    }
  }
  if (mTCanvasTotalCountsClasses) {
    for (const auto member : mHistClassTotalCounts) {
      if (member) {
        member->Reset();
      }
    }
  }
}

/*void SetRateHisto(TH1D* h, double timeOffset)
{
  h->GetXaxis()->SetTimeOffset(timeOffset);
  h->GetXaxis()->SetTimeDisplay(1);
  h->GetXaxis()->SetTimeFormat("%H:%M");
  h->GetXaxis()->SetNdivisions(808);
}
*/
} // namespace o2::quality_control_modules::ctp
