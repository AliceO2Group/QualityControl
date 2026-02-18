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
/// \file   RawDataQcTask.cxx
/// \author Marek Bombara
/// \author Lucia Anna Tarasovicova
/// \author Jan Musinsky
/// \date   2026-02-17
///

#include <TCanvas.h>
#include <TLine.h>
#include <TH1.h>

#include "QualityControl/QcInfoLogger.h"
#include "CTP/RawDataQcTask.h"
#include <DataFormatsParameters/GRPLHCIFData.h>
#include <DetectorsBase/GRPGeomHelper.h>
#include "DetectorsRaw/RDHUtils.h"
#include "Headers/RAWDataHeader.h"
#include "DataFormatsCTP/Digits.h"
#include "DataFormatsCTP/Configuration.h"
// #include "DataFormatsCTP/RunManager.h"
#include <Framework/InputRecord.h>
#include "Framework/TimingInfo.h"
#include "Common/Utils.h"
#include <CommonUtils/StringUtils.h>

using namespace o2::quality_control_modules::common;

namespace o2::quality_control_modules::ctp
{

CTPRawDataReaderTask::~CTPRawDataReaderTask()
{
  for (auto& h : mHisInputs) {
    delete h;
  }
  for (auto& h : mHisInputsNotLHC) { // must be before mHisInputsYesLHC
    delete h;
  }
  for (auto& h : mHisInputsYesLHC) {
    delete h;
  }
}

void CTPRawDataReaderTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize CTPRawDataReaderTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  int norbits = o2::constants::lhc::LHCMaxBunches;
  mHistoInputs = std::make_unique<TH1DRatio>("inputs", "Input Rates; Input ; Rate [kHz]", ninps, 0, ninps, true);
  mHistoClasses = std::make_unique<TH1DRatio>("classes", "Class Rates; Class; Rate [kHz]", nclasses, 0, nclasses, true);
  mHistoInputs->SetStats(0);
  mHistoClasses->SetStats(0);
  mHistoBCMinBias1 = std::make_unique<TH1D>("bcMinBias1", "BC position MB1", norbits, 0, norbits);
  mHistoBCMinBias2 = std::make_unique<TH1D>("bcMinBias2", "BC position MB2", norbits, 0, norbits);
  mHistoInputRatios = std::make_unique<TH1DRatio>("inputRatio", "Input Ratio to MTVX; Input; Ratio;", ninps, 0, ninps, true);
  mHistoClassRatios = std::make_unique<TH1DRatio>("classRatio", "Class Ratio to MB; Class; Ratio", nclasses, 0, nclasses, true);
  getObjectsManager()->startPublishing(mHistoInputs.get());
  getObjectsManager()->startPublishing(mHistoClasses.get());
  getObjectsManager()->startPublishing(mHistoClassRatios.get());
  getObjectsManager()->startPublishing(mHistoInputRatios.get());
  getObjectsManager()->startPublishing(mHistoBCMinBias1.get());
  getObjectsManager()->startPublishing(mHistoBCMinBias2.get());
  std::string sTmp1, sTmp2;
  for (std::size_t i = 0; i < mHisInputs.size(); i++) {
    sTmp1 = std::format("input{:02}", i);
    sTmp2 = std::format("input[{:02}] {}", i, o2::ctp::CTPInputsConfiguration::getInputNameFromIndex(i + 1));
    // getInputNameFromIndex in range [1-48]
    mHisInputs[i] = new TH1D(sTmp1.c_str(), sTmp2.c_str(), norbits, 0, norbits);

    sTmp1 = std::format("input{:02}_yesLHC", i);
    mHisInputsYesLHC[i] = new TH1D(sTmp1.c_str(), sTmp2.c_str(), norbits, 0, norbits);
    mHisInputsYesLHC[i]->SetLineColor(kGreen + 2);
    mHisInputsYesLHC[i]->SetFillColor(kGreen + 2);

    sTmp1 = std::format("input{:02}_notLHC", i);
    mHisInputsNotLHC[i] = new TH1D(sTmp1.c_str(), sTmp2.c_str(), norbits, 0, norbits);
    mHisInputsNotLHC[i]->SetLineColor(kRed + 1);
    mHisInputsNotLHC[i]->SetFillColor(kRed + 1);

    getObjectsManager()->startPublishing(mHisInputs[i]);
    getObjectsManager()->startPublishing(mHisInputsYesLHC[i]);
    // getObjectsManager()->startPublishing(mHisInputsNotLHC[i]);
  }

