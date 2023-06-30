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
/// \file   ZDCRecDataTask.cxx
/// \author Carlo Puggioni
///

#include <TCanvas.h>
#include <TH1.h>

#include "QualityControl/QcInfoLogger.h"
#include "ZDC/ZDCRecDataTask.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/DataRefUtils.h>
#include <fairlogger/Logger.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <ctype.h>
#include <boost/algorithm/string.hpp>
#include <gsl/span>
#include <vector>
#include "DataFormatsZDC/BCData.h"
#include "DataFormatsZDC/ChannelData.h"
#include "DataFormatsZDC/OrbitData.h"
#include "DataFormatsZDC/RecEvent.h"
#include "ZDCBase/ModuleConfig.h"
#include "CommonUtils/NameConf.h"
#include "ZDCReconstruction/RecoConfigZDC.h"
#include "ZDCReconstruction/ZDCEnergyParam.h"
#include "ZDCReconstruction/ZDCTowerParam.h"
#include "DataFormatsZDC/RecEventFlat.h"

using namespace o2::zdc;
namespace o2::quality_control_modules::zdc
{

ZDCRecDataTask::~ZDCRecDataTask()
{
  mVecCh.clear();
  mVecType.clear();
  mNameHisto.clear();
  mHisto1D.clear();
  mHisto2D.clear();
}

void ZDCRecDataTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize ZDCRecDataTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  init();
  reset();
}

void ZDCRecDataTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
}

void ZDCRecDataTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void ZDCRecDataTask::monitorData(o2::framework::ProcessingContext& ctx)
{

  auto bcrec = ctx.inputs().get<gsl::span<o2::zdc::BCRecData>>("zdc-bcrec");
  auto energy = ctx.inputs().get<gsl::span<o2::zdc::ZDCEnergy>>("zdc-energyrec");
  auto tdc = ctx.inputs().get<gsl::span<o2::zdc::ZDCTDCData>>("zdc-tdcrec");
  auto info = ctx.inputs().get<gsl::span<uint16_t>>("zdc-inforec");
  process(bcrec, energy, tdc, info);
}

void ZDCRecDataTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void ZDCRecDataTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void ZDCRecDataTask::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  for (int i = 0; i < (int)mHisto1D.size(); i++) {
    mHisto1D.at(i).histo->Reset();
  }
  for (int i = 0; i < (int)mHisto2D.size(); i++) {
    mHisto2D.at(i).histo->Reset();
  }
}
void ZDCRecDataTask::init()
{
  initVecCh();
  initVecType();
  initHisto();
  // dumpHistoStructure();
}

void ZDCRecDataTask::initVecCh()
{
  // ZNA
  insertChVec("ZNAC");
  insertChVec("ZNA1");
  insertChVec("ZNA2");
  insertChVec("ZNA3");
  insertChVec("ZNA4");
  insertChVec("ZNAS");
  // ZPA
  insertChVec("ZPAC");
  insertChVec("ZPA1");
  insertChVec("ZPA2");
  insertChVec("ZPA3");
  insertChVec("ZPA4");
  insertChVec("ZPAS");
  // ZNC
  insertChVec("ZNCC");
  insertChVec("ZNC1");
  insertChVec("ZNC2");
  insertChVec("ZNC3");
  insertChVec("ZNC4");
  insertChVec("ZNCS");
  // ZPC
  insertChVec("ZPCC");
  insertChVec("ZPC1");
  insertChVec("ZPC2");
  insertChVec("ZPC3");
  insertChVec("ZPC4");
  insertChVec("ZPCS");
  // ZEM
  insertChVec("ZEM1");
  insertChVec("ZEM2");
  // Particular channels
  insertChVec("ZNC-ZNA");
  insertChVec("ZNC+ZNA");
  insertChVec("CH");
  insertChVec("MSG");
  insertChVec("CXZNA");
  insertChVec("CYZNA");
  insertChVec("CXZNC");
  insertChVec("CYZNC");
  insertChVec("CXZPA");
  insertChVec("CXZPC");
}

void ZDCRecDataTask::initVecType()
{
  insertTypeVec("ADC");
  insertTypeVec("TDCV");
  insertTypeVec("TDCA");
  insertTypeVec("BC");
  insertTypeVec("INFO");
}

void ZDCRecDataTask::insertChVec(std::string ch)
{
  mVecCh.push_back(ch);
}

void ZDCRecDataTask::insertTypeVec(std::string type)
{
  mVecType.push_back(type);
}

void ZDCRecDataTask::setBinHisto1D(int numBinX, double minBinX, double maxBinX)
{
  setNumBinX(numBinX);
  setMinBinX(minBinX);
  setMaxBinX(maxBinX);
}

void ZDCRecDataTask::setBinHisto2D(int numBinX, double minBinX, double maxBinX, int numBinY, double minBinY, double maxBinY)
{
  setNumBinX(numBinX);
  setMinBinX(minBinX);
  setMaxBinX(maxBinX);
  setNumBinY(numBinY);
  setMinBinY(minBinY);
  setMaxBinY(maxBinY);
}

