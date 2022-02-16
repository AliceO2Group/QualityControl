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
#include "FairLogger.h"
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
  delete mHistogram;
}

void ZDCRawDataTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize ZDCRawDataTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - myOwnKey: " << param->second << ENDM;
  }
  init();
  mHistogram = new TH1F("eventSize", "eventSize", 20, 0, 30000);
  getObjectsManager()->startPublishing(mHistogram);
  getObjectsManager()->startPublishing(new TH1F("example2", "example2", 20, 0, 30000));
  try {
    getObjectsManager()->addMetadata(mHistogram->GetName(), "custom", "34");
  } catch (...) {
    // some methods can throw exceptions, it is indicated in their doxygen.
    // In case it is recoverable, it is recommended to catch them and do something meaningful.
    // Here we don't care that the metadata was not added and just log the event.
    ILOG(Warning, Support) << "Metadata could not be added to " << mHistogram->GetName() << ENDM;
  }
}

void ZDCRawDataTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity" << activity.mId << ENDM;
  mHistogram->Reset();
}

void ZDCRawDataTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void ZDCRawDataTask::monitorData(o2::framework::ProcessingContext& ctx){

    o2::framework::InputRecord &inputs = ctx.inputs();
    o2::framework::DPLRawParser parser(inputs);
    for (auto it = parser.begin(), end = parser.end(); it != end; ++it) {
        auto const* payload = it.data();
        size_t payloadSize = it.size();
        if (payload != nullptr) {
            mHistogram->Fill((double)payloadSize);
            for (int32_t ip = 0; ip < (int32_t)payloadSize; ip += 16) {
                processWord((const uint32_t*)&payload[ip]);
            }
       }
    }
}

void ZDCRawDataTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void ZDCRawDataTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void ZDCRawDataTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  mHistogram->Reset();
}

void ZDCRawDataTask::setStat(TH1* h)
{
  TString hn = h->GetName();
  h->Draw();
  gPad->Update();
  TPaveStats* st = (TPaveStats*)h->GetListOfFunctions()->FindObject("stats");
  st->SetFillStyle(1001);
  st->SetBorderSize(1);
  if (hn.BeginsWith("hp")) {
    st->SetOptStat(111111);
    st->SetX1NDC(0.1);
    st->SetX2NDC(0.3);
    st->SetY1NDC(0.640);
    st->SetY2NDC(0.9);
  } else if (hn.BeginsWith("hc")) {
    st->SetOptStat(1111);
    st->SetX1NDC(0.799);
    st->SetX2NDC(0.999);
    st->SetY1NDC(0.829);
    st->SetY2NDC(0.999);
  } else if (hn.BeginsWith("hs") || hn.BeginsWith("hb")) {
    st->SetOptStat(11);
    st->SetX1NDC(0.799);
    st->SetX2NDC(0.9995);
    st->SetY1NDC(0.904);
    st->SetY2NDC(0.999);
  }
}

