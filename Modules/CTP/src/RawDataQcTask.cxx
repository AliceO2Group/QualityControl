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
///

#include <TCanvas.h>
#include <TH1.h>

#include "QualityControl/QcInfoLogger.h"
#include "CTP/RawDataQcTask.h"
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

  mRunNumber = activity.mId;
  mTimestamp = activity.mValidity.getMin();

  auto MBclassName = getFromExtendedConfig<string>(activity, mCustomParameters, "MBclassName", "CMTVX-B-NOPF");

  std::string run = std::to_string(mRunNumber);
  std::string ccdbName = mCustomParameters["ccdbName"];
  if (ccdbName.empty()) {
    ccdbName = "https://alice-ccdb.cern.ch";
  }
  /// the ccdb reading to be futher discussed
  // o2::ctp::CTPRunManager::setCCDBHost(ccdbName);
  bool ok;
  // o2::ctp::CTPConfiguration CTPconfig = o2::ctp::CTPRunManager::getConfigFromCCDB(mTimestamp, run, ok);
  auto& mgr = o2::ccdb::BasicCCDBManager::instance();
  mgr.setURL(ccdbName);
  map<string, string> metadata; // can be empty
  metadata["runNumber"] = run;
  auto ctpconfigdb = mgr.getSpecific<o2::ctp::CTPConfiguration>(o2::ctp::CCDBPathCTPConfig, mTimestamp, metadata);
  if (ctpconfigdb == nullptr) {
    LOG(info) << "CTP config not in database, timestamp:" << mTimestamp;
    ok = 0;
  } else {
    // ctpconfigdb->printStream(std::cout);
    LOG(info) << "CTP config found. Run:" << run;
    ok = 1;
  }
  if (ok) {
    // get the index of the MB reference class
    ILOG(Info, Support) << "CTP config found, run:" << run << ENDM;
    std::vector<o2::ctp::CTPClass> ctpcls = ctpconfigdb->getCTPClasses();
    for (size_t i = 0; i < ctpcls.size(); i++) {
      classNames[i] = ctpcls[i].name.c_str();
      if (ctpcls[i].name.find(MBclassName) != std::string::npos) {
        mIndexMBclass = ctpcls[i].getIndex() + 1;
        break;
      }
    }
  } else {
    ILOG(Warning, Support) << "CTP config not found, run:" << run << ENDM;
  }
  if (mIndexMBclass == -1) {
    MBclassName = "CMBV0 (default)";
    mIndexMBclass = 1;
  }
  std::string nameInput1 = getFromExtendedConfig<string>(activity, mCustomParameters, "MB1inputName", "MTVX");
  std::string nameInput2 = getFromExtendedConfig<string>(activity, mCustomParameters, "MB2inputName", "MT0A");

  auto input2Tokens = o2::utils::Str::tokenize(nameInput2, ':', false, true);
  if (input2Tokens.size() > 0) {
    nameInput2 = input2Tokens[0];
  }
  if (input2Tokens.size() > 1) {
    mScaleInput2 = std::stod(input2Tokens[1]);
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

  mHistoBCMinBias1->SetTitle(Form("BC position %s", nameInput1.c_str()));
  mHistoBCMinBias2->SetTitle(Form("BC position %s", nameInput2.c_str()));
  if (mScaleInput2 > 1) {
    mHistoBCMinBias2->SetTitle(Form("BC position %s scaled 1/%g", nameInput2.c_str(), mScaleInput2));
  }
  mHistoClassRatios->SetTitle(Form("Class Ratio to %s", MBclassName.c_str()));
  mHistoInputRatios->SetTitle(Form("Input Ratio to %s", nameInput1.c_str()));
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
  std::vector<o2::framework::InputSpec> filter;
  std::vector<o2::ctp::LumiInfo> lumiPointsHBF1;
  std::vector<o2::ctp::CTPDigit> outputDigits;

  o2::framework::InputRecord& inputs = ctx.inputs();
  mDecoder.decodeRaw(inputs, filter, outputDigits, lumiPointsHBF1);

  // reading the ctp inputs and ctp classes
  for (auto const digit : outputDigits) {
    uint16_t bcid = digit.intRecord.bc;
    if (digit.CTPInputMask.count()) {
      for (int i = 0; i < o2::ctp::CTP_NINPUTS; i++) {
        if (digit.CTPInputMask[i]) {
          mHistoInputs->getNum()->Fill(i);
          mHistoInputRatios->getNum()->Fill(i);
          if (i == indexMB1 - 1) {
            mHistoBCMinBias1->Fill(bcid);
            mHistoInputRatios->getDen()->Fill(0., 1);
          }
          if (i == indexMB2 - 1) {
            mHistoBCMinBias2->Fill(bcid, 1. / mScaleInput2);
          }
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
}

} // namespace o2::quality_control_modules::ctp