void ZDCRecDataTask::dumpHistoStructure()
{
  std::ofstream dumpFile;
  dumpFile.open("dumpStructuresRec.txt");

  dumpFile << "\n Vector Channels\n";
  for (int i = 0; i < (int)mVecCh.size(); i++) {
    dumpFile << mVecCh.at(i) << ", ";
  }
  dumpFile << "\n";

  dumpFile << "\n Vector Type\n";
  for (int i = 0; i < (int)mVecType.size(); i++) {
    dumpFile << mVecType.at(i) << ", ";
  }
  dumpFile << "\n";
  dumpFile << "\n Histos 1D \n";
  for (int i = 0; i < (int)mHisto1D.size(); i++) {
    dumpFile << mHisto1D.at(i).typeh << mHisto1D.at(i).histo->GetName() << " \t" << mHisto1D.at(i).ch << " \t" << mHisto1D.at(i).typech << "\n";
  }
  dumpFile << "\n Histos 2D \n";
  for (int i = 0; i < (int)mHisto2D.size(); i++) {
    dumpFile << mHisto2D.at(i).typeh << mHisto2D.at(i).histo->GetName() << " \t" << mHisto2D.at(i).typech1 << " \t" << mHisto2D.at(i).ch1 << " \t" << mHisto2D.at(i).typech2 << " \t" << mHisto2D.at(i).ch2 << "\n";
  }
  dumpFile << "\n HistoName \n";
  for (int i = 0; i < (int)mNameHisto.size(); i++) {
    dumpFile << mNameHisto.at(i) << "\n";
  }
  dumpFile.close();
}

