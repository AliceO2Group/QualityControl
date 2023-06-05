// Copyright 2019-2022 CERN and copyright holders of ALICE O2.
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
/// \file   ZDCRawDataTask.cxx
/// \author Carlo Puggioni
///

#include <TCanvas.h>
#include <TH1.h>

#include "QualityControl/QcInfoLogger.h"
#include "ZDC/ZDCRawDataTask.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include "DPLUtils/DPLRawParser.h"
#include <TROOT.h>
#include <TPad.h>
#include <TString.h>
#include <TStyle.h>
#include <TPaveStats.h>
#include "CommonConstants/LHCConstants.h"
#include "ZDCSimulation/Digits2Raw.h"
#include "ZDCSimulation/ZDCSimParam.h"
#include <fairlogger/Logger.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <ctype.h>
#include <boost/algorithm/string.hpp>
using namespace o2::zdc;

namespace o2::quality_control_modules::zdc
{

ZDCRawDataTask::~ZDCRawDataTask()
{
  if (fFireChannel) {
    delete fFireChannel;
  }
  if (fTrasmChannel) {
    delete fTrasmChannel;
  }
  if (fSummaryPedestal) {
    delete fSummaryPedestal;
  }
  if (fTriggerBits) {
    delete fTriggerBits;
  }
  if (fTriggerBitsHits) {
    delete fTriggerBitsHits;
  }
  if (fDataLoss) {
    delete fDataLoss;
  }
  if (fOverBc) {
    delete fOverBc;
  }
}

void ZDCRawDataTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize ZDCRawDataTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  init();
}

void ZDCRawDataTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity" << activity.mId << ENDM;
  // reset for all object
  reset();
}

void ZDCRawDataTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
  fNumCycle++;
}

void ZDCRawDataTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  o2::framework::DPLRawParser parser(ctx.inputs());
  int PayloadPerGBTW = 10;
  int dataFormat;
  const uint32_t* gbtw;
  uint64_t count = 0;
  size_t payloadSize;
  size_t offset;
  static uint64_t nErr[3] = { 0 };

  for (auto it = parser.begin(), end = parser.end(); it != end; ++it) {
    auto rdhPtr = reinterpret_cast<const o2::header::RDHAny*>(it.raw());
    if (rdhPtr == nullptr || !o2::raw::RDHUtils::checkRDH(rdhPtr, true)) {
      nErr[0]++;
      /*if (nErr[0] < 5) {
        LOG(warning) << "ZDCDataReaderDPLSpec::run - Missing RAWDataHeader on page " << count;
      } else if (nErr[0] == 5) {
        LOG(warning) << "ZDCDataReaderDPLSpec::run - Missing RAWDataHeader on page " << count << " suppressing further messages";
      }*/
    } else {
      if (it.data() == nullptr) {
        nErr[1]++;
      } else if (it.size() == 0) {
        nErr[2]++;
      } else {
        // retrieving payload pointer of the page
        auto const* payload = it.data();
        // size of payload
        payloadSize = it.size();
        // offset of payload in the raw page
        offset = it.offset();
        dataFormat = o2::raw::RDHUtils::getDataFormat(rdhPtr);
        if (dataFormat == 2) {
          for (int32_t ip = 0; (ip + PayloadPerGBTW) <= payloadSize; ip += PayloadPerGBTW) {
            gbtw = (const uint32_t*)&payload[ip];
            if (gbtw[0] != 0xffffffff || gbtw[1] != 0xffffffff || (gbtw[2] & 0xffff) != 0xffff) {
              processWord(gbtw);
            }
          }
        } else if (dataFormat == 0) {
          for (int32_t ip = 0; ip < (int32_t)payloadSize; ip += 16) {
            processWord((const uint32_t*)&payload[ip]);
          }
        }
      }
    }
  }
}

void ZDCRawDataTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
  // dumpHistoStructure();
}

void ZDCRawDataTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void ZDCRawDataTask::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;

  for (int i = 0; i < o2::zdc::NModules; i++) {
    for (int j = 0; j < o2::zdc::NChPerModule; j++) {
      for (int k = 0; k < (int)fMatrixHistoBaseline[i][j].size(); k++) {
        fMatrixHistoBaseline[i][j].at(k).histo->Reset();
      }
    }
  }

  for (int i = 0; i < o2::zdc::NModules; i++) {
    for (int j = 0; j < o2::zdc::NChPerModule; j++) {
      for (int k = 0; k < (int)fMatrixHistoCounts[i][j].size(); k++) {
        fMatrixHistoCounts[i][j].at(k).histo->Reset();
      }
    }
  }

  for (int i = 0; i < o2::zdc::NModules; i++) {
    for (int j = 0; j < o2::zdc::NChPerModule; j++) {
      for (int k = 0; k < (int)fMatrixHistoSignal[i][j].size(); k++) {
        fMatrixHistoSignal[i][j].at(k).histo->Reset();
      }
    }
  }

  for (int i = 0; i < o2::zdc::NModules; i++) {
    for (int j = 0; j < o2::zdc::NChPerModule; j++) {
      for (int k = 0; k < (int)fMatrixHistoBunch[i][j].size(); k++) {
        fMatrixHistoBunch[i][j].at(k).histo->Reset();
      }
    }
  }
  if (fFireChannel) {
    fFireChannel->Reset();
  }
  if (fTrasmChannel) {
    fTrasmChannel->Reset();
  }
  if (fSummaryPedestal) {
    fSummaryPedestal->Reset();
  }
  if (fTriggerBits) {
    fTriggerBits->Reset();
  }
  if (fTriggerBitsHits) {
    fTriggerBitsHits->Reset();
  }
  if (fDataLoss) {
    fDataLoss->Reset();
  }
  if (fOverBc) {
    fOverBc->Reset();
  }
  if (fSummaryRate) {
    fSummaryRate->Reset();
  }
  if (fSummaryAlign) {
    fSummaryAlign->Reset();
  }
}