void ZDCRawDataTask::initHisto(){
    ILOG(Info, Support) << "initialize ZDC RAW DATA HISTOGRAMS" << ENDM;
    setNameChannel(0,0,"ZNA_TC_TR");
    setNameChannel(0,1,"ZNA_SUM_SP");
    setNameChannel(0,2,"ZNA_T1");
    setNameChannel(0,3,"ZNA_T2");
    setNameChannel(1,0,"ZNA_TC_OTR");
    setNameChannel(1,1,"ZNA_SUM");
    setNameChannel(1,2,"ZNA_T3");
    setNameChannel(1,3,"ZNA_T4");
    setNameChannel(2,0,"ZNC_TC_TR");
    setNameChannel(2,1,"ZNC_SUM_SP");
    setNameChannel(2,2,"ZNC_T1");
    setNameChannel(2,3,"ZNC_T2");
    setNameChannel(3,0,"ZNC_TC_OTR");
    setNameChannel(3,1,"ZNC_SUM");
    setNameChannel(3,2,"ZNC_T3");
    setNameChannel(3,3,"ZNC_T4");
    setNameChannel(4,0,"ZPA_TC_TR");
    setNameChannel(4,1,"ZEM1_TR");
    setNameChannel(4,2,"ZPA_T1");
    setNameChannel(4,3,"ZPA_T2");
    setNameChannel(5,0,"ZPA_TC_OTR");
    setNameChannel(5,1,"ZPA_SUM");
    setNameChannel(5,2,"ZPA_T3");
    setNameChannel(5,3,"ZPA_T4");
    setNameChannel(6,0,"ZPC_TC_TR");
    setNameChannel(6,1,"ZEM2_TR");
    setNameChannel(6,2,"ZPC_T1");
    setNameChannel(6,3,"ZPC_T2");
    setNameChannel(7,0,"ZPC_TC_OTR");
    setNameChannel(7,1,"ZPC_SUM");
    setNameChannel(7,2,"ZPC_T3");
    setNameChannel(7,3,"ZPC_T4");
    // Histograms Baseline
    setBinHisto1D(16378, -0.125,o2::zdc::ADCMax + 0.125);
    addNewHisto("BASELINE","hped-ZNA_TC_TR","Baseline ZNA TC","ZNA_TC_TR", "LBC");
    addNewHisto("BASELINE","hped-ZNA_T1","Baseline ZNA T1","ZNA_T1","LBC");
    addNewHisto("BASELINE","hped-ZNA_T2","Baseline ZNA T2","ZNA_T2","LBC");
    addNewHisto("BASELINE","hped-ZNA_T3","Baseline ZNA T3","ZNA_T3","LBC");
    addNewHisto("BASELINE","hped-ZNA_T4","Baseline ZNA T4","ZNA_T4","LBC");
    addNewHisto("BASELINE","hped-ZNA_SUM","Baseline ZNA SUM","ZNA_SUM","LBC");

    addNewHisto("BASELINE","hped-ZNC_TC_TR","Baseline ZNC TC","ZNC_TC_TR","LBC");
    addNewHisto("BASELINE","hped-ZNC_T1","Baseline ZNC T1","ZNC_T1","LBC");
    addNewHisto("BASELINE","hped-ZNC_T2","Baseline ZNC T2","ZNC_T2","LBC");
    addNewHisto("BASELINE","hped-ZNC_T3","Baseline ZNC T3","ZNC_T3","LBC");
    addNewHisto("BASELINE","hped-ZNC_T4","Baseline ZNC T4","ZNC_T4","LBC");
    addNewHisto("BASELINE","hped-ZNC_SUM","Baseline ZNC SUM","ZNC_SUM","LBC");

    addNewHisto("BASELINE","hped-ZPA_TC_TR","Baseline ZPA TC","ZPA_TC_TR","LBC");
    addNewHisto("BASELINE","hped-ZPA_T1","Baseline ZPA T1","ZPA_T1","LBC");
    addNewHisto("BASELINE","hped-ZPA_T2","Baseline ZPA T2","ZPA_T2","LBC");
    addNewHisto("BASELINE","hped-ZPA_T3","Baseline ZPA T3","ZPA_T3","LBC");
    addNewHisto("BASELINE","hped-ZPA_T4","Baseline ZPA T4","ZPA_T4","LBC");
    addNewHisto("BASELINE","hped-ZPA_SUM","Baseline ZPA SUM","ZPA_SUM","LBC");

    addNewHisto("BASELINE","hped-ZPC_TC_TR","Baseline ZPC TC","ZPC_TC_TR","LBC");
    addNewHisto("BASELINE","hped-ZPC_T1","Baseline ZPC T1","ZPC_T1","LBC");
    addNewHisto("BASELINE","hped-ZPC_T2","Baseline ZPC T2","ZPC_T2","LBC");
    addNewHisto("BASELINE","hped-ZPC_T3","Baseline ZPC T3","ZPC_T3","LBC");
    addNewHisto("BASELINE","hped-ZPC_T4","Baseline ZPC T4","ZPC_T4","LBC");
    addNewHisto("BASELINE","hped-ZPC_SUM","Baseline ZPC SUM","ZPC_SUM","LBC");

    addNewHisto("BASELINE","hped-ZEM1_TR","Baseline ZEM1","ZEM1_TR","LBC");
    addNewHisto("BASELINE","hped-ZEM2_TR","Baseline ZEM2","ZEM2_TR","LBC");

    // Histograms Counts
    //setBinHisto1D(o2::constants::lhc::LHCMaxBunches + 6, -5.5, o2::constants::lhc::LHCMaxBunches+0.5);
    setBinHisto1D(10, -0.5, 9.5);
    addNewHisto("COUNTS","hcounts-ZNA_TC_TR","Counts ZNA TC","ZNA_TC_TR", "LBC");
    addNewHisto("COUNTS","hcounts-ZNA_T1","Counts ZNA T1","ZNA_T1","LBC");
    addNewHisto("COUNTS","hcounts-ZNA_T2","Counts ZNA T2","ZNA_T2","LBC");
    addNewHisto("COUNTS","hcounts-ZNA_T3","Counts ZNA T3","ZNA_T3","LBC");
    addNewHisto("COUNTS","hcounts-ZNA_T4","Counts ZNA T4","ZNA_T4","LBC");
    addNewHisto("COUNTS","hcounts-ZNA_SUM","Counts ZNA SUM","ZNA_SUM","LBC");

    addNewHisto("COUNTS","hcounts-ZNC_TC_TR","Counts ZNC TC","ZNC_TC_TR","LBC");
    addNewHisto("COUNTS","hcounts-ZNC_T1","Counts ZNC T1","ZNC_T1","LBC");
    addNewHisto("COUNTS","hcounts-ZNC_T2","Counts ZNC T2","ZNC_T2","LBC");
    addNewHisto("COUNTS","hcounts-ZNC_T3","Counts ZNC T3","ZNC_T3","LBC");
    addNewHisto("COUNTS","hcounts-ZNC_T4","Counts ZNC T4","ZNC_T4","LBC");
    addNewHisto("COUNTS","hcounts-ZNC_SUM","Counts ZNC SUM","ZNC_SUM","LBC");

    addNewHisto("COUNTS","hcounts-ZPA_TC_TR","Counts ZPA TC","ZPA_TC_TR","LBC");
    addNewHisto("COUNTS","hcounts-ZPA_T1","Counts ZPA T1","ZPA_T1","LBC");
    addNewHisto("COUNTS","hcounts-ZPA_T2","Counts ZPA T2","ZPA_T2","LBC");
    addNewHisto("COUNTS","hcounts-ZPA_T3","Counts ZPA T3","ZPA_T3","LBC");
    addNewHisto("COUNTS","hcounts-ZPA_T4","Counts ZPA T4","ZPA_T4","LBC");
    addNewHisto("COUNTS","hcounts-ZPA_SUM","Counts ZPA SUM","ZPA_SUM","LBC");

    addNewHisto("COUNTS","hcounts-ZPC_TC_TR","Counts ZPC TC","ZPC_TC_TR","LBC");
    addNewHisto("COUNTS","hcounts-ZPC_T1","Counts ZPC T1","ZPC_T1","LBC");
    addNewHisto("COUNTS","hcounts-ZPC_T2","Counts ZPC T2","ZPC_T2","LBC");
    addNewHisto("COUNTS","hcounts-ZPC_T3","Counts ZPC T3","ZPC_T3","LBC");
    addNewHisto("COUNTS","hcounts-ZPC_T4","Counts ZPC T4","ZPC_T4","LBC");
    addNewHisto("COUNTS","hcounts-ZPC_SUM","Counts ZPC SUM","ZPC_SUM","LBC");

    addNewHisto("COUNTS","hcounts-ZEM1_TR","Counts ZEM1","ZEM1_TR","LBC");
    addNewHisto("COUNTS","hcounts-ZEM2_TR","Counts ZEM2","ZEM2_TR","LBC");
    // Histograms Signal
    int nBCAheadTrig =3;
    int nbx = (nBCAheadTrig + 1) * o2::zdc::NTimeBinsPerBC;
    double xmin = -nBCAheadTrig * o2::zdc::NTimeBinsPerBC - 0.5;
    double xmax = o2::zdc::NTimeBinsPerBC - 0.5;
    setBinHisto2D(nbx, xmin, xmax, o2::zdc::ADCRange, o2::zdc::ADCMin - 0.5, o2::zdc::ADCMax + 0.5);
    addNewHisto("SIGNAL","hsignal-ZNA_TC_TR","Signal ZNA TC","ZNA_TC_TR", "A0oT0");
    addNewHisto("SIGNAL","hsignal-ZNA_T1","Signal ZNA T1","ZNA_T1","A0oT0");
    addNewHisto("SIGNAL","hsignal-ZNA_T2","Signal ZNA T2","ZNA_T2","A0oT0");
    addNewHisto("SIGNAL","hsignal-ZNA_T3","Signal ZNA T3","ZNA_T3","A0oT0");
    addNewHisto("SIGNAL","hsignal-ZNA_T4","Signal ZNA T4","ZNA_T4","A0oT0");
    addNewHisto("SIGNAL","hsignal-ZNA_SUM","Signal ZNA SUM","ZNA_SUM","A0oT0");

    addNewHisto("SIGNAL","hsignal-ZNC_TC_TR","Signal ZNC TC","ZNC_TC_TR","A0oT0");
    addNewHisto("SIGNAL","hsignal-ZNC_T1","Signal ZNC T1","ZNC_T1","A0oT0");
    addNewHisto("SIGNAL","hsignal-ZNC_T2","Signal ZNC T2","ZNC_T2","A0oT0");
    addNewHisto("SIGNAL","hsignal-ZNC_T3","Signal ZNC T3","ZNC_T3","A0oT0");
    addNewHisto("SIGNAL","hsignal-ZNC_T4","Signal ZNC T4","ZNC_T4","A0oT0");
    addNewHisto("SIGNAL","hsignal-ZNC_SUM","Signal ZNC SUM","ZNC_SUM","A0oT0");

    addNewHisto("SIGNAL","hsignal-ZPA_TC_TR","Signal ZPA TC","ZPA_TC_TR","A0oT0");
    addNewHisto("SIGNAL","hsignal-ZPA_T1","Signal ZPA T1","ZPA_T1","A0oT0");
    addNewHisto("SIGNAL","hsignal-ZPA_T2","Signal ZPA T2","ZPA_T2","A0oT0");
    addNewHisto("SIGNAL","hsignal-ZPA_T3","Signal ZPA T3","ZPA_T3","A0oT0");
    addNewHisto("SIGNAL","hsignal-ZPA_T4","Signal ZPA T4","ZPA_T4","A0oT0");
    addNewHisto("SIGNAL","hsignal-ZPA_SUM","Signal ZPA SUM","ZPA_SUM","A0oT0");

    addNewHisto("SIGNAL","hsignal-ZPC_TC_TR","Signal ZPC TC","ZPC_TC_TR","A0oT0");
    addNewHisto("SIGNAL","hsignal-ZPC_T1","Signal ZPC T1","ZPC_T1","A0oT0");
    addNewHisto("SIGNAL","hsignal-ZPC_T2","Signal ZPC T2","ZPC_T2","A0oT0");
    addNewHisto("SIGNAL","hsignal-ZPC_T3","Signal ZPC T3","ZPC_T3","A0oT0");
    addNewHisto("SIGNAL","hsignal-ZPC_T4","Signal ZPC T4","ZPC_T4","A0oT0");
    addNewHisto("SIGNAL","hsignal-ZPC_SUM","Signal ZPC SUM","ZPC_SUM","A0oT0");

    addNewHisto("SIGNAL","hsignal-ZEM1_TR","Signal ZEM1","ZEM1_TR","A0oT0");
    addNewHisto("SIGNAL","hsignal-ZEM2_TR","Signal ZEM2","ZEM2_TR","A0oT0");

    // Histograms Bunch Crossing Maps
    setBinHisto2D(100, -0.5, 99.5, 36, -35.5, 0.5);
    addNewHisto("BUNCH","hbunch-ZNA_TC_TR_Ali0","Bunch ZNA TC","ZNA_TC_TR", "A0");
    addNewHisto("BUNCH","hbunch-ZNA_SUM_Ali0","Bunch ZNA SUM","ZNA_SUM","A0");
    addNewHisto("BUNCH","hbunch-ZNC_TC_TR_Ali0","Bunch ZNC TC","ZNC_TC_TR","A0");
    addNewHisto("BUNCH","hbunch-ZNC_SUM_Ali0","Bunch ZNC SUM","ZNC_SUM","A0");
    addNewHisto("BUNCH","hbunch-ZPA_TC_TR_Ali0","Bunch ZPA TC","ZPA_TC_TR","A0");
    addNewHisto("BUNCH","hbunch-ZPA_SUM_Ali0","Bunch ZPA SUM","ZPA_SUM","A0");
    addNewHisto("BUNCH","hbunch-ZPC_TC_TR_Ali0","Bunch ZPC TC","ZPC_TC_TR","A0");
    addNewHisto("BUNCH","hbunch-ZPC_SUM_Ali0","Bunch ZPC SUM","ZPC_SUM","A0");
    addNewHisto("BUNCH","hbunch-ZNA_TC_TR_Aut0","Bunch ZNA TC","ZNA_TC_TR", "T0");
    addNewHisto("BUNCH","hbunch-ZNA_SUM_Aut0","Bunch ZNA SUM","ZNA_SUM","T0");
    addNewHisto("BUNCH","hbunch-ZNC_TC_TR_Aut0","Bunch ZNC TC","ZNC_TC_TR","T0");
    addNewHisto("BUNCH","hbunch-ZNC_SUM_Aut0","Bunch ZNC SUM","ZNC_SUM","T0");
    addNewHisto("BUNCH","hbunch-ZPA_TC_TR_Aut0","Bunch ZPA TC","ZPA_TC_TR","T0");
    addNewHisto("BUNCH","hbunch-ZPA_SUM_Aut0","Bunch ZPA SUM","ZPA_SUM","T0");
    addNewHisto("BUNCH","hbunch-ZPC_TC_TR_Aut0","Bunch ZPC TC","ZPC_TC_TR","T0");
    addNewHisto("BUNCH","hbunch-ZPC_SUM_Aut0","Bunch ZPC SUM","ZPC_SUM","T0");
    setBinHisto2D(8, -0.5, 7.5, 4, -0.5, 3.5);
    addNewHisto("TRASMITTEDCHANNEL","hchTrasmitted","Channels Trasmitted","NONE","ALL");
    addNewHisto("FIRECHANNEL","hchFired","Channels Fired","NONE","ALL");
    setBinHisto1D(26, -0.5, 26 -0.5);
    addNewHisto("SUMMARYBASELINE","hpedSummary","Baseline Summary","NONE","LBC");
}