void ZDCRecDataTask::initHisto()
{
  ILOG(Debug, Devel) << "initialize ZDC REC DATA HISTOGRAMS" << ENDM;
  std::vector<std::string> tokenString;

  if (auto param = mCustomParameters.find("ADC"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - ADC: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto1D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()));
  } else
    setBinHisto1D(1051, -202.5, 4002.5);
  addNewHisto("ADC1D", "h_ADC_ZNA_TC", "ADC ZNA TC (Gev)", "ADC", "ZNAC", "", "", 1);
  addNewHisto("ADC1D", "h_ADC_ZNA_T1", "ADC ZNA T1 (Gev)", "ADC", "ZNA1", "", "", 2);
  addNewHisto("ADC1D", "h_ADC_ZNA_T2", "ADC ZNA T2 (Gev)", "ADC", "ZNA2", "", "", 3);
  addNewHisto("ADC1D", "h_ADC_ZNA_T3", "ADC ZNA T3 (Gev)", "ADC", "ZNA3", "", "", 4);
  addNewHisto("ADC1D", "h_ADC_ZNA_T4", "ADC ZNA T4 (Gev)", "ADC", "ZNA4", "", "", 5);
  addNewHisto("ADC1D", "h_ADC_ZNA_SUM", "ADC ZNA SUM (Gev)", "ADC", "ZNAS", "", "", 6);

  addNewHisto("ADC1D", "h_ADC_ZPA_TC", "ADC ZPA TC (Gev)", "ADC", "ZPAC", "", "", 7);
  addNewHisto("ADC1D", "h_ADC_ZPA_T1", "ADC ZPA T1 (Gev)", "ADC", "ZPA1", "", "", 8);
  addNewHisto("ADC1D", "h_ADC_ZPA_T2", "ADC ZPA T2 (Gev)", "ADC", "ZPA2", "", "", 9);
  addNewHisto("ADC1D", "h_ADC_ZPA_T3", "ADC ZPA T3 (Gev)", "ADC", "ZPA3", "", "", 10);
  addNewHisto("ADC1D", "h_ADC_ZPA_T4", "ADC ZPA T4 (Gev)", "ADC", "ZPA4", "", "", 11);
  addNewHisto("ADC1D", "h_ADC_ZPA_SUM", "ADC ZPA SUM (Gev)", "ADC", "ZPAS", "", "", 12);

  addNewHisto("ADC1D", "h_ADC_ZNC_TC", "ADC ZNC TC (Gev)", "ADC", "ZNCC", "", "", 15);
  addNewHisto("ADC1D", "h_ADC_ZNC_T1", "ADC ZNC T1 (Gev)", "ADC", "ZNC1", "", "", 16);
  addNewHisto("ADC1D", "h_ADC_ZNC_T2", "ADC ZNC T2 (Gev)", "ADC", "ZNC2", "", "", 17);
  addNewHisto("ADC1D", "h_ADC_ZNC_T3", "ADC ZNC T3 (Gev)", "ADC", "ZNC3", "", "", 18);
  addNewHisto("ADC1D", "h_ADC_ZNC_T4", "ADC ZNC T4 (Gev)", "ADC", "ZNC4", "", "", 19);
  addNewHisto("ADC1D", "h_ADC_ZNC_SUM", "ADC ZNC SUM (Gev)", "ADC", "ZNCS", "", "", 20);

  addNewHisto("ADC1D", "h_ADC_ZPC_TC", "ADC ZPC TC (Gev)", "ADC", "ZPCC", "", "", 21);
  addNewHisto("ADC1D", "h_ADC_ZPC_T1", "ADC ZPC T1 (Gev)", "ADC", "ZPC1", "", "", 22);
  addNewHisto("ADC1D", "h_ADC_ZPC_T2", "ADC ZPC T2 (Gev)", "ADC", "ZPC2", "", "", 23);
  addNewHisto("ADC1D", "h_ADC_ZPC_T3", "ADC ZPC T3 (Gev)", "ADC", "ZPC3", "", "", 24);
  addNewHisto("ADC1D", "h_ADC_ZPC_T4", "ADC ZPC T4 (Gev)", "ADC", "ZPC4", "", "", 25);
  addNewHisto("ADC1D", "h_ADC_ZPC_SUM", "ADC ZPC SUM (Gev)", "ADC", "ZPCS", "", "", 26);

  addNewHisto("ADC1D", "h_ADC_ZEM1", "ADC ZEM1 (Gev)", "ADC", "ZEM1", "", "", 13);
  addNewHisto("ADC1D", "h_ADC_ZEM2", "ADC ZEM2 (Gev)", "ADC", "ZEM2", "", "", 14);

  if (auto param = mCustomParameters.find("ADCH"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - ADCH: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto1D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()));
  } else
    setBinHisto1D(1051, -202.5, 4002.5);
  addNewHisto("ADC1D", "h_ADC_ZNA_TC_H", "ADC ZNA TC (Gev) ZOOM", "ADC", "ZNAC", "", "", 0);
  addNewHisto("ADC1D", "h_ADC_ZNA_SUM_H", "ADC ZNA SUM (Gev) ZOOM", "ADC", "ZNAS", "", "", 0);
  addNewHisto("ADC1D", "h_ADC_ZPA_TC_H", "ADC ZPA TC (Gev) ZOOM", "ADC", "ZPAC", "", "", 0);
  addNewHisto("ADC1D", "h_ADC_ZPA_SUM_H", "ADC ZPA SUM (Gev) ZOOM", "ADC", "ZPAS", "", "", 0);
  addNewHisto("ADC1D", "h_ADC_ZNC_TC_H", "ADC ZNC TC (Gev) ZOOM", "ADC", "ZNCC", "", "", 0);
  addNewHisto("ADC1D", "h_ADC_ZNC_SUM_H", "ADC ZNC SUM (Gev) ZOOM", "ADC", "ZNCS", "", "", 0);
  addNewHisto("ADC1D", "h_ADC_ZPC_TC_H", "ADC ZPC TC (Gev) ZOOM", "ADC", "ZPCC", "", "", 0);
  addNewHisto("ADC1D", "h_ADC_ZPC_SUM_H", "ADC ZPC SUM (Gev) ZOOM", "ADC", "ZPCS", "", "", 0);

  if (auto param = mCustomParameters.find("TDCT"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - TDCT: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto1D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()));
  } else
    setBinHisto1D(2500, -5.5, 245.5);
  addNewHisto("TDC1D", "h_TDC_ZNA_TC_V", "TDC Time (ns) ZNA TC", "TDCV", "ZNAC", "", "", 1);
  addNewHisto("TDC1D", "h_TDC_ZNA_SUM_V", "TDC Time (ns) ZNA SUM", "TDCV", "ZNAS", "", "", 2);
  addNewHisto("TDC1D", "h_TDC_ZPA_TC_V", "TDC Time (ns) ZPA TC", "TDCV", "ZPAC", "", "", 3);
  addNewHisto("TDC1D", "h_TDC_ZPA_SUM_V", "TDC Time (ns) ZPA SUM", "TDCV", "ZPAS", "", "", 4);
  addNewHisto("TDC1D", "h_TDC_ZNC_TC_V", "TDC Time (ns) ZNC TC", "TDCV", "ZNCC", "", "", 7);
  addNewHisto("TDC1D", "h_TDC_ZNC_SUM_V", "TDC Time (ns) ZNC SUM", "TDCV", "ZNCS", "", "", 8);
  addNewHisto("TDC1D", "h_TDC_ZPC_TC_V", "TDC Time (ns) ZPC TC", "TDCV", "ZPCC", "", "", 9);
  addNewHisto("TDC1D", "h_TDC_ZPC_SUM_V", "TDC Time (ns) ZPC SUM", "TDCV", "ZPCS", "", "", 10);
  addNewHisto("TDC1D", "h_TDC_ZEM1_V", "TDC Time (ns)  ZEM1", "TDCV", "ZEM1", "", "", 5);
  addNewHisto("TDC1D", "h_TDC_ZEM2_V", "TDC Time (ns)  ZEM2", "TDCV", "ZEM2", "", "", 6);

  if (auto param = mCustomParameters.find("TDCA"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - TDCA: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto1D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()));
  } else
    setBinHisto1D(2000, -0.5, 3999.5);
  addNewHisto("TDC1D", "h_TDC_ZNA_TC_A", "TDC Amplitude ZNA TC", "TDCA", "ZNAC", "", "", 0);
  addNewHisto("TDC1D", "h_TDC_ZNA_SUM_A", "TDC Amplitude ZNA SUM", "TDCA", "ZNAS", "", "", 0);
  addNewHisto("TDC1D", "h_TDC_ZPA_TC_A", "TDC Amplitude ZPA TC", "TDCA", "ZPAC", "", "", 0);
  addNewHisto("TDC1D", "h_TDC_ZPA_SUM_A", "TDC Amplitude ZPA SUM", "TDCA", "ZPAS", "", "", 0);
  addNewHisto("TDC1D", "h_TDC_ZNC_TC_A", "TDC Amplitude ZNC TC", "TDCA", "ZNCC", "", "", 0);
  addNewHisto("TDC1D", "h_TDC_ZNC_SUM_A", "TDC Amplitude ZNC SUM", "TDCA", "ZNCS", "", "", 0);
  addNewHisto("TDC1D", "h_TDC_ZPC_TC_A", "TDC Amplitude ZPC TC", "TDCA", "ZPCC", "", "", 0);
  addNewHisto("TDC1D", "h_TDC_ZPC_SUM_A", "TDC Amplitude ZPC SUM", "TDCA", "ZPCS", "", "", 0);
  addNewHisto("TDC1D", "h_TDC_ZEM1_A", "TDC Amplitude ZEM1", "TDCA", "ZEM1", "", "", 0);
  addNewHisto("TDC1D", "h_TDC_ZEM2_A", "TDC Amplitude ZEM2", "TDCA", "ZEM2", "", "", 0);
  // Centroid ZPA
  if (auto param = mCustomParameters.find("CENTR_ZPA"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - CENTR_ZPA: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto1D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()));
  } else
    setBinHisto1D(2240, 0, 22.4);
  addNewHisto("CENTR_ZPA", "h_CENTR_ZPA", "ZPA Centroid (cm)", "ADC", "CXZPA", "", "", 0);
  // Centroid ZPA
  if (auto param = mCustomParameters.find("CENTR_ZPC"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - CENTR_ZPC: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto1D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()));
  } else
    setBinHisto1D(2240, -22.4, 0);
  addNewHisto("CENTR_ZPC", "h_CENTR_ZPC", "ZPC Centroid (cm)", "ADC", "CXZPC", "", "", 0);

  // 2D Histos

  if (auto param = mCustomParameters.find("ADCSUMvsTC"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - ADCSUMvsTC: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto2D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()), atoi(tokenString.at(3).c_str()), atof(tokenString.at(4).c_str()), atof(tokenString.at(5).c_str()));
  } else
    setBinHisto2D(1051, -202.5, 4002.5, 1051, -202.5, 4002.5);
  addNewHisto("ADCSUMvsTC", "h_ADC_ZNAS_ZNAC", "ADC (Gev) ZNA SUM vs ADC (Gev) ZNA TC", "ADC", "ZNAC", "ADC", "ZNAS", 0);
  addNewHisto("ADCSUMvsTC", "h_ADC_ZPAS_ZPAC", "ADC (Gev) ZPA SUM vs ADC (Gev) ZPA TC", "ADC", "ZPAC", "ADC", "ZPAS", 0);
  addNewHisto("ADCSUMvsTC", "h_ADC_ZNCS_ZNCC", "ADC (Gev) ZNC SUM vs ADC (Gev) ZNC TC", "ADC", "ZNCC", "ADC", "ZNCS", 0);
  addNewHisto("ADCSUMvsTC", "h_ADC_ZPCS_ZPCC", "ADC (Gev) ZPC SUM vs ADC (Gev) ZPC TC", "ADC", "ZPCC", "ADC", "ZPCS", 0);

  if (auto param = mCustomParameters.find("ADCvsTDCT"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - ADCvsTDCT: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto2D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()), atoi(tokenString.at(3).c_str()), atof(tokenString.at(4).c_str()), atof(tokenString.at(5).c_str()));
  } else
    setBinHisto2D(250, -5.5, 24.5, 1051, -202.5, 4002.5);
  addNewHisto("ADCvsTDC", "h_ADC_TDC_ZNAC", "ADC (Gev) ZNA TC vs TDC Time (ns)  ZNA TC", "TDCV", "ZNAC", "ADC", "ZNAC", 0);
  addNewHisto("ADCvsTDC", "h_ADC_TDC_ZNAS", "ADC (Gev) ZNA SUM vs TDC Time (ns) ZNA SUM", "TDCV", "ZNAS", "ADC", "ZNAS", 0);
  addNewHisto("ADCvsTDC", "h_ADC_TDC_ZPAC", "ADC (Gev) ZPA TC vs TDC Time (ns) ZPA TC", "TDCV", "ZPAC", "ADC", "ZPAC", 0);
  addNewHisto("ADCvsTDC", "h_ADC_TDC_ZPAS", "ADC (Gev) ZPA SUM vs TDC Time (ns) ZPA SUM", "TDCV", "ZPAS", "ADC", "ZPAS", 0);
  addNewHisto("ADCvsTDC", "h_ADC_TDC_ZNCC", "ADC (Gev) ZNC TC vs TDC Time (ns) ZNC TC", "TDCV", "ZNCC", "ADC", "ZNCC", 0);
  addNewHisto("ADCvsTDC", "h_ADC_TDC_ZNCS", "ADC (Gev) ZNC SUM vs TDC Time (ns) ZNC SUM", "TDCV", "ZNCS", "ADC", "ZNCS", 0);
  addNewHisto("ADCvsTDC", "h_ADC_TDC_ZPCC", "ADC (Gev) ZPC TC vs TDC Time (ns) ZPC TC", "TDCV", "ZPCC", "ADC", "ZPCC", 0);
  addNewHisto("ADCvsTDC", "h_ADC_TDC_ZPCS", "ADC (Gev) ZPC SUM vs TDC Time (ns) ZPC SUM", "TDCV", "ZPCS", "ADC", "ZPCS", 0);
  addNewHisto("ADCvsTDC", "h_ADC_TDC_ZEM1", "ADC (Gev) ZEM1 vs TDC Time (ns) ZEM1", "TDCV", "ZEM1", "ADC", "ZEM1", 0);
  addNewHisto("ADCvsTDC", "h_ADC_TDC_ZEM2", "ADC (Gev) ZEM2 vs TDC Time (ns) ZEM2", "TDCV", "ZEM2", "ADC", "ZEM2", 0);

  if (auto param = mCustomParameters.find("TDCDIFF"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - TDCDIFF: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto2D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()), atoi(tokenString.at(3).c_str()), atof(tokenString.at(4).c_str()), atof(tokenString.at(5).c_str()));
  } else
    setBinHisto2D(100, -10.5, 10.5, 100, -10.5, 10.5);
  addNewHisto("TDC-DIFF", "h_TDC_ZNC_DIFF_ZNA_ZNC_SUM_ZNA_V", "TDC Time (ns) TDC ZNC + ZNA vs ZNC - ZNA", "TDCV", "ZNC-ZNA", "TDCV", "ZNC+ZNA", 0);

  if (auto param = mCustomParameters.find("TDCAvsTDCT"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - TDCAvsTDCT: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto2D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()), atoi(tokenString.at(3).c_str()), atof(tokenString.at(4).c_str()), atof(tokenString.at(5).c_str()));
  } else
    setBinHisto2D(250, -5.5, 24.5, 2000, -0.5, 3999.5);
  addNewHisto("TDC_T_A", "h_TDC_ZNAC_V_A", "ZNA TC TDC amplitude vs time (ns)", "TDCV", "ZNAC", "TDCA", "ZNAC", 0);
  addNewHisto("TDC_T_A", "h_TDC_ZPAC_V_A", "ZPA TC TDC amplitude vs time (ns)", "TDCV", "ZPAC", "TDCA", "ZPAC", 0);
  addNewHisto("TDC_T_A", "h_TDC_ZNCC_V_A", "ZNC TC TDC amplitude vs time (ns)", "TDCV", "ZNCC", "TDCA", "ZNCC", 0);
  addNewHisto("TDC_T_A", "h_TDC_ZPCC_V_A", "ZPC TC TDC amplitude vs time (ns)", "TDCV", "ZPCC", "TDCA", "ZPCC", 0);
  addNewHisto("TDC_T_A", "h_TDC_ZNAS_V_A", "ZNA SUM TDC amplitude vs time (ns)", "TDCV", "ZNAS", "TDCA", "ZNAS", 0);
  addNewHisto("TDC_T_A", "h_TDC_ZPAS_V_A", "ZPA SUM TDC amplitude vs time (ns)", "TDCV", "ZPAS", "TDCA", "ZPAS", 0);
  addNewHisto("TDC_T_A", "h_TDC_ZNCS_V_A", "ZNC SUM TDC amplitude vs time (ns)", "TDCV", "ZNCS", "TDCA", "ZNCS", 0);
  addNewHisto("TDC_T_A", "h_TDC_ZPCS_V_A", "ZPC SUM TDC amplitude vs time (ns)", "TDCV", "ZPCS", "TDCA", "ZPCS", 0);
  addNewHisto("TDC_T_A", "h_TDC_ZEM1_V_A", "ZEM1 TDC amplitude vs time (ns)", "TDCV", "ZEM1", "TDCA", "ZEM1", 0);
  addNewHisto("TDC_T_A", "h_TDC_ZEM2_V_A", "ZEM2 TDC amplitude vs time (ns)", "TDCV", "ZEM2", "TDCA", "ZEM2", 0);

  // msg histo
  setBinHisto2D(o2::zdc::NChannels, -0.5, o2::zdc::NChannels - 0.5, o2::zdc::MsgEnd, -0.5, o2::zdc::MsgEnd - 0.5);
  addNewHisto("MSG_REC", "h_msg", "Reconstruction messages", "INFO", "CH", "INFO", "MSG", 0);
  int idh_msg = (int)mHisto2D.size() - 1;
  setBinHisto1D(10, -0.5, 9.5);
  addNewHisto("SUMMARY_TDC", "h_summmary_TDC", "Summary TDC", "TDCV", "", "", "", 0);
  mIdhTDC = (int)mHisto1D.size() - 1;
  setBinHisto1D(26, -0.5, 25.5);
  addNewHisto("SUMMARY_ADC", "h_summmary_ADC", "Summary ADC", "ADC", "", "", "", 0);
  mIdhADC = (int)mHisto1D.size() - 1;
  for (int ibx = 1; ibx <= o2::zdc::NChannels; ibx++) {
    mHisto2D.at(idh_msg).histo->GetXaxis()->SetBinLabel(ibx, o2::zdc::ChannelNames[ibx - 1].data());
    mHisto1D.at(mIdhADC).histo->GetXaxis()->SetBinLabel(ibx, o2::zdc::ChannelNames[ibx - 1].data());
  }
  for (int ibx = 0; ibx < mVecTDC.size(); ibx++) {
    mHisto1D.at(mIdhTDC).histo->GetXaxis()->SetBinLabel(ibx + 1, mVecTDC.at(ibx).c_str());
  }
  for (int iby = 1; iby <= o2::zdc::MsgEnd; iby++) {
    mHisto2D.at(idh_msg).histo->GetYaxis()->SetBinLabel(iby, o2::zdc::MsgText[iby - 1].data());
  }
  mHisto2D.at(idh_msg).histo->SetStats(0);
  mHisto1D.at(mIdhTDC).histo->LabelsOption("v");
  mHisto1D.at(mIdhTDC).histo->SetStats(0);
  mHisto1D.at(mIdhADC).histo->LabelsOption("v");
  mHisto1D.at(mIdhADC).histo->SetStats(0);
  // Centroid ZNA
  if (auto param = mCustomParameters.find("CENTR_ZNA"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - CENTR_ZNA: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto2D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()), atoi(tokenString.at(3).c_str()), atof(tokenString.at(4).c_str()), atof(tokenString.at(5).c_str()));
  } else
    setBinHisto2D(200, -2, 2, 200, -2, 2);
  addNewHisto("CENTR_ZNA", "h_CENTR_ZNA", "ZNA Centroid (cm)", "ADC", "CXZNA", "ADC", "CYZNA", 0);
  // Centroid ZNC
  if (auto param = mCustomParameters.find("CENTR_ZNC"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - CENTR_ZNC: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto2D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()), atoi(tokenString.at(3).c_str()), atof(tokenString.at(4).c_str()), atof(tokenString.at(5).c_str()));
  } else
    setBinHisto2D(200, -2, 2, 200, -2, 2);
  addNewHisto("CENTR_ZNC", "h_CENTR_ZNC", "ZNC Centroid (cm)", "ADC", "CXZNC", "ADC", "CYZNC", 0);
}