void ZDCRawDataTask::initHisto()
{
  ILOG(Debug, Devel) << "initialize ZDC RAW DATA HISTOGRAMS" << ENDM;
  setNameChannel(0, 0, "ZNA_TC_TR", 1);
  setNameChannel(0, 1, "ZNA_SUM_SP", -1);
  setNameChannel(0, 2, "ZNA_T1", 2);
  setNameChannel(0, 3, "ZNA_T2", 3);
  setNameChannel(1, 0, "ZNA_TC_OTR", -1);
  setNameChannel(1, 1, "ZNA_SUM", 4);
  setNameChannel(1, 2, "ZNA_T3", 5);
  setNameChannel(1, 3, "ZNA_T4", 6);
  setNameChannel(2, 0, "ZNC_TC_TR", 7);
  setNameChannel(2, 1, "ZNC_SUM_SP", -1);
  setNameChannel(2, 2, "ZNC_T1", 8);
  setNameChannel(2, 3, "ZNC_T2", 9);
  setNameChannel(3, 0, "ZNC_TC_OTR", -1);
  setNameChannel(3, 1, "ZNC_SUM", 10);
  setNameChannel(3, 2, "ZNC_T3", 11);
  setNameChannel(3, 3, "ZNC_T4", 12);
  setNameChannel(4, 0, "ZPA_TC_TR", 13);
  setNameChannel(4, 1, "ZEM1_TR", 14);
  setNameChannel(4, 2, "ZPA_T1", 15);
  setNameChannel(4, 3, "ZPA_T2", 16);
  setNameChannel(5, 0, "ZPA_TC_OTR", -1);
  setNameChannel(5, 1, "ZPA_SUM", 17);
  setNameChannel(5, 2, "ZPA_T3", 18);
  setNameChannel(5, 3, "ZPA_T4", 19);
  setNameChannel(6, 0, "ZPC_TC_TR", 20);
  setNameChannel(6, 1, "ZEM2_TR", 21);
  setNameChannel(6, 2, "ZPC_T3", 22);
  setNameChannel(6, 3, "ZPC_T4", 23);
  setNameChannel(7, 0, "ZPC_TC_OTR", -1);
  setNameChannel(7, 1, "ZPC_SUM", 24);
  setNameChannel(7, 2, "ZPC_T1", 25);
  setNameChannel(7, 3, "ZPC_T2", 26);

  std::vector<std::string> tokenString;
  // Histograms Baseline
  // setBinHisto1D(16378, -0.125, o2::zdc::ADCMax + 0.125);
  if (auto param = mCustomParameters.find("BASELINE"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - BASELINE: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto1D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()));
  } else {
    setBinHisto1D(4096, -2048.5, 2047.5);
  }
  addNewHisto("BASELINE", "hped-ZNA_TC_TR", "Baseline ZNA TC", "ZNA_TC_TR", "LBC");
  addNewHisto("BASELINE", "hped-ZNA_T1", "Baseline ZNA T1", "ZNA_T1", "LBC");
  addNewHisto("BASELINE", "hped-ZNA_T2", "Baseline ZNA T2", "ZNA_T2", "LBC");
  addNewHisto("BASELINE", "hped-ZNA_T3", "Baseline ZNA T3", "ZNA_T3", "LBC");
  addNewHisto("BASELINE", "hped-ZNA_T4", "Baseline ZNA T4", "ZNA_T4", "LBC");
  addNewHisto("BASELINE", "hped-ZNA_SUM", "Baseline ZNA SUM", "ZNA_SUM", "LBC");

  addNewHisto("BASELINE", "hped-ZNC_TC_TR", "Baseline ZNC TC", "ZNC_TC_TR", "LBC");
  addNewHisto("BASELINE", "hped-ZNC_T1", "Baseline ZNC T1", "ZNC_T1", "LBC");
  addNewHisto("BASELINE", "hped-ZNC_T2", "Baseline ZNC T2", "ZNC_T2", "LBC");
  addNewHisto("BASELINE", "hped-ZNC_T3", "Baseline ZNC T3", "ZNC_T3", "LBC");
  addNewHisto("BASELINE", "hped-ZNC_T4", "Baseline ZNC T4", "ZNC_T4", "LBC");
  addNewHisto("BASELINE", "hped-ZNC_SUM", "Baseline ZNC SUM", "ZNC_SUM", "LBC");

  addNewHisto("BASELINE", "hped-ZPA_TC_TR", "Baseline ZPA TC", "ZPA_TC_TR", "LBC");
  addNewHisto("BASELINE", "hped-ZPA_T1", "Baseline ZPA T1", "ZPA_T1", "LBC");
  addNewHisto("BASELINE", "hped-ZPA_T2", "Baseline ZPA T2", "ZPA_T2", "LBC");
  addNewHisto("BASELINE", "hped-ZPA_T3", "Baseline ZPA T3", "ZPA_T3", "LBC");
  addNewHisto("BASELINE", "hped-ZPA_T4", "Baseline ZPA T4", "ZPA_T4", "LBC");
  addNewHisto("BASELINE", "hped-ZPA_SUM", "Baseline ZPA SUM", "ZPA_SUM", "LBC");

  addNewHisto("BASELINE", "hped-ZPC_TC_TR", "Baseline ZPC TC", "ZPC_TC_TR", "LBC");
  addNewHisto("BASELINE", "hped-ZPC_T1", "Baseline ZPC T1", "ZPC_T1", "LBC");
  addNewHisto("BASELINE", "hped-ZPC_T2", "Baseline ZPC T2", "ZPC_T2", "LBC");
  addNewHisto("BASELINE", "hped-ZPC_T3", "Baseline ZPC T3", "ZPC_T3", "LBC");
  addNewHisto("BASELINE", "hped-ZPC_T4", "Baseline ZPC T4", "ZPC_T4", "LBC");
  addNewHisto("BASELINE", "hped-ZPC_SUM", "Baseline ZPC SUM", "ZPC_SUM", "LBC");

  addNewHisto("BASELINE", "hped-ZEM1_TR", "Baseline ZEM1", "ZEM1_TR", "LBC");
  addNewHisto("BASELINE", "hped-ZEM2_TR", "Baseline ZEM2", "ZEM2_TR", "LBC");

  // Histograms Counts
  // setBinHisto1D(o2::constants::lhc::LHCMaxBunches + 6, -5.5, o2::constants::lhc::LHCMaxBunches+0.5);
  if (auto param = mCustomParameters.find("COUNTS"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - COUNTS: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto1D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()));
  } else {
    setBinHisto1D(10, -0.5, 9.5);
  }
  addNewHisto("COUNTS", "hcounts-ZNA_TC_TR", "Counts ZNA TC", "ZNA_TC_TR", "LBC");
  addNewHisto("COUNTS", "hcounts-ZNA_T1", "Counts ZNA T1", "ZNA_T1", "LBC");
  addNewHisto("COUNTS", "hcounts-ZNA_T2", "Counts ZNA T2", "ZNA_T2", "LBC");
  addNewHisto("COUNTS", "hcounts-ZNA_T3", "Counts ZNA T3", "ZNA_T3", "LBC");
  addNewHisto("COUNTS", "hcounts-ZNA_T4", "Counts ZNA T4", "ZNA_T4", "LBC");
  addNewHisto("COUNTS", "hcounts-ZNA_SUM", "Counts ZNA SUM", "ZNA_SUM", "LBC");

  addNewHisto("COUNTS", "hcounts-ZNC_TC_TR", "Counts ZNC TC", "ZNC_TC_TR", "LBC");
  addNewHisto("COUNTS", "hcounts-ZNC_T1", "Counts ZNC T1", "ZNC_T1", "LBC");
  addNewHisto("COUNTS", "hcounts-ZNC_T2", "Counts ZNC T2", "ZNC_T2", "LBC");
  addNewHisto("COUNTS", "hcounts-ZNC_T3", "Counts ZNC T3", "ZNC_T3", "LBC");
  addNewHisto("COUNTS", "hcounts-ZNC_T4", "Counts ZNC T4", "ZNC_T4", "LBC");
  addNewHisto("COUNTS", "hcounts-ZNC_SUM", "Counts ZNC SUM", "ZNC_SUM", "LBC");

  addNewHisto("COUNTS", "hcounts-ZPA_TC_TR", "Counts ZPA TC", "ZPA_TC_TR", "LBC");
  addNewHisto("COUNTS", "hcounts-ZPA_T1", "Counts ZPA T1", "ZPA_T1", "LBC");
  addNewHisto("COUNTS", "hcounts-ZPA_T2", "Counts ZPA T2", "ZPA_T2", "LBC");
  addNewHisto("COUNTS", "hcounts-ZPA_T3", "Counts ZPA T3", "ZPA_T3", "LBC");
  addNewHisto("COUNTS", "hcounts-ZPA_T4", "Counts ZPA T4", "ZPA_T4", "LBC");
  addNewHisto("COUNTS", "hcounts-ZPA_SUM", "Counts ZPA SUM", "ZPA_SUM", "LBC");

  addNewHisto("COUNTS", "hcounts-ZPC_TC_TR", "Counts ZPC TC", "ZPC_TC_TR", "LBC");
  addNewHisto("COUNTS", "hcounts-ZPC_T1", "Counts ZPC T1", "ZPC_T1", "LBC");
  addNewHisto("COUNTS", "hcounts-ZPC_T2", "Counts ZPC T2", "ZPC_T2", "LBC");
  addNewHisto("COUNTS", "hcounts-ZPC_T3", "Counts ZPC T3", "ZPC_T3", "LBC");
  addNewHisto("COUNTS", "hcounts-ZPC_T4", "Counts ZPC T4", "ZPC_T4", "LBC");
  addNewHisto("COUNTS", "hcounts-ZPC_SUM", "Counts ZPC SUM", "ZPC_SUM", "LBC");

  addNewHisto("COUNTS", "hcounts-ZEM1_TR", "Counts ZEM1", "ZEM1_TR", "LBC");
  addNewHisto("COUNTS", "hcounts-ZEM2_TR", "Counts ZEM2", "ZEM2_TR", "LBC");
  // Histograms Signal
  int nBCAheadTrig = 3;
  int nbx = (nBCAheadTrig + 1 + 12) * o2::zdc::NTimeBinsPerBC;
  double xmin = -nBCAheadTrig * o2::zdc::NTimeBinsPerBC - 0.5;
  double xmax = o2::zdc::NTimeBinsPerBC - 0.5 + 12;
  // setBinHisto2D(nbx, xmin, xmax, o2::zdc::ADCRange, o2::zdc::ADCMin - 0.5, o2::zdc::ADCMax + 0.5);
  if (auto param = mCustomParameters.find("SIGNAL"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - SIGNAL: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto2D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()), atoi(tokenString.at(3).c_str()), atof(tokenString.at(4).c_str()), atof(tokenString.at(5).c_str()));
  } else {
    setBinHisto2D(60, -36.5, 23.5, 4096, -2048.5, 2047.5);
  }
  // Alice Trigger Or Auto Trigger
  addNewHisto("SIGNAL", "hsignal-ZNA_TC_TR_AoT", "Signal ZNA TC Trigger Alice OR Auto Trigger", "ZNA_TC_TR", "AoT");
  addNewHisto("SIGNAL", "hsignal-ZNA_T1_AoT", "Signal ZNA T1 Trigger Alice OR Auto Trigger", "ZNA_T1", "AoT");
  addNewHisto("SIGNAL", "hsignal-ZNA_T2_AoT", "Signal ZNA T2 Trigger Alice OR Auto Trigger", "ZNA_T2", "AoT");
  addNewHisto("SIGNAL", "hsignal-ZNA_T3_AoT", "Signal ZNA T3 Trigger Alice OR Auto Trigger", "ZNA_T3", "AoT");
  addNewHisto("SIGNAL", "hsignal-ZNA_T4_AoT", "Signal ZNA T4 Trigger Alice OR Auto Trigger", "ZNA_T4", "AoT");
  addNewHisto("SIGNAL", "hsignal-ZNA_SUM_AoT", "Signal ZNA SUM Trigger Alice OR Auto Trigger", "ZNA_SUM", "AoT");

  addNewHisto("SIGNAL", "hsignal-ZNC_TC_TR_AoT", "Signal ZNC TC Trigger Alice OR Auto Trigger", "ZNC_TC_TR", "AoT");
  addNewHisto("SIGNAL", "hsignal-ZNC_T1_AoT", "Signal ZNC T1 Trigger Alice OR Auto Trigger", "ZNC_T1", "AoT");
  addNewHisto("SIGNAL", "hsignal-ZNC_T2_AoT", "Signal ZNC T2 Trigger Alice OR Auto Trigger", "ZNC_T2", "AoT");
  addNewHisto("SIGNAL", "hsignal-ZNC_T3_AoT", "Signal ZNC T3 Trigger Alice OR Auto Trigger", "ZNC_T3", "AoT");
  addNewHisto("SIGNAL", "hsignal-ZNC_T4_AoT", "Signal ZNC T4 Trigger Alice OR Auto Trigger", "ZNC_T4", "AoT");
  addNewHisto("SIGNAL", "hsignal-ZNC_SUM_AoT", "Signal ZNC SUM Trigger Alice OR Auto Trigger", "ZNC_SUM", "AoT");

  addNewHisto("SIGNAL", "hsignal-ZPA_TC_TR_AoT", "Signal ZPA TC Trigger Alice OR Auto Trigger", "ZPA_TC_TR", "AoT");
  addNewHisto("SIGNAL", "hsignal-ZPA_T1_AoT", "Signal ZPA T1 Trigger Alice OR Auto Trigger", "ZPA_T1", "AoT");
  addNewHisto("SIGNAL", "hsignal-ZPA_T2_AoT", "Signal ZPA T2 Trigger Alice OR Auto Trigger", "ZPA_T2", "AoT");
  addNewHisto("SIGNAL", "hsignal-ZPA_T3_AoT", "Signal ZPA T3 Trigger Alice OR Auto Trigger", "ZPA_T3", "AoT");
  addNewHisto("SIGNAL", "hsignal-ZPA_T4_AoT", "Signal ZPA T4 Trigger Alice OR Auto Trigger", "ZPA_T4", "AoT");
  addNewHisto("SIGNAL", "hsignal-ZPA_SUM_AoT", "Signal ZPA SUM Trigger Alice OR Auto Trigger", "ZPA_SUM", "AoT");

  addNewHisto("SIGNAL", "hsignal-ZPC_TC_TR_AoT", "Signal ZPC TC Trigger Alice OR Auto Trigger", "ZPC_TC_TR", "AoT");
  addNewHisto("SIGNAL", "hsignal-ZPC_T1_AoT", "Signal ZPC T1 Trigger Alice OR Auto Trigger", "ZPC_T1", "AoT");
  addNewHisto("SIGNAL", "hsignal-ZPC_T2_AoT", "Signal ZPC T2 Trigger Alice OR Auto Trigger", "ZPC_T2", "AoT");
  addNewHisto("SIGNAL", "hsignal-ZPC_T3_AoT", "Signal ZPC T3 Trigger Alice OR Auto Trigger", "ZPC_T3", "AoT");
  addNewHisto("SIGNAL", "hsignal-ZPC_T4_AoT", "Signal ZPC T4 Trigger Alice OR Auto Trigger", "ZPC_T4", "AoT");
  addNewHisto("SIGNAL", "hsignal-ZPC_SUM_AoT", "Signal ZPC SUM Trigger Alice OR Auto Trigger", "ZPC_SUM", "AoT");

  addNewHisto("SIGNAL", "hsignal-ZEM1_TR_AoT", "Signal ZEM1 Trigger Alice OR Auto Trigger", "ZEM1_TR", "AoT");
  addNewHisto("SIGNAL", "hsignal-ZEM2_TR_AoT", "Signal ZEM2 Trigger Alice OR Auto Trigger", "ZEM2_TR", "AoT");

  // Histograms Bunch Crossing Maps
  if (auto param = mCustomParameters.find("BUNCH"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - BUNCH: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto2D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()), atoi(tokenString.at(3).c_str()), atof(tokenString.at(4).c_str()), atof(tokenString.at(5).c_str()));
  } else {
    setBinHisto2D(100, -0.5, 99.5, 36, -35.5, 0.5);
  }
  addNewHisto("BUNCH", "hbunch-ZNA_TC_TR_A0oT0", "Bunch ZNA TC Ali Trigger OR AutoTrigger", "ZNA_TC_TR", "A0oT0");
  addNewHisto("BUNCH", "hbunch-ZNA_SUM_A0oT0", "Bunch ZNA SUM Ali Trigger OR AutoTrigger", "ZNA_SUM", "A0oT0");
  addNewHisto("BUNCH", "hbunch-ZNC_TC_TR_A0oT0", "Bunch ZNC TC Ali Trigger OR AutoTrigger", "ZNC_TC_TR", "A0oT0");
  addNewHisto("BUNCH", "hbunch-ZNC_SUM_A0oT0", "Bunch ZNC SUM  Ali Trigger OR AutoTrigger", "ZNC_SUM", "A0oT0");
  addNewHisto("BUNCH", "hbunch-ZPA_TC_TR_A0oT0", "Bunch ZPA TC Ali Trigger OR AutoTrigger", "ZPA_TC_TR", "A0oT0");
  addNewHisto("BUNCH", "hbunch-ZPA_SUM_A0oT0", "Bunch ZPA SUM  Ali Trigger OR AutoTrigger", "ZPA_SUM", "A0oT0");
  addNewHisto("BUNCH", "hbunch-ZPC_TC_TR_A0oT0", "Bunch ZPC TC Ali Trigger OR AutoTrigger", "ZPC_TC_TR", "A0oT0");
  addNewHisto("BUNCH", "hbunch-ZPC_SUM_A0oT0", "Bunch ZPC SUM  Ali Trigger OR AutoTrigger", "ZPC_SUM", "A0oT0");
  addNewHisto("BUNCH", "hbunch-ZEM1_A0oT0", "Bunch ZEM1  Ali Trigger OR AutoTrigger", "ZEM1_TR", "A0oT0");
  addNewHisto("BUNCH", "hbunch-ZEM2_A0oT0", "Bunch ZEM2  Ali Trigger OR AutoTrigger", "ZEM2_TR", "A0oT0");

  addNewHisto("BUNCH", "hbunch-ZNA_TC_TR_A0", "Bunch ZNA TC Trigger Alice", "ZNA_TC_TR", "A0");
  addNewHisto("BUNCH", "hbunch-ZNA_SUM_A0", "Bunch ZNA SUM Trigger Alice", "ZNA_SUM", "A0");
  addNewHisto("BUNCH", "hbunch-ZNC_TC_TR_A0", "Bunch ZNC TC Trigger Alice", "ZNC_TC_TR", "A0");
  addNewHisto("BUNCH", "hbunch-ZNC_SUM_A0", "Bunch ZNC SUM  Trigger Alice", "ZNC_SUM", "A0");
  addNewHisto("BUNCH", "hbunch-ZPA_TC_TR_A0", "Bunch ZPA TC Trigger Alice", "ZPA_TC_TR", "A0");
  addNewHisto("BUNCH", "hbunch-ZPA_SUM_A0", "Bunch ZPA SUM  Trigger Alice", "ZPA_SUM", "A0");
  addNewHisto("BUNCH", "hbunch-ZPC_TC_TR_A0", "Bunch ZPC TC Trigger Alice", "ZPC_TC_TR", "A0");
  addNewHisto("BUNCH", "hbunch-ZPC_SUM_A0", "Bunch ZPC SUM  Trigger Alice", "ZPC_SUM", "A0");
  addNewHisto("BUNCH", "hbunch-ZEM1_A0", "Bunch ZEM1  Trigger Alice", "ZEM1_TR", "A0");
  addNewHisto("BUNCH", "hbunch-ZEM2_A0", "Bunch ZEM2  Trigger Alice", "ZEM2_TR", "A0");

  addNewHisto("BUNCH", "hbunch-ZNA_TC_TR_T0", "Bunch ZNA TC  Auto Trigger", "ZNA_TC_TR", "T0");
  addNewHisto("BUNCH", "hbunch-ZNA_SUM_T0", "Bunch ZNA SUM  Auto Trigger", "ZNA_SUM", "T0");
  addNewHisto("BUNCH", "hbunch-ZNC_TC_TR_T0", "Bunch ZNC TC  Auto Trigger", "ZNC_TC_TR", "T0");
  addNewHisto("BUNCH", "hbunch-ZNC_SUM_T0", "Bunch ZNC SUM  Auto Trigger", "ZNC_SUM", "T0");
  addNewHisto("BUNCH", "hbunch-ZPA_TC_TR_T0", "Bunch ZPA TC  Auto Trigger", "ZPA_TC_TR", "T0");
  addNewHisto("BUNCH", "hbunch-ZPA_SUM_T0", "Bunch ZPA SUM  Auto Trigger", "ZPA_SUM", "T0");
  addNewHisto("BUNCH", "hbunch-ZPC_TC_TR_T0", "Bunch ZPC TC  Auto Trigger", "ZPC_TC_TR", "T0");
  addNewHisto("BUNCH", "hbunch-ZPC_SUM_T0", "Bunch ZPC SUM  Auto Trigger", "ZPC_SUM", "T0");
  addNewHisto("BUNCH", "hbunch-ZEM1_T0", "Bunch ZEM1  Auto Trigger", "ZEM1_TR", "T0");
  addNewHisto("BUNCH", "hbunch-ZEM2_T0", "Bunch ZEM2  Auto Trigger", "ZEM2_TR", "T0");

  if (auto param = mCustomParameters.find("TRASMITTEDCHANNEL"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - TRASMITTEDCHANNEL: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto2D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()), atoi(tokenString.at(3).c_str()), atof(tokenString.at(4).c_str()), atof(tokenString.at(5).c_str()));
  } else {
    setBinHisto2D(8, -0.5, 7.5, 4, -0.5, 3.5);
  }
  addNewHisto("TRASMITTEDCHANNEL", "hchTrasmitted", "Channels Trasmitted", "NONE", "ALL");

  if (auto param = mCustomParameters.find("FIRECHANNEL"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - FIRECHANNELL: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto2D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()), atoi(tokenString.at(3).c_str()), atof(tokenString.at(4).c_str()), atof(tokenString.at(5).c_str()));
  } else {
    setBinHisto2D(8, -0.5, 7.5, 4, -0.5, 3.5);
  }
  addNewHisto("FIRECHANNEL", "hchFired", "Channels Fired", "NONE", "ALL");

  if (auto param = mCustomParameters.find("DATALOSS"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - DATALOSS: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto2D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()), atoi(tokenString.at(3).c_str()), atof(tokenString.at(4).c_str()), atof(tokenString.at(5).c_str()));
  } else {
    setBinHisto2D(8, -0.5, 7.5, 4, -0.5, 3.5);
  }
  addNewHisto("DATALOSS", "hchDataLoss", "Data Loss", "NONE", "ALL");

  if (auto param = mCustomParameters.find("TRIGGER_BIT"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - TRIGGER_BIT: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto2D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()), atoi(tokenString.at(3).c_str()), atof(tokenString.at(4).c_str()), atof(tokenString.at(5).c_str()));
  } else {
    setBinHisto2D(32, -0.5, 31.5, 10, -0.5, 9.5);
  }
  addNewHisto("TRIGGER_BIT", "hchTriggerBits", "Trigger Bits", "NONE", "ALL");

  if (auto param = mCustomParameters.find("TRIGGER_BIT_HIT"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - TRIGGER_BIT_HIT: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto2D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()), atoi(tokenString.at(3).c_str()), atof(tokenString.at(4).c_str()), atof(tokenString.at(5).c_str()));
  } else {
    setBinHisto2D(32, -0.5, 31.5, 10, -0.5, 9.5);
  }
  addNewHisto("TRIGGER_BIT_HIT", "hchTriggerBitsHits", "Trigger Bits Hit", "NONE", "ALL");

  if (auto param = mCustomParameters.find("OVER_BC"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - OVER_BC: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto1D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()));
  } else {
    setBinHisto1D(32, -0.5, 31.5);
  }
  addNewHisto("OVER_BC", "hbcOver", "BC Overflow", "NONE", "ALL");

  if (auto param = mCustomParameters.find("SUMMARYBASELINE"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - SUMMARYBASELINE: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto1D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()));
  } else {
    setBinHisto1D(26, -0.5, 25.5);
  }
  addNewHisto("SUMMARYBASELINE", "hpedSummary", "Baseline Summary", "NONE", "LBC");

  if (auto param = mCustomParameters.find("SUMMARYRATE"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - SUMMARYRATE: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto1D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()));
  } else {
    setBinHisto1D(26, -0.5, 25.5);
  }
  addNewHisto("SUMMARYRATE", "hrateSummary", "Rate Summary (KHz)", "NONE", "LBC");

  if (auto param = mCustomParameters.find("SUMMARY_ALIGN"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - SUMMARY_ALIGN: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto2D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()), atoi(tokenString.at(3).c_str()), atof(tokenString.at(4).c_str()), atof(tokenString.at(5).c_str()));
  } else {
    setBinHisto2D(26, 0.5, 26.5, 12, -0.5, 11.5);
  }
  addNewHisto("SUMMARY_ALIGN", "hAlignPlot", "Alignment Plot", "NONE", "A0oT0");
  addNewHisto("SUMMARY_ALIGN_SHIFT", "hAlignPlotShift", "Alignment Plot", "NONE", "A0oT0");

  if (auto param = mCustomParameters.find("ALIGN_NUM_CYCLE"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter -ALIGN_CYCLE: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    fAlignCycle = atoi(param->second.c_str());
  } else {
    fAlignCycle = 1;
  }

  if (auto param = mCustomParameters.find("ALIGN_NUM_ENTRIES"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter -ALIGN_NUM_ENTRIES: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    fAlignNumEntries = atoi(param->second.c_str());
  } else {
    fAlignNumEntries = 2000;
  }
}

void ZDCRawDataTask::init()
{
  gROOT->SetBatch();
  configureRawDataTask();
  // Word id not present in payload
  mCh.f.fixed_0 = o2::zdc::Id_wn;
  mCh.f.fixed_1 = o2::zdc::Id_wn;
  mCh.f.fixed_2 = o2::zdc::Id_wn;
  // dumpHistoStructure();
}

inline int ZDCRawDataTask::getHPos(uint32_t board, uint32_t ch, int matrix[o2::zdc::NModules][o2::zdc::NChPerModule])
{
  if (board < o2::zdc::NModules && ch < o2::zdc::NChPerModule) {
    return matrix[board][ch];
  } else {
    LOG(error) << "Wrong board " << board << " ch " << ch;
    return -1;
  }
  return -1;
}

int ZDCRawDataTask::processWord(const uint32_t* word)
{
  if (word == nullptr) {
    printf("NULL\n");
    return 1;
  }
  if ((word[0] & 0x3) == o2::zdc::Id_w0) {
    for (int32_t iw = 0; iw < o2::zdc::NWPerGBTW; iw++) {
      mCh.w[0][iw] = word[iw];
    }
  } else if ((word[0] & 0x3) == o2::zdc::Id_w1) {
    if (mCh.f.fixed_0 == o2::zdc::Id_w0) {
      for (int32_t iw = 0; iw < o2::zdc::NWPerGBTW; iw++) {
        mCh.w[1][iw] = word[iw];
      }
    } else {
      LOG(error) << "Wrong word sequence";
      mCh.f.fixed_0 = o2::zdc::Id_wn;
      mCh.f.fixed_1 = o2::zdc::Id_wn;
      mCh.f.fixed_2 = o2::zdc::Id_wn;
    }
  } else if ((word[0] & 0x3) == o2::zdc::Id_w2) {
    if (mCh.f.fixed_0 == o2::zdc::Id_w0 && mCh.f.fixed_1 == o2::zdc::Id_w1) {
      for (int32_t iw = 0; iw < o2::zdc::NWPerGBTW; iw++) {
        mCh.w[2][iw] = word[iw];
      }
      process(mCh);
    } else {
      LOG(error) << "Wrong word sequence";
    }
    mCh.f.fixed_0 = o2::zdc::Id_wn;
    mCh.f.fixed_1 = o2::zdc::Id_wn;
    mCh.f.fixed_2 = o2::zdc::Id_wn;
  } else {
    // Word not present in payload
    LOG(error) << "Event format error";
    return 1;
  }
  return 0;
}

int ZDCRawDataTask::process(const o2::zdc::EventChData& ch)
{
  static constexpr int last_bc = o2::constants::lhc::LHCMaxBunches - 1;
  union {
    uint16_t uns;
    int16_t sig;
  } word16;
  int flag = 0;
  // Not empty event
  auto f = ch.f;
  uint16_t us[12];
  int16_t s[12];
  us[0] = f.s00;
  us[1] = f.s01;
  us[2] = f.s02;
  us[3] = f.s03;
  us[4] = f.s04;
  us[5] = f.s05;
  us[6] = f.s06;
  us[7] = f.s07;
  us[8] = f.s08;
  us[9] = f.s09;
  us[10] = f.s10;
  us[11] = f.s11;
  int itb = 4 * (int)f.board + (int)f.ch;
  if (f.Hit == 1 && fFireChannel) {
    fFireChannel->Fill(f.board, f.ch);
  }
  if (fTrasmChannel) {
    fTrasmChannel->Fill(f.board, f.ch);
  }

  if (f.Alice_0 || f.Auto_0 || f.Alice_1 || f.Auto_1 || f.Alice_2 || f.Auto_2 || f.Alice_3 || f.Auto_3) {
    for (int j = 0; j < (int)fMatrixHistoSignal[f.board][f.ch].size(); j++) {
      for (int32_t i = 0; i < 12; i++) {
        if (us[i] > o2::zdc::ADCMax) {
          s[i] = us[i] - o2::zdc::ADCRange;
        } else {
          s[i] = us[i];
        }
        if ((fMatrixHistoSignal[f.board][f.ch].at(j).condHisto.at(0).compare("AoT") == 0) && (f.Alice_3 || f.Auto_3)) {
          fMatrixHistoSignal[f.board][f.ch].at(j).histo->Fill(i - 36., double(s[i]));
        }
        if ((fMatrixHistoSignal[f.board][f.ch].at(j).condHisto.at(0).compare("AoT") == 0) && (f.Alice_2 || f.Auto_2)) {
          fMatrixHistoSignal[f.board][f.ch].at(j).histo->Fill(i - 24., double(s[i]));
        }
        if ((fMatrixHistoSignal[f.board][f.ch].at(j).condHisto.at(0).compare("AoT") == 0) && (f.Alice_1 || f.Auto_1)) {
          fMatrixHistoSignal[f.board][f.ch].at(j).histo->Fill(i - 12., double(s[i]));
        }
        if ((fMatrixHistoSignal[f.board][f.ch].at(j).condHisto.at(0).compare("AoT") == 0) && (f.Alice_0 || f.Auto_0)) {
          fMatrixHistoSignal[f.board][f.ch].at(j).histo->Fill(i + 0., double(s[i]));
          if (f.Auto_0) {
            fMatrixAlign[f.board][f.ch].minSample.vSamples[i].num_entry += 1;
            fMatrixAlign[f.board][f.ch].minSample.vSamples[i].sum += (int)s[i];
            fMatrixAlign[f.board][f.ch].minSample.vSamples[i].mean = (double)fMatrixAlign[f.board][f.ch].minSample.vSamples[i].sum / (double)fMatrixAlign[f.board][f.ch].minSample.vSamples[i].num_entry;
            if (fMatrixAlign[f.board][f.ch].minSample.vSamples[0].num_entry > fAlignNumEntries && fMatrixAlign[f.board][f.ch].minSample.vSamples[i].mean < fMatrixAlign[f.board][f.ch].minSample.min_mean) {
              fMatrixAlign[f.board][f.ch].minSample.id_min_sample = i;
              fMatrixAlign[f.board][f.ch].minSample.min_mean = fMatrixAlign[f.board][f.ch].minSample.vSamples[i].mean;
              fMatrixAlign[f.board][f.ch].minSample.num_entry = fMatrixAlign[f.board][f.ch].minSample.vSamples[i].num_entry;
            }
          }
        }
      }
    }
  }

  if (fNumCycle == fAlignCycle) {
    fSummaryAlignShift->Reset();
    for (int i_mod = 0; i_mod < o2::zdc::NModules; i_mod++) {
      for (int i_ch = 0; i_ch < o2::zdc::NChPerModule; i_ch++) {
        if (fMatrixAlign[i_mod][i_ch].minSample.vSamples[0].num_entry > 0) {
          fSummaryAlign->Fill(fMatrixAlign[i_mod][i_ch].bin, fMatrixAlign[i_mod][i_ch].minSample.id_min_sample);
          fSummaryAlignShift->Fill(fMatrixAlign[i_mod][i_ch].bin, fMatrixAlign[i_mod][i_ch].minSample.id_min_sample);
        }
      }
    }
    resetAlign();
    fNumCycle = 0;
  }

  if ((f.Alice_0 || f.Auto_0 || f.Alice_1 || f.Auto_1 || f.Alice_2 || f.Auto_2 || f.Alice_3 || f.Auto_3 || f.Auto_m) && fTriggerBits && fTriggerBitsHits) {
    if (f.Alice_3) {
      fTriggerBits->Fill(itb, 9);
      if (f.Hit) {
        if (f.Hit) {
          fTriggerBitsHits->Fill(itb, 9);
        }
      }
    }
    if (f.Alice_2) {
      fTriggerBits->Fill(itb, 8);
      if (f.Hit) {
        fTriggerBitsHits->Fill(itb, 8);
      }
    }
    if (f.Alice_1) {
      fTriggerBits->Fill(itb, 7);
      if (f.Hit) {
        fTriggerBitsHits->Fill(itb, 7);
      }
    }
    if (f.Alice_0) {
      fTriggerBits->Fill(itb, 6);
      if (f.Hit) {
        fTriggerBitsHits->Fill(itb, 6);
      }
    }
    if (f.Auto_3) {
      fTriggerBits->Fill(itb, 5);
      if (f.Hit) {
        fTriggerBitsHits->Fill(itb, 5);
      }
    }
    if (f.Auto_2) {
      fTriggerBits->Fill(itb, 4);
      if (f.Hit) {
        fTriggerBitsHits->Fill(itb, 4);
      }
    }
    if (f.Auto_1) {
      fTriggerBits->Fill(itb, 3);
      if (f.Hit) {
        fTriggerBitsHits->Fill(itb, 3);
      }
    }
    if (f.Auto_0) {
      fTriggerBits->Fill(itb, 2);
      if (f.Hit) {
        fTriggerBitsHits->Fill(itb, 2);
      }
    }
    if (f.Auto_m) {
      fTriggerBits->Fill(itb, 1);
      if (f.Hit) {
        fTriggerBitsHits->Fill(itb, 1);
      }
    }
  }
  if (!(f.Alice_3 || f.Alice_2 || f.Alice_1 || f.Alice_0 || f.Alice_1 || f.Auto_3 || f.Auto_2 || f.Auto_1 || f.Auto_0 || f.Auto_m)) {
    if (fTriggerBits && fTriggerBitsHits) {
      fTriggerBits->Fill(itb, 0);
      if (f.Hit)
        fTriggerBitsHits->Fill(itb, 0);
    }
  }
  // Bunch
  if ((fOverBc) && f.bc >= o2::constants::lhc::LHCMaxBunches) {
    fOverBc->Fill(itb);
  }
  if (f.Alice_0 || f.Auto_0) {
    double bc_d = uint32_t(f.bc / 100);
    double bc_m = uint32_t(f.bc % 100);
    for (int i = 0; i < (int)fMatrixHistoBunch[f.board][f.ch].size(); i++) {
      if (fMatrixHistoBunch[f.board][f.ch].at(i).condHisto.at(0).compare("A0oT0") == 0) {
        fMatrixHistoBunch[f.board][f.ch].at(i).histo->Fill(bc_m, -bc_d);
      }
      if (f.Alice_0 && fMatrixHistoBunch[f.board][f.ch].at(i).condHisto.at(0).compare("A0") == 0) {
        fMatrixHistoBunch[f.board][f.ch].at(i).histo->Fill(bc_m, -bc_d);
      }
      if (f.Auto_0 && fMatrixHistoBunch[f.board][f.ch].at(i).condHisto.at(0).compare("T0") == 0) {
        fMatrixHistoBunch[f.board][f.ch].at(i).histo->Fill(bc_m, -bc_d);
      }
    }
  }
  if (f.bc == last_bc) {
    // Fill Baseline
    // int32_t offset =(int32_t) f.offset;
    // if (offset > 32768) offset = offset - 32768;
    word16.uns = f.offset;
    // if (word16.sig < 0) word16.sig = word16.sig + 32768;
    for (int i = 0; i < (int)fMatrixHistoBaseline[f.board][f.ch].size(); i++) {
      fMatrixHistoBaseline[f.board][f.ch].at(i).histo->Fill(word16.sig / 12.);
    }

    // Fill Counts

    if (fDataLoss && (f.dLoss)) {
      fDataLoss->Fill(f.board, f.ch);
    }

    for (int i = 0; i < (int)fMatrixHistoCounts[f.board][f.ch].size(); i++) {
      fMatrixHistoCounts[f.board][f.ch].at(i).histo->Fill(f.hits & 0xfff);
    }

    // Fill Summary
    if (fMapBinNameIdSummaryHisto.find(getNameChannel(f.board, f.ch)) != fMapBinNameIdSummaryHisto.end()) {
      if (fMatrixHistoBaseline[f.board][f.ch].size() > 0) {
        if (fSummaryPedestal && fMatrixHistoBaseline[f.board][f.ch].at(0).histo) {
          fSummaryPedestal->SetBinContent(fMapBinNameIdSummaryHisto[getNameChannel(f.board, f.ch)], fMatrixHistoBaseline[f.board][f.ch].at(0).histo->GetMean());
          fSummaryPedestal->SetBinError(fMapBinNameIdSummaryHisto[getNameChannel(f.board, f.ch)], fMatrixHistoBaseline[f.board][f.ch].at(0).histo->GetMeanError());
        }
        if (fSummaryRate && fMatrixHistoBaseline[f.board][f.ch].at(0).histo) {
          fSummaryRate->SetBinContent(fMapBinNameIdSummaryHisto[getNameChannel(f.board, f.ch)], fMatrixHistoCounts[f.board][f.ch].at(0).histo->GetMean() * 11.2455);
          fSummaryRate->SetBinError(fMapBinNameIdSummaryHisto[getNameChannel(f.board, f.ch)], fMatrixHistoCounts[f.board][f.ch].at(0).histo->GetMeanError());
        }
      }
    }
  }
  return 0;
}

int ZDCRawDataTask::process(const o2::zdc::EventData& ev)
{
  for (int32_t im = 0; im < o2::zdc::NModules; im++) {
    for (int32_t ic = 0; ic < o2::zdc::NChPerModule; ic++) {
      if (ev.data[im][ic].f.fixed_0 == o2::zdc::Id_w0 && ev.data[im][ic].f.fixed_1 == o2::zdc::Id_w1 && ev.data[im][ic].f.fixed_2 == o2::zdc::Id_w2) {
        process(ev.data[im][ic]);
      } else if (ev.data[im][ic].f.fixed_0 == 0 && ev.data[im][ic].f.fixed_1 == 0 && ev.data[im][ic].f.fixed_2 == 0) {
        // Empty channel
      } else {
        LOG(error) << "Data format error";
      }
    }
  }
  return 0;
}

std::string ZDCRawDataTask::getNameChannel(int imod, int ich)
{
  return fNameChannel[imod][ich];
}

void ZDCRawDataTask::setNameChannel(int imod, int ich, std::string namech, int bin)
{
  sSample sample;
  fNameChannel[imod][ich] = namech;
  std::vector<int> coord;
  coord.push_back(imod);
  coord.push_back(ich);
  fMapChNameModCh.insert(std::pair<std::string, std::vector<int>>(namech, coord));

  // init  structure alignment
  fMatrixAlign[imod][ich].name_ch = namech;
  fMatrixAlign[imod][ich].bin = bin;
  fMatrixAlign[imod][ich].minSample.id_min_sample = -1;
  fMatrixAlign[imod][ich].minSample.min_mean = 2048.0;
  fMatrixAlign[imod][ich].minSample.num_entry = 0;
  for (int i = 0; i < 12; i++) {
    sample.id_sample = i;
    sample.num_entry = 0;
    sample.mean = 0.0;
    fMatrixAlign[imod][ich].minSample.vSamples.push_back(sample);
  }
}

bool ZDCRawDataTask::getModAndCh(std::string chName, int* module, int* channel)
{
  if (chName.compare("NONE") == 0) {
    return true;
  }
  if (fMapChNameModCh.find(chName) != fMapChNameModCh.end()) {
    *module = fMapChNameModCh[chName].at(0);
    *channel = fMapChNameModCh[chName].at(1);
    return true;
  }
  return false;
}

void ZDCRawDataTask::setBinHisto1D(int numBinX, double minBinX, double maxBinX)
{
  setNumBinX(numBinX);
  setMinBinX(minBinX);
  setMaxBinX(maxBinX);
}

void ZDCRawDataTask::setBinHisto2D(int numBinX, double minBinX, double maxBinX, int numBinY, double minBinY, double maxBinY)
{
  setNumBinX(numBinX);
  setMinBinX(minBinX);
  setMaxBinX(maxBinX);
  setNumBinY(numBinY);
  setMinBinY(minBinY);
  setMaxBinY(maxBinY);
}

bool ZDCRawDataTask::addNewHisto(std::string type, std::string name, std::string title, std::string chName, std::string condition)
{
  int mod;
  int ch;
  int ih; // index Histogram
  infoHisto1D h1d;
  infoHisto2D h2d;
  // Check if the channel name defined
  if (getModAndCh(chName, &mod, &ch) == true) {
    // Check if channel selected produced data. (_SP spare _OTR only trigger)
    if (chName.find("_SP") != std::string::npos || chName.find("_OTR") != std::string::npos) {
      return false;
    }
    TString hname = TString::Format("%s", name.c_str());
    TString htit = TString::Format("%s", title.c_str());
    // BASELINE
    if (type.compare("BASELINE") == 0) {
      // Check if Histogram Exist
      if (std::find(fNameHisto.begin(), fNameHisto.end(), name) == fNameHisto.end()) {
        fNameHisto.push_back(name);
        h1d.histo = new TH1F(hname, htit, fNumBinX, fMinBinX, fMaxBinX);
        h1d.condHisto.push_back(condition);
        ih = (int)fMatrixHistoBaseline[mod][ch].size();
        fMatrixHistoBaseline[mod][ch].push_back(h1d);

        if (ih < (int)fMatrixHistoBaseline[mod][ch].size()) {
          getObjectsManager()->startPublishing(fMatrixHistoBaseline[mod][ch].at(ih).histo);
          try {
            getObjectsManager()->addMetadata(fMatrixHistoBaseline[mod][ch].at(ih).histo->GetName(), fMatrixHistoBaseline[mod][ch].at(ih).histo->GetName(), "34");
            return true;
          } catch (...) {
            ILOG(Warning, Support) << "Metadata could not be added to " << fMatrixHistoBaseline[mod][ch].at(ih).histo->GetName() << ENDM;
            return false;
          }

          delete h1d.histo;
          h1d.condHisto.clear();
        }
        return true;
      } else {
        for (int i = 0; i < (int)fMatrixHistoBaseline[mod][ch].size(); i++) {
          fMatrixHistoBaseline[mod][ch].at(i).histo->Reset();
        }
        return true;
      }
    }
    // COUNTS
    if (type.compare("COUNTS") == 0) {
      // Check if Histogram Exist
      if (std::find(fNameHisto.begin(), fNameHisto.end(), name) == fNameHisto.end()) {
        fNameHisto.push_back(name);
        h1d.histo = new TH1F(hname, htit, fNumBinX, fMinBinX, fMaxBinX);
        h1d.condHisto.push_back(condition);
        ih = (int)fMatrixHistoCounts[mod][ch].size();
        fMatrixHistoCounts[mod][ch].push_back(h1d);

        if (ih < (int)fMatrixHistoCounts[mod][ch].size()) {
          getObjectsManager()->startPublishing(fMatrixHistoCounts[mod][ch].at(ih).histo);
          try {
            getObjectsManager()->addMetadata(fMatrixHistoCounts[mod][ch].at(ih).histo->GetName(), fMatrixHistoCounts[mod][ch].at(ih).histo->GetName(), "34");
            return true;
          } catch (...) {
            ILOG(Warning, Support) << "Metadata could not be added to " << fMatrixHistoCounts[mod][ch].at(ih).histo->GetName() << ENDM;
            return false;
          }

          delete h1d.histo;
          h1d.condHisto.clear();
        }
        return true;
      } else {
        for (int i = 0; i < (int)fMatrixHistoCounts[mod][ch].size(); i++) {
          fMatrixHistoCounts[mod][ch].at(i).histo->Reset();
        }
        return true;
      }
    }
    // SIGNAL
    if (type.compare("SIGNAL") == 0) {
      // Check if Histogram Exist
      if (std::find(fNameHisto.begin(), fNameHisto.end(), name) == fNameHisto.end()) {
        fNameHisto.push_back(name);
        h2d.histo = new TH2F(hname, htit, fNumBinX, fMinBinX, fMaxBinX, fNumBinY, fMinBinY, fMaxBinY);
        h2d.condHisto.push_back(condition);
        ih = (int)fMatrixHistoSignal[mod][ch].size();
        fMatrixHistoSignal[mod][ch].push_back(h2d);

        if (ih < (int)fMatrixHistoSignal[mod][ch].size()) {
          getObjectsManager()->startPublishing(fMatrixHistoSignal[mod][ch].at(ih).histo);
          try {
            getObjectsManager()->addMetadata(fMatrixHistoSignal[mod][ch].at(ih).histo->GetName(), fMatrixHistoSignal[mod][ch].at(ih).histo->GetName(), "34");
            return true;
          } catch (...) {
            ILOG(Warning, Support) << "Metadata could not be added to " << fMatrixHistoSignal[mod][ch].at(ih).histo->GetName() << ENDM;
            return false;
          }

          delete h2d.histo;
          h2d.condHisto.clear();
        }
        return true;
      } else {
        for (int i = 0; i < (int)fMatrixHistoSignal[mod][ch].size(); i++) {
          fMatrixHistoSignal[mod][ch].at(i).histo->Reset();
        }
        return true;
      }
    }
    // BUNCH
    if (type.compare("BUNCH") == 0) {
      // Check if Histogram Exist
      if (std::find(fNameHisto.begin(), fNameHisto.end(), name) == fNameHisto.end()) {
        fNameHisto.push_back(name);
        h2d.histo = new TH2F(hname, htit, fNumBinX, fMinBinX, fMaxBinX, fNumBinY, fMinBinY, fMaxBinY);
        h2d.condHisto.push_back(condition);
        h2d.histo->SetStats(0);
        ih = (int)fMatrixHistoBunch[mod][ch].size();
        fMatrixHistoBunch[mod][ch].push_back(h2d);

        if (ih < (int)fMatrixHistoBunch[mod][ch].size()) {
          getObjectsManager()->startPublishing(fMatrixHistoBunch[mod][ch].at(ih).histo);
          try {
            getObjectsManager()->addMetadata(fMatrixHistoBunch[mod][ch].at(ih).histo->GetName(), fMatrixHistoBunch[mod][ch].at(ih).histo->GetName(), "34");
            return true;
          } catch (...) {
            ILOG(Warning, Support) << "Metadata could not be added to " << fMatrixHistoBunch[mod][ch].at(ih).histo->GetName() << ENDM;
            return false;
          }

          delete h2d.histo;
          h2d.condHisto.clear();
        }
        return true;
      } else {
        for (int i = 0; i < (int)fMatrixHistoBunch[mod][ch].size(); i++) {
          fMatrixHistoBunch[mod][ch].at(i).histo->Reset();
          fMatrixHistoBunch[mod][ch].at(i).histo->SetStats(0);
        }
        return true;
      }
    }

    if (type.compare("FIRECHANNEL") == 0) {
      fFireChannel = new TH2I(hname, htit, fNumBinX, fMinBinX, fMaxBinX, fNumBinY, fMinBinY, fMaxBinY);
      fFireChannel->SetStats(0);
      getObjectsManager()->startPublishing(fFireChannel);
      try {
        getObjectsManager()->addMetadata(fFireChannel->GetName(), fFireChannel->GetName(), "34");
        return true;
      } catch (...) {
        ILOG(Warning, Support) << "Metadata could not be added to " << fFireChannel->GetName() << ENDM;
        return false;
      }
    }
    if (type.compare("DATALOSS") == 0) {
      fDataLoss = new TH2F(hname, htit, fNumBinX, fMinBinX, fMaxBinX, fNumBinY, fMinBinY, fMaxBinY);
      getObjectsManager()->startPublishing(fDataLoss);
      try {
        getObjectsManager()->addMetadata(fDataLoss->GetName(), fDataLoss->GetName(), "34");
        return true;
      } catch (...) {
        ILOG(Warning, Support) << "Metadata could not be added to " << fDataLoss->GetName() << ENDM;
        return false;
      }
    }
    if (type.compare("TRASMITTEDCHANNEL") == 0) {
      fTrasmChannel = new TH2I(hname, htit, fNumBinX, fMinBinX, fMaxBinX, fNumBinY, fMinBinY, fMaxBinY);
      fTrasmChannel->SetStats(0);
      getObjectsManager()->startPublishing(fTrasmChannel);
      try {
        getObjectsManager()->addMetadata(fTrasmChannel->GetName(), fTrasmChannel->GetName(), "34");
        return true;
      } catch (...) {
        ILOG(Warning, Support) << "Metadata could not be added to " << fTrasmChannel->GetName() << ENDM;
        return false;
      }
    }
    if (type.compare("TRIGGER_BIT") == 0) {
      fTriggerBits = new TH2F(hname, htit, fNumBinX, fMinBinX, fMaxBinX, fNumBinY, fMinBinY, fMaxBinY);
      fTriggerBits->GetYaxis()->SetBinLabel(10, "Alice_3");
      fTriggerBits->GetYaxis()->SetBinLabel(9, "Alice_2");
      fTriggerBits->GetYaxis()->SetBinLabel(8, "Alice_1");
      fTriggerBits->GetYaxis()->SetBinLabel(7, "Alice_0");
      fTriggerBits->GetYaxis()->SetBinLabel(6, "Auto_3");
      fTriggerBits->GetYaxis()->SetBinLabel(5, "Auto_2");
      fTriggerBits->GetYaxis()->SetBinLabel(4, "Auto_1");
      fTriggerBits->GetYaxis()->SetBinLabel(3, "Auto_0");
      fTriggerBits->GetYaxis()->SetBinLabel(2, "Auto_m");
      fTriggerBits->GetYaxis()->SetBinLabel(1, "None");
      for (int im = 0; im < o2::zdc::NModules; im++) {
        for (int ic = 0; ic < o2::zdc::NChPerModule; ic++) {
          fTriggerBits->GetXaxis()->SetBinLabel(im * o2::zdc::NChPerModule + ic + 1, TString::Format("%d%d", im, ic));
        }
      }
      fTriggerBits->SetStats(0);
      getObjectsManager()->startPublishing(fTriggerBits);
      try {
        getObjectsManager()->addMetadata(fTriggerBits->GetName(), fTriggerBits->GetName(), "34");
        return true;
      } catch (...) {
        ILOG(Warning, Support) << "Metadata could not be added to " << fTriggerBits->GetName() << ENDM;
        return false;
      }
    }
    if (type.compare("TRIGGER_BIT_HIT") == 0) {
      fTriggerBitsHits = new TH2F(hname, htit, fNumBinX, fMinBinX, fMaxBinX, fNumBinY, fMinBinY, fMaxBinY);
      fTriggerBitsHits->GetYaxis()->SetBinLabel(10, "Alice_3");
      fTriggerBitsHits->GetYaxis()->SetBinLabel(9, "Alice_2");
      fTriggerBitsHits->GetYaxis()->SetBinLabel(8, "Alice_1");
      fTriggerBitsHits->GetYaxis()->SetBinLabel(7, "Alice_0");
      fTriggerBitsHits->GetYaxis()->SetBinLabel(6, "Auto_3");
      fTriggerBitsHits->GetYaxis()->SetBinLabel(5, "Auto_2");
      fTriggerBitsHits->GetYaxis()->SetBinLabel(4, "Auto_1");
      fTriggerBitsHits->GetYaxis()->SetBinLabel(3, "Auto_0");
      fTriggerBitsHits->GetYaxis()->SetBinLabel(2, "Auto_m");
      fTriggerBitsHits->GetYaxis()->SetBinLabel(1, "None");
      fTriggerBitsHits->SetStats(0);
      for (int im = 0; im < o2::zdc::NModules; im++) {
        for (int ic = 0; ic < o2::zdc::NChPerModule; ic++) {
          fTriggerBitsHits->GetXaxis()->SetBinLabel(im * o2::zdc::NChPerModule + ic + 1, TString::Format("%d%d", im, ic));
        }
      }
      getObjectsManager()->startPublishing(fTriggerBitsHits);
      try {
        getObjectsManager()->addMetadata(fTriggerBitsHits->GetName(), fTriggerBitsHits->GetName(), "34");
        return true;
      } catch (...) {
        ILOG(Warning, Support) << "Metadata could not be added to " << fTriggerBitsHits->GetName() << ENDM;
        return false;
      }
    }
    if (type.compare("OVER_BC") == 0) {
      fOverBc = new TH1F(hname, htit, fNumBinX, fMinBinX, fMaxBinX);
      for (int im = 0; im < o2::zdc::NModules; im++) {
        for (int ic = 0; ic < o2::zdc::NChPerModule; ic++) {
          fOverBc->GetXaxis()->SetBinLabel(im * o2::zdc::NChPerModule + ic + 1, TString::Format("%d%d", im, ic));
        }
      }
      fOverBc->SetStats(0);
      getObjectsManager()->startPublishing(fOverBc);
      try {
        getObjectsManager()->addMetadata(fOverBc->GetName(), fOverBc->GetName(), "34");
        return true;
      } catch (...) {
        ILOG(Warning, Support) << "Metadata could not be added to " << fOverBc->GetName() << ENDM;
        return false;
      }
    }

    if ((type.compare("SUMMARYBASELINE") == 0) || (type.compare("SUMMARYRATE") == 0) || (type.compare("SUMMARY_ALIGN") == 0) || (type.compare("SUMMARY_ALIGN_SHIFT") == 0)) {
      if (type.compare("SUMMARYBASELINE") == 0) {
        fSummaryPedestal = new TH1F(hname, htit, fNumBinX, fMinBinX, fMaxBinX);
        fSummaryPedestal->GetXaxis()->LabelsOption("v");
        fSummaryPedestal->SetStats(0);
      }
      if (type.compare("SUMMARYRATE") == 0) {
        fSummaryRate = new TH1F(hname, htit, fNumBinX, fMinBinX, fMaxBinX);
        fSummaryRate->GetXaxis()->LabelsOption("v");
        fSummaryRate->SetStats(0);
      }
      if (type.compare("SUMMARY_ALIGN") == 0) {
        fSummaryAlign = new TH2D(hname, htit, fNumBinX, fMinBinX, fMaxBinX, fNumBinY, fMinBinY, fMaxBinY);
        fSummaryAlign->GetXaxis()->LabelsOption("v");
        fSummaryAlign->SetStats(0);
      }
      if (type.compare("SUMMARY_ALIGN_SHIFT") == 0) {
        fSummaryAlignShift = new TH2D(hname, htit, fNumBinX, fMinBinX, fMaxBinX, fNumBinY, fMinBinY, fMaxBinY);
        fSummaryAlignShift->GetXaxis()->LabelsOption("v");
        fSummaryAlignShift->SetStats(0);
      }
      int i = 0;
      for (uint32_t imod = 0; imod < o2::zdc::NModules; imod++) {
        for (uint32_t ich = 0; ich < o2::zdc::NChPerModule; ich++) {
          chName = getNameChannel(imod, ich);
          if (chName.find("_SP") != std::string::npos || chName.find("_OTR") != std::string::npos) {
            continue;
          } else {
            i++;
            if (type.compare("SUMMARYBASELINE") == 0) {
              fSummaryPedestal->GetXaxis()->SetBinLabel(i, TString::Format("%s", getNameChannel(imod, ich).c_str()));
            }
            if (type.compare("SUMMARYRATE") == 0) {
              fSummaryRate->GetXaxis()->SetBinLabel(i, TString::Format("%s", getNameChannel(imod, ich).c_str()));
            }
            if (type.compare("SUMMARY_ALIGN") == 0) {
              fSummaryAlign->GetXaxis()->SetBinLabel(fMatrixAlign[imod][ich].bin, TString::Format("%s", fMatrixAlign[imod][ich].name_ch.c_str()));
            }
            if (type.compare("SUMMARY_ALIGN_SHIFT") == 0) {
              fSummaryAlignShift->GetXaxis()->SetBinLabel(fMatrixAlign[imod][ich].bin, TString::Format("%s", fMatrixAlign[imod][ich].name_ch.c_str()));
            }
            fMapBinNameIdSummaryHisto.insert(std::pair<std::string, int>(chName, i));
          }
        }
      }
      if (type.compare("SUMMARYBASELINE") == 0) {
        getObjectsManager()->startPublishing(fSummaryPedestal);
      }
      if (type.compare("SUMMARYRATE") == 0) {
        getObjectsManager()->startPublishing(fSummaryRate);
      }
      if (type.compare("SUMMARY_ALIGN") == 0) {
        getObjectsManager()->startPublishing(fSummaryAlign);
      }
      if (type.compare("SUMMARY_ALIGN_SHIFT") == 0) {
        getObjectsManager()->startPublishing(fSummaryAlignShift);
      }
      try {
        if (type.compare("SUMMARYBASELINE") == 0) {
          getObjectsManager()->addMetadata(fSummaryPedestal->GetName(), fSummaryPedestal->GetName(), "34");
        }
        if (type.compare("SUMMARYRATE") == 0) {
          getObjectsManager()->addMetadata(fSummaryRate->GetName(), fSummaryRate->GetName(), "34");
        }
        if (type.compare("SUMMARY_ALIGN") == 0) {
          getObjectsManager()->addMetadata(fSummaryAlign->GetName(), fSummaryAlign->GetName(), "34");
        }
        if (type.compare("SUMMARY_ALIGN_SHIFT") == 0) {
          getObjectsManager()->addMetadata(fSummaryAlign->GetName(), fSummaryAlignShift->GetName(), "34");
        }
        return true;
      } catch (...) {
        if (type.compare("SUMMARYBASELINE") == 0) {
          ILOG(Warning, Support) << "Metadata could not be added to " << fSummaryPedestal->GetName() << ENDM;
        }
        if (type.compare("SUMMARYRATE") == 0) {
          ILOG(Warning, Support) << "Metadata could not be added to " << fSummaryRate->GetName() << ENDM;
        }
        if (type.compare("SUMMARY_ALIGN") == 0) {
          ILOG(Warning, Support) << "Metadata could not be added to " << fSummaryAlign->GetName() << ENDM;
        }
        if (type.compare("SUMMARY_ALIGN_SHIFT") == 0) {
          ILOG(Warning, Support) << "Metadata could not be added to " << fSummaryAlignShift->GetName() << ENDM;
        }
        return false;
      }
    }
  }
  return false;
}

// Decode Configuration file

std::vector<std::string> ZDCRawDataTask::tokenLine(std::string Line, std::string Delimiter)
{
  std::string token;
  size_t pos = 0;
  int i = 0;
  std::vector<std::string> stringToken;
  while ((pos = Line.find(Delimiter)) != std::string::npos) {
    token = Line.substr(i, pos);
    stringToken.push_back(token);
    Line.erase(0, pos + Delimiter.length());
  }
  stringToken.push_back(Line);
  return stringToken;
}

std::string ZDCRawDataTask::removeSpaces(std::string s)
{
  std::string result = boost::trim_copy(s);
  while (result.find(" ") != result.npos)
    boost::replace_all(result, " ", "");
  return result;
}

bool ZDCRawDataTask::configureRawDataTask()
{

  std::ifstream file(std::getenv("CONF_HISTO_QCZDCRAW"));
  std::string line;
  std::vector<std::string> tokenString;
  int line_number = 0;
  bool check;
  bool error = true;
  if (!file) {
    initHisto();
  } else {
    ILOG(Debug, Devel) << "initialize ZDC RAW DATA HISTOGRAMS FROM FILE" << ENDM;
    while (getline(file, line)) {
      line_number++;
      tokenString = tokenLine(line, ";");
      // ILOG(Info, Support) << TString::Format("Decode Line number %d %s",line_number, line.c_str()) << ENDM;
      if (line.compare(0, 1, "#") == 0 || line.compare(0, 1, "\n") == 0 || line.compare(0, 1, " ") == 0 || line.compare(0, 1, "\t") == 0 || line.compare(0, 1, "") == 0)
        check = true;
      else {
        check = decodeConfLine(tokenString, line_number);
      }
      if (check == false) {
        error = false;
      }
      tokenString.clear();
    }
  }
  if (error == false) {
    initHisto();
  }
  file.close();
  return true;
}

bool ZDCRawDataTask::decodeConfLine(std::vector<std::string> TokenString, int LineNumber)
{
  // Module Mapping
  // ILOG(Info, Support) << TString::Format("Key Word %s",TokenString.at(0).c_str()) << ENDM;

  if (strcmp(TokenString.at(0).c_str(), "MODULE") == 0) {
    return decodeModule(TokenString, LineNumber);
  }
  // Bin Histograms
  if (TokenString.at(0).compare("BIN") == 0) {
    return decodeBinHistogram(TokenString, LineNumber);
  }
  if (TokenString.at(0).compare("BASELINE") == 0) {
    return decodeBaseline(TokenString, LineNumber);
  }
  if (TokenString.at(0).compare("COUNTS") == 0) {
    return decodeCounts(TokenString, LineNumber);
  }
  if (TokenString.at(0).compare("SIGNAL") == 0) {
    return decodeSignal(TokenString, LineNumber);
  }
  if (TokenString.at(0).compare("BUNCH") == 0) {
    return decodeBunch(TokenString, LineNumber);
  }
  if (TokenString.at(0).compare("TRASMITTEDCHANNEL") == 0) {
    return decodeTrasmittedChannel(TokenString, LineNumber);
  }
  if (TokenString.at(0).compare("FIRECHANNEL") == 0) {
    return decodeFireChannel(TokenString, LineNumber);
  }
  if (TokenString.at(0).compare("DATALOSS") == 0) {
    return decodeDataLoss(TokenString, LineNumber);
  }
  if (TokenString.at(0).compare("TRIGGER_BIT") == 0) {
    return decodeTriggerBitChannel(TokenString, LineNumber);
  }
  if (TokenString.at(0).compare("TRIGGER_BIT_HIT") == 0) {
    return decodeTriggerBitHitChannel(TokenString, LineNumber);
  }
  if (TokenString.at(0).compare("OVER_BC") == 0) {
    return decodeOverBc(TokenString, LineNumber);
  }
  if ((TokenString.at(0).compare("SUMMARYBASELINE") == 0) || (TokenString.at(0).compare("SUMMARYRATE") == 0)) {
    return decodeSummary(TokenString, LineNumber);
  }
  ILOG(Error, Support) << TString::Format("ERROR Line number %d Key word %s does not exist.", LineNumber, TokenString.at(0).c_str()) << ENDM;
  return false;
}

bool ZDCRawDataTask::decodeModule(std::vector<std::string> TokenString, int LineNumber)
{
  int imod = atoi(TokenString.at(1).c_str());
  int ich = 0;
  // Module Number
  if (imod >= o2::zdc::NModules) {
    ILOG(Error, Support) << TString::Format("ERROR Line number %d Module Number %d is too big.", LineNumber, imod) << ENDM;
    return false;
  }
  // Check number channels
  if (TokenString.size() - 2 > o2::zdc::NChPerModule) {
    ILOG(Error, Support) << TString::Format("ERROR Line number %d Insert too channels", LineNumber) << ENDM;
    return false;
  }
  // set Name Channels
  for (int i = 2; i < (int)TokenString.size(); i++) {
    setNameChannel(imod, ich, TokenString.at(i), 0);
    ich = ich + 1;
  }
  return true;
}

bool ZDCRawDataTask::decodeBinHistogram(std::vector<std::string> TokenString, int LineNumber)
{
  if (TokenString.size() == 4) {
    setBinHisto1D(atoi(TokenString.at(1).c_str()), atoi(TokenString.at(2).c_str()), atoi(TokenString.at(3).c_str()));
    return true;
  }
  if (TokenString.size() == 7) {
    setBinHisto2D(atoi(TokenString.at(1).c_str()), atoi(TokenString.at(2).c_str()), atoi(TokenString.at(3).c_str()), atoi(TokenString.at(4).c_str()), atoi(TokenString.at(5).c_str()), atoi(TokenString.at(6).c_str()));
    return true;
  }
  ILOG(Error, Support) << TString::Format("ERROR Line number %d %s;%s;%s;%s BIN is not correct number of paramiter %d", LineNumber, TokenString.at(0).c_str(), TokenString.at(1).c_str(), TokenString.at(2).c_str(), TokenString.at(3).c_str(), (int)TokenString.size()) << ENDM;
  return false;
}

bool ZDCRawDataTask::decodeBaseline(std::vector<std::string> TokenString, int LineNumber)
{
  if (TokenString.size() == 5) {
    return addNewHisto(TokenString.at(0), TokenString.at(1), TokenString.at(2), TokenString.at(3), TokenString.at(4));
  }
  ILOG(Error, Support) << TString::Format("ERROR Line number %d not correct number of paramiter", LineNumber) << ENDM;
  if (checkCondition(TokenString.at(4)) == false) {
    ILOG(Error, Support) << TString::Format("ERROR Line number %d the condition specified don't exist", LineNumber) << ENDM;
  }
  return false;
}

bool ZDCRawDataTask::decodeCounts(std::vector<std::string> TokenString, int LineNumber)
{
  if (TokenString.size() == 5) {
    return addNewHisto(TokenString.at(0), TokenString.at(1), TokenString.at(2), TokenString.at(3), TokenString.at(4));
  }
  ILOG(Error, Support) << TString::Format("ERROR Line number %d not correct number of paramiter", LineNumber) << ENDM;
  if (checkCondition(TokenString.at(4)) == false) {
    ILOG(Error, Support) << TString::Format("ERROR Line number %d the condition specified don't exist", LineNumber) << ENDM;
  }
  return false;
}

bool ZDCRawDataTask::decodeSignal(std::vector<std::string> TokenString, int LineNumber)
{
  if (TokenString.size() == 5) {
    return addNewHisto(TokenString.at(0), TokenString.at(1), TokenString.at(2), TokenString.at(3), TokenString.at(4));
  }
  ILOG(Error, Support) << TString::Format("ERROR Line number %d not correct number of paramiter", LineNumber) << ENDM;
  if (checkCondition(TokenString.at(4)) == false) {
    ILOG(Error, Support) << TString::Format("ERROR Line number %d the condition specified don't exist", LineNumber) << ENDM;
  }
  return false;
}

bool ZDCRawDataTask::decodeBunch(std::vector<std::string> TokenString, int LineNumber)
{
  if (TokenString.size() == 5) {
    return addNewHisto(TokenString.at(0), TokenString.at(1), TokenString.at(2), TokenString.at(3), TokenString.at(4));
  }
  ILOG(Error, Support) << TString::Format("ERROR Line number %d not correct number of paramiter", LineNumber) << ENDM;
  if (checkCondition(TokenString.at(4)) == false) {
    ILOG(Error, Support) << TString::Format("ERROR Line number %d the condition specified don't exist", LineNumber) << ENDM;
  }
  return false;
}

bool ZDCRawDataTask::decodeFireChannel(std::vector<std::string> TokenString, int LineNumber)
{
  if (TokenString.size() == 5) {
    return addNewHisto(TokenString.at(0), TokenString.at(1), TokenString.at(2), TokenString.at(3), TokenString.at(4));
  }
  ILOG(Error, Support) << TString::Format("ERROR Line number %d not correct number of paramiter", LineNumber) << ENDM;
  if (checkCondition(TokenString.at(4)) == false) {
    ILOG(Error, Support) << TString::Format("ERROR Line number %d the condition specified don't exist", LineNumber) << ENDM;
  }
  return false;
}

bool ZDCRawDataTask::decodeDataLoss(std::vector<std::string> TokenString, int LineNumber)
{
  if (TokenString.size() == 5) {
    return addNewHisto(TokenString.at(0), TokenString.at(1), TokenString.at(2), TokenString.at(3), TokenString.at(4));
  }
  ILOG(Error, Support) << TString::Format("ERROR Line number %d not correct number of paramiter", LineNumber) << ENDM;
  if (checkCondition(TokenString.at(4)) == false) {
    ILOG(Error, Support) << TString::Format("ERROR Line number %d the condition specified don't exist", LineNumber) << ENDM;
  }
  return false;
}
bool ZDCRawDataTask::decodeOverBc(std::vector<std::string> TokenString, int LineNumber)
{
  if (TokenString.size() == 5) {
    return addNewHisto(TokenString.at(0), TokenString.at(1), TokenString.at(2), TokenString.at(3), TokenString.at(4));
  }
  ILOG(Error, Support) << TString::Format("ERROR Line number %d not correct number of paramiter", LineNumber) << ENDM;
  if (checkCondition(TokenString.at(4)) == false) {
    ILOG(Error, Support) << TString::Format("ERROR Line number %d the condition specified don't exist", LineNumber) << ENDM;
  }
  return false;
}
bool ZDCRawDataTask::decodeTrasmittedChannel(std::vector<std::string> TokenString, int LineNumber)
{
  if (TokenString.size() == 5) {
    return addNewHisto(TokenString.at(0), TokenString.at(1), TokenString.at(2), TokenString.at(3), TokenString.at(4));
  }
  ILOG(Error, Support) << TString::Format("ERROR Line number %d not correct number of paramiter", LineNumber) << ENDM;
  if (checkCondition(TokenString.at(4)) == false) {
    ILOG(Error, Support) << TString::Format("ERROR Line number %d the condition specified don't exist", LineNumber) << ENDM;
  }
  return false;
}

bool ZDCRawDataTask::decodeTriggerBitChannel(std::vector<std::string> TokenString, int LineNumber)
{
  if (TokenString.size() == 5) {
    return addNewHisto(TokenString.at(0), TokenString.at(1), TokenString.at(2), TokenString.at(3), TokenString.at(4));
  }
  ILOG(Error, Support) << TString::Format("ERROR Line number %d not correct number of paramiter", LineNumber) << ENDM;
  if (checkCondition(TokenString.at(4)) == false) {
    ILOG(Error, Support) << TString::Format("ERROR Line number %d the condition specified don't exist", LineNumber) << ENDM;
  }
  return false;
}

bool ZDCRawDataTask::decodeTriggerBitHitChannel(std::vector<std::string> TokenString, int LineNumber)
{
  if (TokenString.size() == 5) {
    return addNewHisto(TokenString.at(0), TokenString.at(1), TokenString.at(2), TokenString.at(3), TokenString.at(4));
  }
  ILOG(Error, Support) << TString::Format("ERROR Line number %d not correct number of paramiter", LineNumber) << ENDM;
  if (checkCondition(TokenString.at(4)) == false) {
    ILOG(Error, Support) << TString::Format("ERROR Line number %d the condition specified don't exist", LineNumber) << ENDM;
  }
  return false;
}

bool ZDCRawDataTask::decodeSummary(std::vector<std::string> TokenString, int LineNumber)
{
  if (TokenString.size() == 5) {
    return addNewHisto(TokenString.at(0), TokenString.at(1), TokenString.at(2), TokenString.at(3), TokenString.at(4));
  }
  ILOG(Error, Support) << TString::Format("ERROR Line number %d not correct number of paramiter", LineNumber) << ENDM;
  if (checkCondition(TokenString.at(4)) == false) {
    ILOG(Error, Support) << TString::Format("ERROR Line number %d the condition specified don't exist", LineNumber) << ENDM;
  }
  return false;
}

bool ZDCRawDataTask::checkCondition(std::string cond)
{

  if (cond.compare("A0") == 0) {
    return true; // Alice Trigger 0
  }
  if (cond.compare("T0") == 0) {
    return true; // Auto Trigger 0
  }
  if (cond.compare("A0eT0") == 0) {
    return true; // Alice Trigger 0 AND Auto Trigger 0
  }
  if (cond.compare("A0oT0") == 0) {
    return true; // Alice Trigger 0 OR Auto Trigger 0
  }
  if (cond.compare("AoT") == 0) {
    return true; // Alice Trigger 0 OR Auto Trigger 0
  }
  if (cond.compare("LBC") == 0) {
    return true; // Last BC
  }
  if (cond.compare("ALL") == 0) {
    return true; // no trigger
  }
  return false;
}

void ZDCRawDataTask::resetAlign()
{
  for (int i = 0; i < o2::zdc::NModules; i++) {
    for (int j = 0; j < NChPerModule; j++) {
      fMatrixAlign[i][j].minSample.id_min_sample = -1;
      fMatrixAlign[i][j].minSample.min_mean = 2048.0;
      fMatrixAlign[i][j].minSample.num_entry = 0;
      for (int k = 0; k < 12; k++) {
        fMatrixAlign[i][j].minSample.vSamples[k].id_sample = k;
        fMatrixAlign[i][j].minSample.vSamples[k].num_entry = 0;
        fMatrixAlign[i][j].minSample.vSamples[k].sum = 0;
        fMatrixAlign[i][j].minSample.vSamples[k].mean = 0;
      }
    }
  }
}

void ZDCRawDataTask::dumpHistoStructure()
{
  std::ofstream dumpFile;
  dumpFile.open("dumpStructures.txt");
  dumpFile << "Matrix Name Channel \n";
  for (int i = 0; i < o2::zdc::NModules; i++) {
    for (int j = 0; j < o2::zdc::NChPerModule; j++) {
      dumpFile << fNameChannel[i][j] << "  \t";
    }
    dumpFile << "\n";
  }

  dumpFile << "\nChannel Name Coordinate \n";
  for (auto it = fMapChNameModCh.cbegin(); it != fMapChNameModCh.cend(); ++it) {
    dumpFile << it->first << "[" << it->second.at(0) << "][" << it->second.at(1) << "] "
             << "\n";
  }

  dumpFile << "\n Summary Histo Channel Name Index Histogram \n";
  for (auto it = fMapBinNameIdSummaryHisto.cbegin(); it != fMapBinNameIdSummaryHisto.cend(); ++it) {
    dumpFile << it->first << "[" << it->second << "] "
             << "\n";
  }
  dumpFile << "\nMatrix id Histo Baseline \n";
  for (int i = 0; i < o2::zdc::NModules; i++) {
    for (int j = 0; j < o2::zdc::NChPerModule; j++) {
      for (int k = 0; k < (int)fMatrixHistoBaseline[i][j].size(); k++) {
        dumpFile << "[" << i << "][" << j << "] " << fMatrixHistoBaseline[i][j].at(k).histo->GetName() << "  \t";
      }
      dumpFile << "\n";
    }
    dumpFile << "\n";
  }

  dumpFile << "\nMatrix id Histo Counts \n";
  for (int i = 0; i < o2::zdc::NModules; i++) {
    for (int j = 0; j < o2::zdc::NChPerModule; j++) {
      for (int k = 0; k < (int)fMatrixHistoCounts[i][j].size(); k++) {
        dumpFile << "[" << i << "][" << j << "] " << fMatrixHistoCounts[i][j].at(k).histo->GetName() << "  \t";
      }
      dumpFile << "\n";
    }
    dumpFile << "\n";
  }

  dumpFile << "\nMatrix id Histo Signal\n";
  for (int i = 0; i < o2::zdc::NModules; i++) {
    for (int j = 0; j < o2::zdc::NChPerModule; j++) {
      for (int k = 0; k < (int)fMatrixHistoSignal[i][j].size(); k++) {
        dumpFile << "[" << i << "][" << j << "] " << fMatrixHistoSignal[i][j].at(k).histo->GetName() << " Condition " << fMatrixHistoSignal[i][j].at(k).condHisto.at(0) << "  \t";
      }
      dumpFile << "\n";
    }
    dumpFile << "\n";
  }

  dumpFile << "\nMatrix id Histo Bunch \n";
  for (int i = 0; i < o2::zdc::NModules; i++) {
    for (int j = 0; j < o2::zdc::NChPerModule; j++) {
      for (int k = 0; k < (int)fMatrixHistoBunch[i][j].size(); k++) {
        dumpFile << "[" << i << "][" << j << "] " << fMatrixHistoBunch[i][j].at(k).histo->GetName() << "  \t";
      }
      dumpFile << "\n";
    }
    dumpFile << "\n";
  }

  dumpFile << "\nAlign Struct \n";
  for (int i = 0; i < o2::zdc::NModules; i++) {
    for (int j = 0; j < o2::zdc::NChPerModule; j++) {
      dumpFile << "[" << i << "][" << j << "] " << fMatrixAlign[i][j].name_ch << "  \t pos_histo " << fMatrixAlign[i][j].bin << "  \t" << fMatrixAlign[i][j].minSample.id_min_sample << "  \t";
      dumpFile << "\n";
    }
    dumpFile << "\n";
  }

  dumpFile << "\nAlign Struct details  Num Cycle: " << fNumCycle << "\n";
  for (int i = 0; i < o2::zdc::NModules; i++) {
    for (int j = 0; j < o2::zdc::NChPerModule; j++) {
      dumpFile << "[" << i << "][" << j << "] " << fMatrixAlign[i][j].name_ch << "  \t pos_histo " << fMatrixAlign[i][j].bin << "  \t id " << fMatrixAlign[i][j].minSample.id_min_sample << "  \t mean: " << fMatrixAlign[i][j].minSample.min_mean << "  \t entry: " << fMatrixAlign[i][j].minSample.num_entry << "  \t";
      dumpFile << "\n";
      for (int k = 0; k < 12; k++) {
        dumpFile << "\t id [" << k << "] sample " << fMatrixAlign[i][j].minSample.vSamples[k].id_sample << "  \t  mean: " << fMatrixAlign[i][j].minSample.vSamples[k].mean << "  \t  sum: " << fMatrixAlign[i][j].minSample.vSamples[k].sum << "  \t entry: " << fMatrixAlign[i][j].minSample.vSamples[k].num_entry << "  \t";
        dumpFile << "\n";
      }
      dumpFile << "\n";
    }
    dumpFile << "\n";
    dumpFile << "\nAlign Param Num Cycle: " << fAlignCycle << "\n";
    dumpFile << "\nAlign Param Num entries: " << fAlignNumEntries << "\n";
  }
  dumpFile.close();
}

} // namespace o2::quality_control_modules::zdc