void ZDCRawDataTask::init()
{
   gROOT->SetBatch();
   configureRawDataTask();
  // Word id not present in payload
  mCh.f.fixed_0 = o2::zdc::Id_wn;
  mCh.f.fixed_1 = o2::zdc::Id_wn;
  mCh.f.fixed_2 = o2::zdc::Id_wn;
     DumpHistoStructure();
}

inline int ZDCRawDataTask::getHPos(uint32_t board, uint32_t ch, int matrix[o2::zdc::NModules][o2::zdc::NChPerModule])
{
  if (board< o2::zdc::NModules &&  ch< o2::zdc::NChPerModule){
      return matrix[board][ch];
  }
  else {
    LOG(error) << "Wrong board " << board << " ch " << ch;
    return -1;
  }
  return -1;
}

int ZDCRawDataTask::processWord(const uint32_t* word){
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

    //if (f.hit == 1) fFireChannel->Fill(f.board,f.ch);
    fTrasmChannel->Fill(f.board,f.ch);

    for (int32_t i = 0; i < 12; i++) {
        if (us[i] > o2::zdc::ADCMax) {
            s[i] = us[i] - o2::zdc::ADCRange;
        } else {
            s[i] = us[i];
        }
    }

    if (f.Alice_3) {
        for (int j=0; j< (int)fMatrixHistoSignal[f.board][f.ch].size(); j++){
            for (int32_t i = 0; i < 12; i++) {
                fMatrixHistoSignal[f.board][f.ch].at(j).histo->Fill(i - 36., double(s[i]));
            }
        }
    }
    if (f.Alice_2) {
        for (int j=0; j< (int)fMatrixHistoSignal[f.board][f.ch].size(); j++){
            for (int32_t i = 0; i < 12; i++) {
                fMatrixHistoSignal[f.board][f.ch].at(j).histo->Fill(i - 24., double(s[i]));
            }
        }
    }
    if (f.Alice_1 || f.Auto_1) {
        for (int j=0; j< (int)fMatrixHistoSignal[f.board][f.ch].size(); j++){
            for (int32_t i = 0; i < 12; i++) {
                fMatrixHistoSignal[f.board][f.ch].at(j).histo->Fill(i - 12., double(s[i]));
            }
        }
    }
    if (f.Alice_0 || f.Auto_0) {
        for (int j=0; j< (int)fMatrixHistoSignal[f.board][f.ch].size(); j++){
            for (int32_t i = 0; i < 12; i++) {
                fMatrixHistoSignal[f.board][f.ch].at(j).histo->Fill(i + 0., double(s[i]));
            }
        }
        double bc_d = uint32_t(f.bc / 100);
        double bc_m = uint32_t(f.bc % 100);
        for (int i=0; i< (int)fMatrixHistoBunch[f.board][f.ch].size(); i++){
            if (fMatrixHistoBunch[f.board][f.ch].at(i).condHisto.at(0).compare("A0oT0")== 0)
                fMatrixHistoBunch[f.board][f.ch].at(i).histo->Fill(bc_m, -bc_d);
        }
    }
    if (f.Alice_0) {
        double bc_d = uint32_t(f.bc / 100);
        double bc_m = uint32_t(f.bc % 100);
   // Fill Bunch
        for (int i=0; i< (int)fMatrixHistoBunch[f.board][f.ch].size(); i++){
            if (fMatrixHistoBunch[f.board][f.ch].at(i).condHisto.at(0).compare("A0")== 0)
                fMatrixHistoBunch[f.board][f.ch].at(i).histo->Fill(bc_m, -bc_d);
        }
    }
    if (f.Auto_0) {
        double bc_d = uint32_t(f.bc / 100);
        double bc_m = uint32_t(f.bc % 100);
   // Fill Bunch
        for (int i=0; i< (int)fMatrixHistoBunch[f.board][f.ch].size(); i++){
            if (fMatrixHistoBunch[f.board][f.ch].at(i).condHisto.at(0).compare("T0")== 0)
                fMatrixHistoBunch[f.board][f.ch].at(i).histo->Fill(bc_m, -bc_d);
        }
    }
    if (f.bc == last_bc) {
    // Fill Baseline
        int32_t offset = f.offset - 32768;
        double foffset = offset / 8.;
        for (int i=0; i< (int)fMatrixHistoBaseline[f.board][f.ch].size(); i++){
            fMatrixHistoBaseline[f.board][f.ch].at(i).histo->Fill(foffset);
        }

    // Fill Counts
        for (int i=0; i< (int)fMatrixHistoCounts[f.board][f.ch].size(); i++){
            fMatrixHistoCounts[f.board][f.ch].at(i).histo->Fill(f.hits);
        }

   // Fill Summary
        if (fMapBinNameIdSummaryHisto.find(getNameChannel(f.board,f.ch)) != fMapBinNameIdSummaryHisto.end()) {
            if (fMatrixHistoBaseline[f.board][f.ch].size()>0){
                fSummaryPedestal->SetBinContent(fMapBinNameIdSummaryHisto[getNameChannel(f.board,f.ch)],fMatrixHistoBaseline[f.board][f.ch].at(0).histo->GetMean());
                fSummaryPedestal->SetBinError(fMapBinNameIdSummaryHisto[getNameChannel(f.board,f.ch)],fMatrixHistoBaseline[f.board][f.ch].at(0).histo->GetMeanError());

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

std::string ZDCRawDataTask::getNameChannel(int imod,int ich){
    return fNameChannel[imod][ich];
}

void ZDCRawDataTask::setNameChannel(int imod, int ich, std::string namech){
    fNameChannel[imod][ich] = namech;
     std::vector<int> coord;
     coord.push_back (imod);
     coord.push_back (ich);
     fMapChNameModCh.insert(std::pair<std::string,std::vector<int>>(namech,coord));
}

bool ZDCRawDataTask::getModAndCh(std::string chName,int * module, int * channel){
    if (chName.compare("NONE")== 0){
        return true;
    }
    if (fMapChNameModCh.find(chName) != fMapChNameModCh.end()) {
        *module=fMapChNameModCh[chName].at(0);
        *channel=fMapChNameModCh[chName].at(1);
    return true;
    }
    return false;
}

void ZDCRawDataTask::setBinHisto1D(int numBinX, double minBinX, double maxBinX){
    setNumBinX(numBinX);
    setMinBinX(minBinX);
    setMaxBinX(maxBinX);
}

void ZDCRawDataTask::setBinHisto2D(int numBinX, double minBinX, double maxBinX, int numBinY, double minBinY, double maxBinY){
    setNumBinX(numBinX);
    setMinBinX(minBinX);
    setMaxBinX(maxBinX);
    setNumBinY(numBinY);
    setMinBinY(minBinY);
    setMaxBinY(maxBinY);
}


bool ZDCRawDataTask::addNewHisto(std::string type,std::string name,std::string title,std::string chName, std::string condition){
    int mod;
    int ch;
    int ih; // index Histogram
    infoHisto1D h1d;
    infoHisto2D h2d;
    // Check if the channel name defined
    if (getModAndCh(chName,&mod, &ch) == true){
    // Check if channel selected produced data. (_SP spare _OTR only trigger)
        if (chName.find("_SP") != std::string::npos || chName.find("_OTR") != std::string::npos)
            return false;
        TString hname = TString::Format("%s",name.c_str());
        TString htit = TString::Format("%s",title.c_str());
//BASELINE
        if ( type.compare("BASELINE")== 0){
    // Check if Histogram Exist
            if (std::find(fNameHisto.begin(), fNameHisto.end(), name) == fNameHisto.end()) {
                fNameHisto.push_back(name);
                ih = fMapBinNameIdSummaryHisto.size();
                fMapBinNameIdSummaryHisto.insert(std::pair<std::string,int>(chName,ih));
                h1d.histo=new TH1F(hname, htit, fNumBinX, fMinBinX,fMaxBinX);
                h1d.condHisto.push_back(condition);
                ih = (int)fMatrixHistoBaseline[mod][ch].size();
                fMatrixHistoBaseline[mod][ch].push_back(h1d);

                if (ih < (int)fMatrixHistoBaseline[mod][ch].size()){
                    getObjectsManager()->startPublishing(fMatrixHistoBaseline[mod][ch].at(ih).histo);
                    try {
                        getObjectsManager()->addMetadata(fMatrixHistoBaseline[mod][ch].at(ih).histo->GetName(), "custom", "34");
                        return true;
                    }
                    catch (...) {
                        ILOG(Warning, Support) << "Metadata could not be added to " << fMatrixHistoBaseline[mod][ch].at(ih).histo->GetName() << ENDM;
                        return false;
                    }

                    delete h1d.histo;
                    h1d.condHisto.clear();
                }
                return true;
            }
            else{
                for (int i=0; i< (int)fMatrixHistoBaseline[mod][ch].size(); i++){
                    fMatrixHistoBaseline[mod][ch].at(i).histo->Reset();
                    }
               return true;
            }
        }
//COUNTS
        if ( type.compare("COUNTS")== 0){
    // Check if Histogram Exist
            if (std::find(fNameHisto.begin(), fNameHisto.end(), name) == fNameHisto.end()) {
                fNameHisto.push_back(name);
                h1d.histo=new TH1F(hname, htit, fNumBinX, fMinBinX,fMaxBinX);
                h1d.condHisto.push_back(condition);
                ih = (int)fMatrixHistoCounts[mod][ch].size();
                fMatrixHistoCounts[mod][ch].push_back(h1d);

                if (ih < (int)fMatrixHistoCounts[mod][ch].size()){
                    getObjectsManager()->startPublishing(fMatrixHistoCounts[mod][ch].at(ih).histo);
                    try {
                        getObjectsManager()->addMetadata(fMatrixHistoCounts[mod][ch].at(ih).histo->GetName(), "custom", "34");
                        return true;
                    }
                    catch (...) {
                        ILOG(Warning, Support) << "Metadata could not be added to " << fMatrixHistoCounts[mod][ch].at(ih).histo->GetName() << ENDM;
                        return false;
                    }

                    delete h1d.histo;
                    h1d.condHisto.clear();
                }
                return true;
            }
            else{
                for (int i=0; i< (int)fMatrixHistoCounts[mod][ch].size(); i++){
                    fMatrixHistoCounts[mod][ch].at(i).histo->Reset();
                    }
               return true;
            }
        }
//SIGNAL
        if ( type.compare("SIGNAL")== 0){
    // Check if Histogram Exist
            if (std::find(fNameHisto.begin(), fNameHisto.end(), name) == fNameHisto.end()) {
                fNameHisto.push_back(name);
                h2d.histo=new TH2F(hname, htit, fNumBinX, fMinBinX,fMaxBinX,fNumBinY, fMinBinY,fMaxBinY);
                h2d.condHisto.push_back(condition);
                ih = (int)fMatrixHistoSignal[mod][ch].size();
                fMatrixHistoSignal[mod][ch].push_back(h2d);

                if (ih < (int)fMatrixHistoSignal[mod][ch].size()){
                    getObjectsManager()->startPublishing(fMatrixHistoSignal[mod][ch].at(ih).histo);
                    try {
                        getObjectsManager()->addMetadata(fMatrixHistoSignal[mod][ch].at(ih).histo->GetName(), "custom", "34");
                        return true;
                    }
                    catch (...) {
                        ILOG(Warning, Support) << "Metadata could not be added to " << fMatrixHistoSignal[mod][ch].at(ih).histo->GetName() << ENDM;
                        return false;
                    }

                    delete h2d.histo;
                    h2d.condHisto.clear();
                }
                return true;
            }
            else{
                for (int i=0; i< (int)fMatrixHistoSignal[mod][ch].size(); i++){
                    fMatrixHistoSignal[mod][ch].at(i).histo->Reset();
                    }
               return true;
            }
        }
// BUNCH
        if ( type.compare("BUNCH")== 0){
    // Check if Histogram Exist
            if (std::find(fNameHisto.begin(), fNameHisto.end(), name) == fNameHisto.end()) {
                fNameHisto.push_back(name);
                h2d.histo=new TH2F(hname, htit, fNumBinX, fMinBinX,fMaxBinX,fNumBinY, fMinBinY,fMaxBinY);
                h2d.condHisto.push_back(condition);
                ih = (int)fMatrixHistoBunch[mod][ch].size();
                fMatrixHistoBunch[mod][ch].push_back(h2d);

                if (ih < (int)fMatrixHistoBunch[mod][ch].size()){
                    getObjectsManager()->startPublishing(fMatrixHistoBunch[mod][ch].at(ih).histo);
                    try {
                        getObjectsManager()->addMetadata(fMatrixHistoBunch[mod][ch].at(ih).histo->GetName(), "custom", "34");
                        return true;
                    }
                    catch (...) {
                        ILOG(Warning, Support) << "Metadata could not be added to " << fMatrixHistoBunch[mod][ch].at(ih).histo->GetName()<< ENDM;
                        return false;
                    }

                    delete h2d.histo;
                    h2d.condHisto.clear();
                }
                return true;
            }
            else{
                for (int i=0; i< (int)fMatrixHistoBunch[mod][ch].size(); i++){
                    fMatrixHistoBunch[mod][ch].at(i).histo->Reset();
                    }
               return true;
            }
        }

        if ( type.compare("FIRECHANNEL")== 0){
            fFireChannel=new TH2F(hname, htit, fNumBinX, fMinBinX,fMaxBinX,fNumBinY, fMinBinY,fMaxBinY);
            getObjectsManager()->startPublishing(fFireChannel);
            try {
              getObjectsManager()->addMetadata(fFireChannel->GetName(), "custom", "34");
              return true;
            }
            catch (...) {
              ILOG(Warning, Support) << "Metadata could not be added to " << fFireChannel->GetName() << ENDM;
              return false;
            }
        }

        if ( type.compare("TRASMITTEDCHANNEL")== 0){
            fTrasmChannel=new TH2F(hname, htit, fNumBinX, fMinBinX,fMaxBinX,fNumBinY, fMinBinY,fMaxBinY);
            getObjectsManager()->startPublishing(fTrasmChannel);
            try {
              getObjectsManager()->addMetadata(fTrasmChannel->GetName(), "custom", "34");
              return true;
            }
            catch (...) {
              ILOG(Warning, Support) << "Metadata could not be added to " << fTrasmChannel->GetName() << ENDM;
              return false;
            }
        }

        if ( type.compare("SUMMARYBASELINE")== 0){
            fSummaryPedestal=new TH1F(hname, htit, fNumBinX, fMinBinX,fMaxBinX);
            fSummaryPedestal->GetXaxis()->LabelsOption ("v");

            int i=0;
            for (uint32_t imod = 0; imod< o2::zdc::NModules;imod++) {
                for (uint32_t ich = 0; ich < o2::zdc::NChPerModule; ich++) {
                     chName = getNameChannel(imod,ich);
                     if (chName.find("_SP") != std::string::npos || chName.find("_OTR") != std::string::npos) {
                        continue;
                     }
                     else{
                         i++;
                         fSummaryPedestal->GetXaxis()->SetBinLabel(i, TString::Format("%s",getNameChannel(imod,ich).c_str()));
                         //fMapBinNameIdSummaryHisto.insert(std::pair<std::string,int>(chName,i));
                         //ILOG(Info, Support) << TString::Format("{%d}[%d][%d] %s ",i,imod,ich,chName.c_str()) << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
                     }
                }
            }
            getObjectsManager()->startPublishing(fSummaryPedestal);
            try {
              getObjectsManager()->addMetadata(fSummaryPedestal->GetName(), "custom", "34");
              return true;
            }
            catch (...) {
              ILOG(Warning, Support) << "Metadata could not be added to " << fSummaryPedestal->GetName() << ENDM;
              return false;
            }
        }

    }
    return false;
}



// Decode Configuration file

std::vector<std::string> ZDCRawDataTask::tokenLine(std::string Line, std::string Delimiter){
    std::string token;
    size_t pos = 0;
    int i=0;
    std::vector<std::string> stringToken;
    while ((pos = Line.find(Delimiter)) != std::string::npos) {
        token = Line.substr(i, pos);
        stringToken.push_back(token);
        Line.erase(0, pos + Delimiter.length());
    }
    stringToken.push_back(Line);
    return stringToken;
}

std::string ZDCRawDataTask::removeSpaces( std::string s )
  {
  std::string result = boost::trim_copy( s );
  while (result.find( " " ) != result.npos)
    boost::replace_all( result, " ", "" );
  return result;
  }

bool ZDCRawDataTask::configureRawDataTask() {

    std::ifstream file(std::getenv("CONF_HISTO_QCZDCRAW"));
    std::string line;
    std::vector<std::string> tokenString;
    int line_number=0;
    bool check;
    bool error = true;
    if(!file){
       initHisto();
        }
    else{
        ILOG(Info, Support) << "initialize ZDC RAW DATA HISTOGRAMS FROM FILE" << ENDM;
        while(getline(file, line)){
            line_number ++;
            tokenString = tokenLine(line,";");
            ILOG(Info, Support) << TString::Format("Decode Line number %d %s",line_number, line.c_str()) << ENDM;
            if (line.compare(0,1,"#") == 0 || line.compare(0,1,"\n") == 0 || line.compare(0,1," ") == 0 || line.compare(0,1,"\t") == 0 || line.compare(0,1,"") == 0  ) check = true;
            else check=decodeConfLine(tokenString,line_number);
            if(check == false){
                error = false;
            }
            tokenString.clear();
        }
    }
    if (error == false) initHisto();
    file.close();
    return true;
}

bool ZDCRawDataTask::decodeConfLine(std::vector<std::string> TokenString,int LineNumber){
    // Module Mapping
    ILOG(Info, Support) << TString::Format("Key Word %s",TokenString.at(0).c_str()) << ENDM;

    if (strcmp(TokenString.at(0).c_str(), "MODULE") == 0) {
        if (decodeModule(TokenString,LineNumber) == false) return false;
        else return true;
    }
    // Bin Histograms
    if (TokenString.at(0).compare("BIN") == 0) {
        if (decodeBinHistogram(TokenString,LineNumber) == false) return false;
        else return true;
    }
    if (TokenString.at(0).compare("BASELINE") == 0) {
        if (decodeBaseline(TokenString,LineNumber) == false) return false;
        else return true;
    }
    if (TokenString.at(0).compare("COUNTS") == 0) {
        if (decodeCounts(TokenString,LineNumber) == false) return false;
        else return true;
    }
    if (TokenString.at(0).compare("SIGNAL") == 0) {
        if (decodeSignal(TokenString,LineNumber) == false) return false;
        else return true;
    }
    if (TokenString.at(0).compare("BUNCH") == 0) {
        if (decodeBunch(TokenString,LineNumber) == false) return false;
        else return true;
    }
    if (TokenString.at(0).compare("TRASMITTEDCHANNEL") == 0) {
        if (decodeTrasmittedChannel(TokenString,LineNumber) == false) return false;
        else return true;
    }
    if (TokenString.at(0).compare("FIRECHANNEL") == 0) {
        if (decodeFireChannel(TokenString,LineNumber) == false) return false;
        else return true;
    }
    if (TokenString.at(0).compare("SUMMARYBASELINE") == 0) {
        if (decodeSummaryBaseline(TokenString,LineNumber) == false) return false;
        else return true;
    }

    ILOG(Error, Support) << TString::Format("ERROR Line number %d Key word %s does not exist.",LineNumber,TokenString.at(0).c_str()) << ENDM;
    return false;
}

bool ZDCRawDataTask::decodeModule(std::vector<std::string> TokenString,int LineNumber){
   int imod = atoi( TokenString.at(1).c_str());
   int ich=0;
   // Module Number
   if (imod >= o2::zdc::NModules){
       ILOG(Error, Support) << TString::Format("ERROR Line number %d Module Number %d is too big.",LineNumber,imod) << ENDM;
       return false;
   }
   // Check number channels
   if (TokenString.size() -2 > o2::zdc::NChPerModule){
       ILOG(Error, Support) << TString::Format("ERROR Line number %d Insert too channels",LineNumber) << ENDM;
       return false;
   }
   // set Name Channels
   for (int i=2;i< (int)TokenString.size(); i++){
       setNameChannel(imod, ich, TokenString.at(i));
       ich=ich+1;
   }
   return true;
}

bool ZDCRawDataTask::decodeBinHistogram(std::vector<std::string> TokenString,int LineNumber){
   if (TokenString.size() == 4){
      setBinHisto1D(atoi( TokenString.at(1).c_str()), atoi( TokenString.at(2).c_str()), atoi( TokenString.at(3).c_str()));
      return true;
   }
   if (TokenString.size() == 7){
       setBinHisto2D(atoi( TokenString.at(1).c_str()), atoi( TokenString.at(2).c_str()), atoi( TokenString.at(3).c_str()), atoi(TokenString.at(4).c_str()), atoi( TokenString.at(5).c_str()), atoi( TokenString.at(6).c_str()));
       return true;
   }
   ILOG(Error, Support) << TString::Format("ERROR Line number %d %s;%s;%s;%s BIN is not correct number of paramiter %d",LineNumber,TokenString.at(0).c_str(),TokenString.at(1).c_str(),TokenString.at(2).c_str(),TokenString.at(3).c_str(), (int)TokenString.size()) << ENDM;
   return false;
}

bool ZDCRawDataTask::decodeBaseline(std::vector<std::string> TokenString,int LineNumber){
   if (TokenString.size() == 5){
      if (addNewHisto(TokenString.at(0),TokenString.at(1),TokenString.at(2),TokenString.at(3),TokenString.at(4))== true) return true;
      else return false;
   }
   ILOG(Error, Support) << TString::Format("ERROR Line number %d not correct number of paramiter",LineNumber) << ENDM;
    if (checkCondition(TokenString.at(4)) == false)  ILOG(Error, Support) << TString::Format("ERROR Line number %d the condition specified don't exist",LineNumber) << ENDM;
   return false;
}

bool ZDCRawDataTask::decodeCounts(std::vector<std::string> TokenString,int LineNumber){
   if (TokenString.size() == 5){
      if (addNewHisto(TokenString.at(0),TokenString.at(1),TokenString.at(2),TokenString.at(3),TokenString.at(4))== true) return true;
      else return false;
   }
   ILOG(Error, Support) << TString::Format("ERROR Line number %d not correct number of paramiter",LineNumber) << ENDM;
    if (checkCondition(TokenString.at(4)) == false)  ILOG(Error, Support) << TString::Format("ERROR Line number %d the condition specified don't exist",LineNumber) << ENDM;
   return false;
}

bool ZDCRawDataTask::decodeSignal(std::vector<std::string> TokenString,int LineNumber){
   if (TokenString.size() == 5){
      if (addNewHisto(TokenString.at(0),TokenString.at(1),TokenString.at(2),TokenString.at(3),TokenString.at(4))== true) return true;
      else return false;
   }
   ILOG(Error, Support) << TString::Format("ERROR Line number %d not correct number of paramiter",LineNumber) << ENDM;
    if (checkCondition(TokenString.at(4)) == false)  ILOG(Error, Support) << TString::Format("ERROR Line number %d the condition specified don't exist",LineNumber) << ENDM;
   return false;
}

bool ZDCRawDataTask::decodeBunch(std::vector<std::string> TokenString,int LineNumber){
   if (TokenString.size() == 5){
      if (addNewHisto(TokenString.at(0),TokenString.at(1),TokenString.at(2),TokenString.at(3),TokenString.at(4))== true) return true;
      else return false;
   }
   ILOG(Error, Support) << TString::Format("ERROR Line number %d not correct number of paramiter",LineNumber) << ENDM;
    if (checkCondition(TokenString.at(4)) == false)  ILOG(Error, Support) << TString::Format("ERROR Line number %d the condition specified don't exist",LineNumber) << ENDM;
   return false;
}

bool ZDCRawDataTask::decodeFireChannel(std::vector<std::string> TokenString,int LineNumber){
   if (TokenString.size() == 5){
      if (addNewHisto(TokenString.at(0),TokenString.at(1),TokenString.at(2),TokenString.at(3),TokenString.at(4))== true) return true;
      else return false;
   }
   ILOG(Error, Support) << TString::Format("ERROR Line number %d not correct number of paramiter",LineNumber) << ENDM;
    if (checkCondition(TokenString.at(4)) == false)  ILOG(Error, Support) << TString::Format("ERROR Line number %d the condition specified don't exist",LineNumber) << ENDM;
   return false;
}

bool ZDCRawDataTask::decodeTrasmittedChannel(std::vector<std::string> TokenString,int LineNumber){
   if (TokenString.size() == 5){
      if (addNewHisto(TokenString.at(0),TokenString.at(1),TokenString.at(2),TokenString.at(3),TokenString.at(4))== true) return true;
      else return false;
   }
   ILOG(Error, Support) << TString::Format("ERROR Line number %d not correct number of paramiter",LineNumber) << ENDM;
    if (checkCondition(TokenString.at(4)) == false)  ILOG(Error, Support) << TString::Format("ERROR Line number %d the condition specified don't exist",LineNumber) << ENDM;
   return false;
}

bool ZDCRawDataTask::decodeSummaryBaseline(std::vector<std::string> TokenString,int LineNumber){
   if (TokenString.size() == 5){
      if (addNewHisto(TokenString.at(0),TokenString.at(1),TokenString.at(2),TokenString.at(3),TokenString.at(4))== true) return true;
      else return false;
   }
   ILOG(Error, Support) << TString::Format("ERROR Line number %d not correct number of paramiter",LineNumber) << ENDM;
    if (checkCondition(TokenString.at(4)) == false)  ILOG(Error, Support) << TString::Format("ERROR Line number %d the condition specified don't exist",LineNumber) << ENDM;
   return false;
}

bool ZDCRawDataTask::checkCondition(std::string cond){
    if (cond.compare("A") == 0) return true;     // All Alice trigger (A0, A1,A2,A3)
    if (cond.compare("T") == 0) return true;     // All Auto trigger (A0, A1,A2,A3)
    if (cond.compare("A0") == 0) return true;    // Alice Trigger 0
    if (cond.compare("T0") == 0) return true;    // Auto Trigger 0
    if (cond.compare("A0eT0") == 0) return true; // Alice Trigger 0 AND Auto Trigger 0
    if (cond.compare("A0oT0") == 0) return true; // Alice Trigger 0 OR Auto Trigger 0
    if (cond.compare("LBC") == 0) return true;   // Last BC
    if (cond.compare("ALL") == 0) return true;   // no trigger
    return false;

}

void ZDCRawDataTask::DumpHistoStructure(){
    std::ofstream dumpFile;
    dumpFile.open("dumpStructures.txt");
    dumpFile << "Matrix Name Channel \n";
    for (int i=0; i<o2::zdc::NModules; i++){
        for (int j=0; j<o2::zdc::NChPerModule; j++){
            dumpFile << fNameChannel[i][j] << "  \t";
        }
        dumpFile << "\n";
    }

    dumpFile << "\nChannel Name Coordinate \n";
    for(auto it = fMapChNameModCh.cbegin(); it != fMapChNameModCh.cend(); ++it)
    {
        dumpFile  << it->first << "["<< it->second.at(0) << "][" << it->second.at(1) << "] " << "\n";
    }

    dumpFile << "\n Summary Histo Channel Name Index Histogram \n";
    for(auto it = fMapBinNameIdSummaryHisto.cbegin(); it != fMapBinNameIdSummaryHisto.cend(); ++it)
    {
        dumpFile  << it->first << "[" << it->second << "] " << "\n";
    }
    dumpFile << "\nMatrix id Histo Baseline \n";
    for (int i=0; i<o2::zdc::NModules; i++){
        for (int j=0; j<o2::zdc::NChPerModule; j++){
            for (int k=0; k< (int)fMatrixHistoBaseline[i][j].size(); k++){
                dumpFile << "["<< i <<"]["<<j<<"] "<< fMatrixHistoBaseline[i][j].at(k).histo->GetName() << "  \t";
            }
            dumpFile << "\n";
        }
        dumpFile << "\n";
    }

    dumpFile << "\nMatrix id Histo Counts \n";
    for (int i=0; i<o2::zdc::NModules; i++){
        for (int j=0; j<o2::zdc::NChPerModule; j++){
            for (int k=0; k< (int)fMatrixHistoCounts[i][j].size(); k++){
                dumpFile << "["<< i <<"]["<<j<<"] "<< fMatrixHistoCounts[i][j].at(k).histo->GetName() << "  \t";
            }
            dumpFile << "\n";
        }
        dumpFile << "\n";
    }

    dumpFile << "\nMatrix id Histo Signal\n";
    for (int i=0; i<o2::zdc::NModules; i++){
        for (int j=0; j<o2::zdc::NChPerModule; j++){
            for (int k=0; k< (int)fMatrixHistoSignal[i][j].size(); k++){
                dumpFile << "["<< i <<"]["<<j<<"] "<< fMatrixHistoSignal[i][j].at(k).histo->GetName() << "  \t";
            }
            dumpFile << "\n";
        }
        dumpFile << "\n";
    }

    dumpFile << "\nMatrix id Histo Bunch \n";
    for (int i=0; i<o2::zdc::NModules; i++){
        for (int j=0; j<o2::zdc::NChPerModule; j++){
            for (int k=0; k< (int)fMatrixHistoBunch[i][j].size(); k++){
                dumpFile << "["<< i <<"]["<<j<<"] "<< fMatrixHistoBunch[i][j].at(k).histo->GetName() << "  \t";
            }
            dumpFile << "\n";
        }
        dumpFile << "\n";
    }
    dumpFile.close();
}

} // namespace o2::quality_control_modules::zdc