bool ZDCRecDataTask::add1DHisto(std::string typeH, std::string name, std::string title, std::string typeCh1, std::string ch1, int bin)
{

  TString hname = TString::Format("%s", name.c_str());
  TString htit = TString::Format("%s", title.c_str());
  mNameHisto.push_back(name);
  sHisto1D h1d;
  h1d.histo = new TH1F(hname, htit, fNumBinX, fMinBinX, fMaxBinX);
  h1d.typeh = typeH;
  h1d.typech = typeCh1;
  h1d.ch = ch1;
  h1d.bin = bin;
  int ih = (int)mHisto1D.size();
  mHisto1D.push_back(h1d);
  h1d.typeh.clear();
  h1d.typech.clear();
  h1d.ch.clear();
  // delete h1d.histo;
  if (ih < (int)mHisto1D.size()) {
    getObjectsManager()->startPublishing(mHisto1D.at(ih).histo);
    try {
      getObjectsManager()->addMetadata(mHisto1D.at(ih).histo->GetName(), mHisto1D.at(ih).histo->GetName(), "34");
      return true;
    } catch (...) {
      ILOG(Warning, Support) << "Metadata could not be added to " << mHisto1D.at(ih).histo->GetName() << ENDM;
      return false;
    }
  } else
    return false;
}

