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
#include "DPLUtils/DPLRawParser.h"
#include "DataFormatsCTP/Digits.h"
#include "DataFormatsCTP/Configuration.h"
#include "DataFormatsCTP/RunManager.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include "Framework/TimingInfo.h"
#include <DetectorsBase/GRPGeomHelper.h>

namespace o2::quality_control_modules::ctp
{

CTPRawDataReaderTask::~CTPRawDataReaderTask()
{
}

void CTPRawDataReaderTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize CTPRawDataReaderTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  int ninps = o2::ctp::CTP_NINPUTS + 1;
  int nclasses = o2::ctp::CTP_NCLASSES + 1;
  int norbits = o2::constants::lhc::LHCMaxBunches;
  mHistoInputs = std::make_unique<TH1DRatio>("inputs", "Input Rates; Input ; Rate [kHz]", ninps, 0, ninps, true);
  mHistoClasses = std::make_unique<TH1DRatio>("classes", "Class Rates; Index; Rate [kHz]", nclasses, 0, nclasses, true);
  mHistoInputs->SetStats(0);
  mHistoClasses->SetStats(0);
  mHistoMTVXBC = std::make_unique<TH1D>("bcMTVX", "BC position of MTVX", norbits, 0, norbits);
  mHistoInputRatios = std::make_unique<TH1DRatio>("inputRatio", "Input Ratio to MTVX; Input; Ratio;", ninps, 0, ninps, true);
  mHistoClassRatios = std::make_unique<TH1DRatio>("classRatio", "Class Ratio to MB; Index; Ratio", nclasses, 0, nclasses, true);
  getObjectsManager()->startPublishing(mHistoInputs.get());
  getObjectsManager()->startPublishing(mHistoClasses.get());
  getObjectsManager()->startPublishing(mHistoClassRatios.get());
  getObjectsManager()->startPublishing(mHistoInputRatios.get());
  getObjectsManager()->startPublishing(mHistoMTVXBC.get());

  mDecoder.setDoLumi(1);
  mDecoder.setDoDigits(1);
}

void CTPRawDataReaderTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
  mHistoInputs->Reset();
  mHistoClasses->Reset();
  mHistoClassRatios->Reset();
  mHistoInputRatios->Reset();
  mHistoMTVXBC->Reset();

  mRunNumber = activity.mId;
  mTimestamp = activity.mValidity.getMin();

  std::string MBclassName = mCustomParameters["MBclassName"];
  if (MBclassName.empty()) {
    MBclassName = "CMTVX-B-NOPF";
  }
  std::string run = std::to_string(mRunNumber);
  std::string ccdbName = mCustomParameters["ccdbName"];
  if (ccdbName.empty()) {
    ccdbName = "https://alice-ccdb.cern.ch";
  }

  o2::ctp::CTPRunManager::setCCDBHost(ccdbName);
  bool ok;
  o2::ctp::CTPConfiguration CTPconfig = o2::ctp::CTPRunManager::getConfigFromCCDB(mTimestamp, run, ok);
  if (ok) {
    // get the index of the MB reference class
    ILOG(Info, Support) << "CTP config found, run:" << run << ENDM;
    std::vector<o2::ctp::CTPClass> ctpcls = CTPconfig.getCTPClasses();
    for (size_t i = 0; i < ctpcls.size(); i++) {
      if (ctpcls[i].name.find(MBclassName) != std::string::npos) {
        mIndexMBclass = ctpcls[i].getIndex() + 1;
        break;
      }
    }
  } else {
    ILOG(Warning, Support) << "CTP config not found, run:" << run << ENDM;
  }
  if (mIndexMBclass == -1) {
    mIndexMBclass = 1;
  }
}

void CTPRawDataReaderTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void CTPRawDataReaderTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  static constexpr double sOrbitLengthInMS = o2::constants::lhc::LHCOrbitMUS / 1000;
  // auto nOrbitsPerTF = o2::base::GRPGeomHelper::instance().(getNHBFPerTF); gives 128 ?
  auto nOrbitsPerTF = 32.;
  // LOG(info) << "============  Starting monitoring ================== ";
  //   get the input
  std::vector<o2::framework::InputSpec> filter;
  std::vector<o2::ctp::LumiInfo> lumiPointsHBF1;
  std::vector<o2::ctp::CTPDigit> outputDigits;

  o2::framework::InputRecord& inputs = ctx.inputs();
  mDecoder.decodeRaw(inputs, filter, outputDigits, lumiPointsHBF1);

  std::string nameInput = "MTVX";
  auto indexTvx = o2::ctp::CTPInputsConfiguration::getInputIndexFromName(nameInput);
  for (auto const digit : outputDigits) {
    uint16_t bcid = digit.intRecord.bc;
    if (digit.CTPInputMask.count()) {
      for (int i = 0; i < o2::ctp::CTP_NINPUTS; i++) {
        if (digit.CTPInputMask[i]) {
          mHistoInputs->getNum()->Fill(i);
          mHistoInputRatios->getNum()->Fill(i);
          if (i == indexTvx - 1) {
            mHistoMTVXBC->Fill(bcid);
            mHistoInputRatios->getDen()->Fill(0., 1);
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
  mHistoMTVXBC->Reset();
}

} // namespace o2::quality_control_modules::ctp