  mDecoder.setDoLumi(1);
  mDecoder.setDoDigits(1);
  for (size_t i = 0; i < nclasses; i++) {
    classNames[i] = "";
  }
  mHistoClassRatios.get()->GetXaxis()->CenterLabels(true);
  mHistoClasses.get()->GetXaxis()->CenterLabels(true);
}

void CTPRawDataReaderTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
  mHistoInputs->Reset();
  mHistoClasses->Reset();
  mHistoClassRatios->Reset();
  mHistoInputRatios->Reset();
  mHistoBCMinBias1->Reset();
  mHistoBCMinBias2->Reset();
  for (auto& h : mHisInputs) {
    h->Reset();
  }
  for (auto& h : mHisInputsYesLHC) {
    h->Reset("ICES");
  }
  for (auto& h : mHisInputsNotLHC) {
    h->Reset();
  }

  mRunNumber = activity.mId;
  mTimestamp = activity.mValidity.getMin();
  readLHCFillingScheme(); // call after mTimestamp is set

  std::string readCTPConfig = getFromExtendedConfig<std::string>(activity, mCustomParameters, "readCTPconfigInMonitorData", "false");
  if (readCTPConfig == "true") {
    mReadCTPconfigInMonitorData = true;
  }

  mMBclassName = getFromExtendedConfig<std::string>(activity, mCustomParameters, "MBclassName", "CMTVX-B-NOPF");

  std::string run = std::to_string(mRunNumber);
  std::string ccdbName = mCustomParameters["ccdbName"];
  if (ccdbName.empty()) {
    ccdbName = "https://alice-ccdb.cern.ch";
  }
  /// the ccdb reading to be futher discussed
  // o2::ctp::CTPRunManager::setCCDBHost(ccdbName);
  if (!mReadCTPconfigInMonitorData) {
    bool ok;
    auto& mgr = o2::ccdb::BasicCCDBManager::instance();
    mgr.setURL(ccdbName);
    std::map<std::string, std::string> metadata; // can be empty
    metadata["runNumber"] = run;
    auto ctpconfigdb = mgr.getSpecific<o2::ctp::CTPConfiguration>(o2::ctp::CCDBPathCTPConfig, mTimestamp, metadata);
    if (ctpconfigdb == nullptr) {
      LOG(info) << "CTP config not in database, timestamp:" << mTimestamp;
      ok = 0;
    } else {
      LOG(info) << "CTP config found. Run:" << run;
      ok = 1;
    }
    if (ok) {
      // get the index of the MB reference class
      ILOG(Info, Support) << "CTP config found, run:" << run << ENDM;
      std::vector<o2::ctp::CTPClass> ctpcls = ctpconfigdb->getCTPClasses();
      for (size_t i = 0; i < ctpcls.size(); i++) {
        classNames[i] = ctpcls[i].name.c_str();
        if (ctpcls[i].name.find(mMBclassName) != std::string::npos) {
          mIndexMBclass = ctpcls[i].getIndex() + 1;
          break;
        }
      }
      mDecoder.setCTPConfig(*ctpconfigdb);
    } else {
      ILOG(Warning, Support) << "CTP config not found, run:" << run << ENDM;
    }
    if (mIndexMBclass == -1) {
      mMBclassName = "CMBV0 (default)";
      mIndexMBclass = 1;
    }
    mHistoClassRatios->SetTitle(Form("Class Ratio to %s", mMBclassName.c_str()));
  }

  std::string nameInput1 = getFromExtendedConfig<std::string>(activity, mCustomParameters, "MB1inputName", "MTVX");
  std::string nameInput2 = getFromExtendedConfig<std::string>(activity, mCustomParameters, "MB2inputName", "MT0A");

  auto input1Tokens = o2::utils::Str::tokenize(nameInput1, ':', false, true);
  if (input1Tokens.size() > 0) {
    nameInput1 = input1Tokens[0];
  }
  if (input1Tokens.size() > 1) {
    mScaleInput1 = std::stod(input1Tokens[1]);
  }
  if (input1Tokens.size() > 2) {
    mShiftInput1 = std::stod(input1Tokens[2]);
  }

  auto input2Tokens = o2::utils::Str::tokenize(nameInput2, ':', false, true);
  if (input2Tokens.size() > 0) {
    nameInput2 = input2Tokens[0];
  }
  if (input2Tokens.size() > 1) {
    mScaleInput2 = std::stod(input2Tokens[1]);
  }
  if (input2Tokens.size() > 2) {
    mShiftInput2 = std::stod(input2Tokens[2]);
  }

  indexMB1 = o2::ctp::CTPInputsConfiguration::getInputIndexFromName(nameInput1);
  indexMB2 = o2::ctp::CTPInputsConfiguration::getInputIndexFromName(nameInput2);
  if (indexMB1 == -1) {
    indexMB1 = 3; // 3 is the MTVX index
    nameInput1 = "MTVX (default)";
  }
  if (indexMB2 == -1) {
    indexMB2 = 5; // 5 is the MTCE index
    nameInput2 = "MTCE (default)";
  }
  for (int i = 0; i < nclasses; i++) {
    if (classNames[i] == "") {
      mHistoClasses.get()->GetXaxis()->SetBinLabel(i + 1, Form("%i", i + 1));
      mHistoClassRatios.get()->GetXaxis()->SetBinLabel(i + 1, Form("%i", i + 1));
    } else {
      mHistoClasses.get()->GetXaxis()->SetBinLabel(i + 1, Form("%s", classNames[i].c_str()));
      mHistoClassRatios.get()->GetXaxis()->SetBinLabel(i + 1, Form("%s", classNames[i].c_str()));
    }
  }
  mHistoClasses.get()->GetXaxis()->SetLabelSize(0.025);
  mHistoClasses.get()->GetXaxis()->LabelsOption("v");
  mHistoClassRatios.get()->GetXaxis()->SetLabelSize(0.025);
  mHistoClassRatios.get()->GetXaxis()->LabelsOption("v");

  TString title1 = Form("BC position %s", nameInput1.c_str());
  TString titley1 = Form("Rate");
  TString titleX1 = Form("BC");
  if (mScaleInput1 > 1) {
    title1 += Form(", scaled with 1/%g", mScaleInput1);
    titley1 += Form(" #times 1/%g", mScaleInput1);
  }
  if (mShiftInput1 > 0) {
    title1 += Form(", shifted by - %d", mShiftInput1);
    titleX1 += Form(" - %d", mShiftInput1);
  }
  mHistoBCMinBias1->SetTitle(Form("%s; %s; %s", title1.Data(), titleX1.Data(), titley1.Data()));

  TString title2 = Form("BC position %s", nameInput2.c_str());
  TString titley2 = Form("Rate");
  TString titleX2 = Form("BC");
  if (mScaleInput2 > 1) {
    title2 += Form(", scaled with 1/%g", mScaleInput2);
    titley2 += Form(" #times 1/%g", mScaleInput2);
  }
  if (mShiftInput2 > 0) {
    title2 += Form(", shifted by - %d", mShiftInput2);
    titleX2 += Form(" - %d", mShiftInput2);
  }
  mHistoBCMinBias2->SetTitle(Form("%s; %s; %s", title2.Data(), titleX2.Data(), titley2.Data()));
  mHistoInputRatios->SetTitle(Form("Input Ratio to %s", nameInput1.c_str()));

  std::string performConsistencyCheck = getFromExtendedConfig<std::string>(activity, mCustomParameters, "consistencyCheck", "true");
  if (performConsistencyCheck == "true") {
    mDecoder.setCheckConsistency(1);
    mDecoder.setDecodeInps(1);
    mPerformConsistencyCheck = true;
    for (std::size_t i = 0; i < shiftBC.size(); i++) {
      shiftBC[i] = 0; // no shift
    }
  } else {
    mDecoder.setCheckConsistency(0);
    for (std::size_t i = 0; i < shiftBC.size(); i++) {
      if (i <= 10) {
        shiftBC[i] = 0; //   [00-10] without shift
      }
      if (i >= 11 && i <= 23) {
        shiftBC[i] = 15; //  [11-23] shift by 15 BCs
      } else if (i >= 24 && i <= 47) {
        shiftBC[i] = 296; // [24-47] shift by 296 BCs
      }
    }
  }

  if (mPerformConsistencyCheck) {
    mHistoDecodeError = std::make_unique<TH1D>("decodeError", "Errors from decoder", nclasses, 0, nclasses);
    getObjectsManager()->startPublishing(mHistoDecodeError.get());
  }
}

void CTPRawDataReaderTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void CTPRawDataReaderTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  static constexpr double sOrbitLengthInMS = o2::constants::lhc::LHCOrbitMUS / 1000;
  auto nOrbitsPerTF = 32.;
  //   get the input
  std::vector<o2::framework::InputSpec> filter{ o2::framework::InputSpec{ "filter", o2::framework::ConcreteDataTypeMatcher{ "DS", "RAWDATA" }, o2::framework::Lifetime::Timeframe } };
  std::vector<o2::ctp::LumiInfo> lumiPointsHBF1;
  std::vector<o2::ctp::CTPDigit> outputDigits;

  if (mReadCTPconfigInMonitorData) {
    if (mCTPconfig == nullptr) {
      mCTPconfig = ctx.inputs().get<o2::ctp::CTPConfiguration*>("ctp-config").get();
      // mCTPconfig = ctpConfigPtr.get();
      if (mCTPconfig != nullptr) {
        ILOG(Info, Support) << "CTP config found" << ENDM;
        std::vector<o2::ctp::CTPClass> ctpcls = mCTPconfig->getCTPClasses();
        for (size_t i = 0; i < ctpcls.size(); i++) {
          classNames[i] = ctpcls[i].name.c_str();
          if (ctpcls[i].name.find(mMBclassName) != std::string::npos) {
            mIndexMBclass = ctpcls[i].getIndex() + 1;
            break;
          }
        }
        mDecoder.setCTPConfig(*mCTPconfig);
      }
    }
    for (int i = 0; i < nclasses; i++) {
      if (classNames[i] == "") {
        mHistoClasses.get()->GetXaxis()->SetBinLabel(i + 1, Form("%i", i + 1));
        mHistoClassRatios.get()->GetXaxis()->SetBinLabel(i + 1, Form("%i", i + 1));
      } else {
        mHistoClasses.get()->GetXaxis()->SetBinLabel(i + 1, Form("%s", classNames[i].c_str()));
        mHistoClassRatios.get()->GetXaxis()->SetBinLabel(i + 1, Form("%s", classNames[i].c_str()));
      }
    }

    if (mIndexMBclass == -1) {
      mMBclassName = "CMBV0 (default)";
      mIndexMBclass = 1;
    }
    mHistoClassRatios->SetTitle(Form("Class Ratio to %s", mMBclassName.c_str()));
  }

  o2::framework::InputRecord& inputs = ctx.inputs();
  int ret = mDecoder.decodeRaw(inputs, filter, outputDigits, lumiPointsHBF1);
  mClassErrorsA = mDecoder.getClassErrorsA();
  if (mPerformConsistencyCheck) {
    for (size_t i = 0; i < o2::ctp::CTP_NCLASSES; i++) {
      if (mClassErrorsA[i] > 0) {
        mHistoDecodeError->Fill(i, mClassErrorsA[i]);
      }
    }
  }

  // reading the ctp inputs and ctp classes
  for (auto const digit : outputDigits) {
    uint16_t bcid = digit.intRecord.bc;
    if (digit.CTPInputMask.count()) {
      for (int i = 0; i < o2::ctp::CTP_NINPUTS; i++) {
        if (digit.CTPInputMask[i]) {
          mHistoInputs->getNum()->Fill(i);
          mHistoInputRatios->getNum()->Fill(i);
          if (i == indexMB1 - 1) {
            int bc = bcid - mShiftInput1 >= 0 ? bcid - mShiftInput1 : bcid - mShiftInput1 + 3564;
            mHistoBCMinBias1->Fill(bc, 1. / mScaleInput1);
            mHistoInputRatios->getDen()->Fill(0., 1);
          }
          if (i == indexMB2 - 1) {
            int bc = bcid - mShiftInput2 >= 0 ? bcid - mShiftInput2 : bcid - mShiftInput2 + 3564;
            mHistoBCMinBias2->Fill(bc, 1. / mScaleInput2);
          }
          // int bc = (bcid - shiftBC[i]) >= 0 ? bcid - shiftBC[i] : bcid - shiftBC[i] + o2::constants::lhc::LHCMaxBunches;
          int bc = bcid - shiftBC[i];
          mHisInputs[i]->Fill(bc);
        }
      }
    }
    if (digit.CTPClassMask.count()) {
      for (int i = 0; i < o2::ctp::CTP_NCLASSES; i++) {
        if (digit.CTPClassMask[i]) {
          mHistoClasses->getNum()->Fill(i);
          mHistoClassRatios->getNum()->Fill(i);
          if (i == mIndexMBclass - 1) {
            mHistoClassRatios->getDen()->Fill(0., 1);
          }
        }
      }
    }
  }
  mHistoInputs->getNum()->Fill(o2::ctp::CTP_NINPUTS);
  mHistoClasses->getNum()->Fill(o2::ctp::CTP_NCLASSES);

  // store total duration (in milliseconds) in denominator
  mHistoInputs->getDen()->Fill((double)0, sOrbitLengthInMS * nOrbitsPerTF);
  mHistoClasses->getDen()->Fill((double)0, sOrbitLengthInMS * nOrbitsPerTF);
}

void CTPRawDataReaderTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
  mHistoInputs->update();
  mHistoInputRatios->update();
  mHistoClasses->update();
  mHistoClassRatios->update();
  splitSortInputs();
}

void CTPRawDataReaderTask::splitSortInputs()
{
  struct BC {       // BC (Bunch Crossing)
    int id;         // id [0-3563]
    double entries; // number of entries for id
    // TH1::GetBinContent(i) return always double over TH1::RetrieveBinContent(i) implemented in TH1*
  };

  std::vector<BC> BCs; // ?! std::array<BC, 3564> BCs = {} ?!
  for (std::size_t ih = 0; ih < mHisInputs.size(); ih++) {
    if (mHisInputs[ih]->GetEntries() == 0) {
      continue;
    }
    BCs.clear();
    for (int ib = 1; ib <= mHisInputs[ih]->GetNbinsX(); ib++) { // skip underflow bin
      BCs.emplace_back(ib - 1, mHisInputs[ih]->GetBinContent(ib));
    }

    std::sort(BCs.begin(), BCs.end(),
              [](const BC& a, const BC& b) { return a.entries > b.entries; }); // sort by BC.entries
    // std::cout << "size: " << BCs.size() << std::endl;
    // for (const BC& bc : BCs) {
    //   std::cout << "BC.id: " << std::setw(4) << bc.id << ", BC.entries: " << bc.entries << std::endl;
    // }

    mHisInputsYesLHC[ih]->Reset("ICES"); // don't delete mHisInputsNotLHC[ih] object
    mHisInputsNotLHC[ih]->Reset();
    for (std::size_t ibc = 0; ibc < BCs.size(); ibc++) { // skip underflow bin (in loop)
      if (mLHCBCs.test(BCs[ibc].id)) {
        mHisInputsYesLHC[ih]->SetBinContent(ibc + 1, BCs[ibc].entries);
      } else {
        mHisInputsNotLHC[ih]->SetBinContent(ibc + 1, BCs[ibc].entries);
      }
    }

    TLine* line = nullptr;
    if (!mHisInputsYesLHC[ih]->FindObject(mHisInputsNotLHC[ih])) { // only once
      // temporary hack: hisNotLHC->Draw("same")
      mHisInputsYesLHC[ih]->GetListOfFunctions()->Add(mHisInputsNotLHC[ih], "same");
      // temporary hack: line->Draw("same")
      line = new TLine(mLHCBCs.count(), 0, mLHCBCs.count(), mHisInputsYesLHC[ih]->GetMaximum() * 1.05);
      line->SetLineStyle(kDotted);
      line->SetLineColor(mHisInputsYesLHC[ih]->GetLineColor());
      mHisInputsYesLHC[ih]->GetListOfFunctions()->Add(line, "same");
    } else { // always set Y2 line maximum (is different for each cycle)
      line = dynamic_cast<TLine*>(mHisInputsYesLHC[ih]->FindObject("TLine"));
      if (line) {
        line->SetY2(mHisInputsYesLHC[ih]->GetMaximum() * 1.05);
      }
    }
  }
}

void CTPRawDataReaderTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void CTPRawDataReaderTask::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  mHistoInputs->Reset();
  mHistoClasses->Reset();
  mHistoInputRatios->Reset();
  mHistoClassRatios->Reset();
  mHistoBCMinBias1->Reset();
  mHistoBCMinBias2->Reset();
  for (auto& h : mHisInputs) {
    h->Reset();
  }
  for (auto& h : mHisInputsYesLHC) {
    h->Reset("ICES");
  }
  for (auto& h : mHisInputsNotLHC) {
    h->Reset();
  }
  if (mPerformConsistencyCheck)
    mHistoDecodeError->Reset();
}

void CTPRawDataReaderTask::readLHCFillingScheme()
{
  // mTimestamp = activity.mValidity.getMin(); // set in startOfActivity()
  //
  // manually added timestamps corresponding to known fills and runs (for testing)
  // mTimestamp = 1716040930304 + 1; // fill:  9644, run: 551731
  // mTimestamp = 1754317528872 + 1; // fill: 10911, run: 565118
  // mTimestamp = 1760845636156 + 1; // fill: 11203, run: 567147

  std::map<std::string, std::string> metadata; // can be empty
  auto lhcifdata = UserCodeInterface::retrieveConditionAny<o2::parameters::GRPLHCIFData>("GLO/Config/GRPLHCIF", metadata, mTimestamp);
  if (lhcifdata == nullptr) {
    ILOG(Info, Support) << "LHC data not found for (task) timestamp:" << mTimestamp << ENDM;
    lhcDataFileFound = false;
    return;
  } else {
    ILOG(Info, Support) << "LHC data found for (task) timestamp:" << mTimestamp << ENDM;
    lhcDataFileFound = true;
    // lhcifdata->print();
  }
  auto bfilling = lhcifdata->getBunchFilling();
  std::vector<int> bcs = bfilling.getFilledBCs();
  mLHCBCs.reset();
  for (auto const& bc : bcs) {
    mLHCBCs.set(bc, 1);
  }
  // ?! test on mLHCBCs.size() == or <= o2::constants::lhc::LHCMaxBunches ?!
}

} // namespace o2::quality_control_modules::ctp