bool ZDCRecDataTask::add2DHisto(std::string typeH, std::string name, std::string title, std::string typeCh1, std::string ch1, std::string typeCh2, std::string ch2)
{
  TString hname = TString::Format("%s", name.c_str());
  TString htit = TString::Format("%s", title.c_str());
  mNameHisto.push_back(name);
  sHisto2D h2d;
  h2d.histo = new TH2F(hname, htit, fNumBinX, fMinBinX, fMaxBinX, fNumBinY, fMinBinY, fMaxBinY);
  h2d.typeh = typeH;
  h2d.typech1 = typeCh1;
  h2d.ch1 = ch1;
  h2d.typech2 = typeCh2;
  h2d.ch2 = ch2;
  int ih = (int)mHisto2D.size();
  mHisto2D.push_back(h2d);
  h2d.typeh.clear();
  h2d.typech1.clear();
  h2d.typech2.clear();
  h2d.ch1.clear();
  h2d.ch2.clear();
  h2d.typeh.clear();
  // delete h1d.histo;
  if (ih < (int)mHisto2D.size()) {
    getObjectsManager()->startPublishing(mHisto2D.at(ih).histo);
    try {
      getObjectsManager()->addMetadata(mHisto2D.at(ih).histo->GetName(), mHisto2D.at(ih).histo->GetName(), "34");
      return true;
    } catch (...) {
      ILOG(Warning, Support) << "Metadata could not be added to " << mHisto2D.at(ih).histo->GetName() << ENDM;
      return false;
    }
  } else
    return false;
}

