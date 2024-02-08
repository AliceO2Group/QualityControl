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

namespace o2::quality_control_modules::ctp
{

CTPRawDataReaderTask::~CTPRawDataReaderTask()
{
  delete mHistoInputs;
  delete mHistoClasses;
  delete mHistoMTVXBC;
}

void CTPRawDataReaderTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize CTPRawDataReaderTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  mHistoInputs = new TH1F("inputs", "Inputs distribution", 48, 0, 48);
  mHistoClasses = new TH1F("classes", "Classes distribution", 65, 0, 65);
  mHistoMTVXBC = new TH1F("bcMTVX", "BC position of MTVX", 3564, 0, 3564);
  getObjectsManager()->startPublishing(mHistoInputs);
  getObjectsManager()->startPublishing(mHistoClasses);
  getObjectsManager()->startPublishing(mHistoMTVXBC);

  mDecoder.setDoLumi(1);
  mDecoder.setDoDigits(1);
}

void CTPRawDataReaderTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
  mHistoInputs->Reset();
  mHistoClasses->Reset();
  mHistoMTVXBC->Reset();
}

void CTPRawDataReaderTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void CTPRawDataReaderTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // LOG(info) << "============  Starting monitoring ================== ";
  //  get the input
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
          mHistoInputs->Fill(i);
          if (i == indexTvx - 1)
            mHistoMTVXBC->Fill(bcid);
        }
      }
    }
    if (digit.CTPClassMask.count()) {
      for (int i = 0; i < o2::ctp::CTP_NCLASSES; i++) {
        if (digit.CTPClassMask[i]) {
          mHistoClasses->Fill(i);
        }
      }
    }
  }
}

void CTPRawDataReaderTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
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
  mHistoMTVXBC->Reset();
}

} // namespace o2::quality_control_modules::ctp