bool ZDCRecDataTask::addNewHisto(std::string typeH, std::string name, std::string title, std::string typeCh1, std::string ch1, std::string typeCh2, std::string ch2, int bin)
{
  // Check if Histogram Exist
  if (std::find(mNameHisto.begin(), mNameHisto.end(), name) == mNameHisto.end()) {

    // ADC 1D (ENERGY) OR TDC 1D
    if (typeH.compare("ADC1D") == 0 || typeH.compare("TDC1D") == 0 || typeH.compare("CENTR_ZPA") == 0 || typeH.compare("CENTR_ZPC") == 0 || typeH.compare("SUMMARY_TDC") == 0 || typeH.compare("SUMMARY_ADC") == 0) {
      if (add1DHisto(typeH, name, title, typeCh1, ch1, bin))
        return true;
      else
        return false;
    } else if (typeH.compare("ADCSUMvsTC") == 0 || typeH.compare("ADCvsTDC") == 0 || typeH.compare("TDC-DIFF") == 0 || typeH.compare("TDC_T_A") == 0 || typeH.compare("MSG_REC") == 0 || typeH.compare("CENTR_ZNA") == 0 || typeH.compare("CENTR_ZNC") == 0) {
      if (add2DHisto(typeH, name, title, typeCh1, ch1, typeCh2, ch2))
        return true;
      else
        return false;
    } else
      return false;
  } else {
    reset();
    return true;
  }
  return false;
}

int ZDCRecDataTask::process(const gsl::span<const o2::zdc::BCRecData>& RecBC,
                            const gsl::span<const o2::zdc::ZDCEnergy>& Energy,
                            const gsl::span<const o2::zdc::ZDCTDCData>& TDCData,
                            const gsl::span<const uint16_t>& Info)
{
  LOG(info) << "o2::zdc::InterCalibEPN processing " << RecBC.size();
  float x, y;
  mEv.init(RecBC, Energy, TDCData, Info);
  while (mEv.next()) {
    // Histo 1D
    for (int i = 0; i < (int)mHisto1D.size(); i++) {
      // Fill ADC 1D
      if (mHisto1D.at(i).typeh.compare("ADC1D") == 0 && mHisto1D.at(i).typech.compare("ADC") == 0) {
        mHisto1D.at(i).histo->Fill(getADCRecValue(mHisto1D.at(i).typech, mHisto1D.at(i).ch));
        mHisto1D.at(mIdhADC).histo->SetBinContent(mHisto1D.at(i).bin, mHisto1D.at(i).histo->GetMean());
      }

      // Fill TDC 1D
      if (mHisto1D.at(i).typeh.compare("TDC1D") == 0 && (mHisto1D.at(i).typech.compare("TDCV") == 0 || mHisto1D.at(i).typech.compare("TDCA") == 0)) {
        int tdcid = getIdTDCch(mHisto1D.at(i).typech, mHisto1D.at(i).ch);
        auto nhitv = mEv.NtdcV(tdcid);
        if (mEv.NtdcA(tdcid) == nhitv && nhitv > 0) {
          for (int ihit = 0; ihit < nhitv; ihit++) {
            if (mHisto1D.at(i).typech.compare("TDCV") == 0)
              mHisto1D.at(i).histo->Fill(mEv.tdcV(tdcid, ihit));
            if (mHisto1D.at(i).typech.compare("TDCA") == 0)
              mHisto1D.at(i).histo->Fill(mEv.tdcA(tdcid, ihit));
          }
          mHisto1D.at(mIdhTDC).histo->SetBinContent(mHisto1D.at(i).bin, mHisto1D.at(i).histo->GetMean());
        }
      }
      // Fill CENTROID ZP
      if (mHisto1D.at(i).typeh.compare("CENTR_ZPA") == 0 && mHisto1D.at(i).typech.compare("ADC") == 0)
        mHisto1D.at(i).histo->Fill(mEv.xZPA());
      if (mHisto1D.at(i).typeh.compare("CENTR_ZPC") == 0 && mHisto1D.at(i).typech.compare("ADC") == 0)
        mHisto1D.at(i).histo->Fill(mEv.xZPC());
    } // for histo 1D

    // Histo 2D
    for (int i = 0; i < (int)mHisto2D.size(); i++) {
      if (mHisto2D.at(i).typeh.compare("ADCSUMvsTC") == 0 && mHisto2D.at(i).typech1.compare("ADC") == 0 && mHisto2D.at(i).typech2.compare("ADC") == 0)
        mHisto2D.at(i).histo->Fill((Double_t)getADCRecValue(mHisto2D.at(i).typech1, mHisto2D.at(i).ch1), getADCRecValue(mHisto2D.at(i).typech2, mHisto2D.at(i).ch2));

      if (mHisto2D.at(i).typeh.compare("ADCvsTDC") == 0 && mHisto2D.at(i).typech1.compare("TDCV") == 0 && mHisto2D.at(i).typech2.compare("ADC") == 0) {
        int tdcid = getIdTDCch(mHisto2D.at(i).typech1, mHisto2D.at(i).ch1);
        auto nhit = mEv.NtdcV(tdcid);
        if (mEv.NtdcA(tdcid) == nhit && nhit > 0) {
          mHisto2D.at(i).histo->Fill(mEv.tdcV(tdcid, 0), getADCRecValue(mHisto2D.at(i).typech2, mHisto2D.at(i).ch2));
        }
      }
      if (mHisto2D.at(i).typeh.compare("TDC-DIFF") == 0 && mHisto2D.at(i).typech1.compare("TDCV") == 0 && mHisto2D.at(i).typech2.compare("TDCV") == 0) {
        int zncc_id = getIdTDCch("TDCV", "ZNCC");
        int znac_id = getIdTDCch("TDCV", "ZNAC");
        auto nhit_zncc = mEv.NtdcV(zncc_id);
        auto nhit_znac = mEv.NtdcV(znac_id);
        if ((mEv.NtdcA(zncc_id) == nhit_zncc && nhit_zncc > 0) && (mEv.NtdcA(znac_id) == nhit_znac && nhit_znac > 0)) {
          auto sum = mEv.tdcV(zncc_id, 0) + mEv.tdcV(znac_id, 0);
          auto diff = mEv.tdcV(zncc_id, 0) - mEv.tdcV(znac_id, 0);
          mHisto2D.at(i).histo->Fill(diff, sum);
        }
      }
      if (mHisto2D.at(i).typeh.compare("TDC_T_A") == 0 && mHisto2D.at(i).typech1.compare("TDCV") == 0 && mHisto2D.at(i).typech2.compare("TDCA") == 0) {
        int tdcid = getIdTDCch(mHisto2D.at(i).typech1, mHisto2D.at(i).ch1);
        auto nhitv = mEv.NtdcV(tdcid);
        if (mEv.NtdcA(tdcid) == nhitv && nhitv > 0) {
          for (int ihit = 0; ihit < nhitv; ihit++) {
            mHisto2D.at(i).histo->Fill(mEv.tdcV(tdcid, ihit), mEv.tdcA(tdcid, ihit));
          }
        }
      }
      if (mEv.getNInfo() > 0 && mHisto2D.at(i).typech1.compare("INFO") == 0) {
        auto& decodedInfo = mEv.getDecodedInfo();
        for (uint16_t info : decodedInfo) {
          uint8_t ch = (info >> 10) & 0x1f;
          uint16_t code = info & 0x03ff;
          ;
          mHisto2D.at(i).histo->Fill(ch, code);
        }
      }
      if (mHisto2D.at(i).typeh.compare("CENTR_ZNA") == 0 && mHisto2D.at(i).typech1.compare("ADC") == 0 && mHisto2D.at(i).typech2.compare("ADC") == 0) {
        mEv.centroidZNA(x, y);
        mHisto2D.at(i).histo->Fill(x, y);
      }
      if (mHisto2D.at(i).typeh.compare("CENTR_ZNC") == 0 && mHisto2D.at(i).typech1.compare("ADC") == 0 && mHisto2D.at(i).typech2.compare("ADC") == 0) {
        mEv.centroidZNC(x, y);
        mHisto2D.at(i).histo->Fill(x, y);
      }
    } // for histo 2D
  }   // while
  return 0;
}

float ZDCRecDataTask::getADCRecValue(std::string typech, std::string ch)
{
  if (typech.compare("ADC") == 0 && ch.compare("ZNAC") == 0)
    return mEv.EZNAC();
  if (typech.compare("ADC") == 0 && ch.compare("ZNA1") == 0)
    return mEv.EZNA1();
  if (typech.compare("ADC") == 0 && ch.compare("ZNA2") == 0)
    return mEv.EZNA2();
  if (typech.compare("ADC") == 0 && ch.compare("ZNA3") == 0)
    return mEv.EZNA3();
  if (typech.compare("ADC") == 0 && ch.compare("ZNA4") == 0)
    return mEv.EZNA4();
  if (typech.compare("ADC") == 0 && ch.compare("ZNAS") == 0)
    return mEv.EZNASum();

  if (typech.compare("ADC") == 0 && ch.compare("ZPAC") == 0)
    return mEv.EZPAC();
  if (typech.compare("ADC") == 0 && ch.compare("ZPA1") == 0)
    return mEv.EZPA1();
  if (typech.compare("ADC") == 0 && ch.compare("ZPA2") == 0)
    return mEv.EZPA2();
  if (typech.compare("ADC") == 0 && ch.compare("ZPA3") == 0)
    return mEv.EZPA3();
  if (typech.compare("ADC") == 0 && ch.compare("ZPA4") == 0)
    return mEv.EZPA4();
  if (typech.compare("ADC") == 0 && ch.compare("ZPAS") == 0)
    return mEv.EZPASum();

  if (typech.compare("ADC") == 0 && ch.compare("ZNCC") == 0)
    return mEv.EZNCC();
  if (typech.compare("ADC") == 0 && ch.compare("ZNC1") == 0)
    return mEv.EZNC1();
  if (typech.compare("ADC") == 0 && ch.compare("ZNC2") == 0)
    return mEv.EZNC2();
  if (typech.compare("ADC") == 0 && ch.compare("ZNC3") == 0)
    return mEv.EZNC3();
  if (typech.compare("ADC") == 0 && ch.compare("ZNC4") == 0)
    return mEv.EZNC4();
  if (typech.compare("ADC") == 0 && ch.compare("ZNCS") == 0)
    return mEv.EZNCSum();

  if (typech.compare("ADC") == 0 && ch.compare("ZPCC") == 0)
    return mEv.EZPCC();
  if (typech.compare("ADC") == 0 && ch.compare("ZPC1") == 0)
    return mEv.EZPC1();
  if (typech.compare("ADC") == 0 && ch.compare("ZPC2") == 0)
    return mEv.EZPC2();
  if (typech.compare("ADC") == 0 && ch.compare("ZPC3") == 0)
    return mEv.EZPC3();
  if (typech.compare("ADC") == 0 && ch.compare("ZPC4") == 0)
    return mEv.EZPC4();
  if (typech.compare("ADC") == 0 && ch.compare("ZPCS") == 0)
    return mEv.EZPCSum();

  if (typech.compare("ADC") == 0 && ch.compare("ZEM1") == 0)
    return mEv.EZEM1();
  if (typech.compare("ADC") == 0 && ch.compare("ZEM2") == 0)
    return mEv.EZEM2();
  return 0.00;
}

int ZDCRecDataTask::getIdTDCch(std::string typech, std::string ch)
{
  if ((typech.compare("TDCV") == 0 || typech.compare("TDCA") == 0) && ch.compare("ZNAC") == 0)
    return o2::zdc::TDCZNAC;
  if ((typech.compare("TDCV") == 0 || typech.compare("TDCA") == 0) && ch.compare("ZNAS") == 0)
    return o2::zdc::TDCZNAS;
  if ((typech.compare("TDCV") == 0 || typech.compare("TDCA") == 0) && ch.compare("ZPAC") == 0)
    return o2::zdc::TDCZPAC;
  if ((typech.compare("TDCV") == 0 || typech.compare("TDCA") == 0) && ch.compare("ZPAS") == 0)
    return o2::zdc::TDCZPAS;
  if ((typech.compare("TDCV") == 0 || typech.compare("TDCA") == 0) && ch.compare("ZNCC") == 0)
    return o2::zdc::TDCZNCC;
  if ((typech.compare("TDCV") == 0 || typech.compare("TDCA") == 0) && ch.compare("ZNCS") == 0)
    return o2::zdc::TDCZNCS;
  if ((typech.compare("TDCV") == 0 || typech.compare("TDCA") == 0) && ch.compare("ZPCC") == 0)
    return o2::zdc::TDCZPCC;
  if ((typech.compare("TDCV") == 0 || typech.compare("TDCA") == 0) && ch.compare("ZPCS") == 0)
    return o2::zdc::TDCZPCS;
  if ((typech.compare("TDCV") == 0 || typech.compare("TDCA") == 0) && ch.compare("ZEM1") == 0)
    return o2::zdc::TDCZEM1;
  if ((typech.compare("TDCV") == 0 || typech.compare("TDCA") == 0) && ch.compare("ZEM2") == 0)
    return o2::zdc::TDCZEM2;
  return 0;
}

std::vector<std::string> ZDCRecDataTask::tokenLine(std::string Line, std::string Delimiter)
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

} // namespace o2::quality_control_modules::zdc
