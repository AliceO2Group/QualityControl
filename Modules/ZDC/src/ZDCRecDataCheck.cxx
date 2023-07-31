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
/// \file   ZDCRecDataCheck.cxx
/// \author Carlo Puggioni
///
/*
 * The task checks the two histograms: h_summmary_ADC and h_summmary_TDC .
 * - h_summmary_ADC: Each bin contains the average value of an ADC channel. Checking this histogram verifies that all ADC channels have a value within a threshold defined in the json file.
 * - h_summmary_TDC: Each bin contains the average value of an TDC channel. Checking this histogram verifies that all TDC channels have a value within a threshold defined in the json file.
 */

#include "ZDC/ZDCRecDataCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TH1.h>
#include <TLatex.h>
#include <TList.h>
#include <TLine.h>

#include <DataFormatsQualityControl/FlagReasons.h>

#include <chrono>  // chrono::system_clock
#include <ctime>   // localtime
#include <sstream> // stringstream
#include <iomanip> // put_time
#include <string>
using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::zdc
{

void ZDCRecDataCheck::configure() {}

Quality ZDCRecDataCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;
  init();
  mNumEADC = 0;
  mNumWADC = 0;
  mStringWADC = "";
  mStringEADC = "";
  int ib = 0;
  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if (mo->getName() == "h_summmary_ADC") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      // dumpVecParam((int)h->GetNbinsX(),(int)mVectParamADC.size());
      if ((int)h->GetNbinsX() != (int)mVectParamADC.size()) {
        return Quality::Null;
      }
      for (int i = 0; i < h->GetNbinsX(); i++) {
        ib = i + 1;
        if ((((float)h->GetBinContent(ib) < (float)mVectParamADC.at(i).minW && (float)h->GetBinContent(ib) >= (float)mVectParamADC.at(i).minE)) || ((float)h->GetBinContent(ib) > (float)mVectParamADC.at(i).maxW && (float)h->GetBinContent(ib) < (float)mVectParamADC.at(i).maxE)) {
          mNumWADC += 1;
          mStringWADC = mStringWADC + mVectParamADC.at(i).ch + " ";
          ILOG(Warning, Support) << "Rec Warning in " << mVectParamADC.at(i).ch << " intervall: " << mVectParamADC.at(i).minW << " - " << mVectParamADC.at(i).maxW << " Value: " << h->GetBinContent(ib) << ENDM;
        }
        if (((float)h->GetBinContent(ib) < (float)mVectParamADC.at(i).minE) || ((float)h->GetBinContent(ib) > (float)mVectParamADC.at(i).maxE)) {
          mNumEADC += 1;
          mStringEADC = mStringEADC + mVectParamADC.at(i).ch + " ";
          ILOG(Error, Support) << "Rec Error in " << mVectParamADC.at(i).ch << " intervall: " << mVectParamADC.at(i).minE << " - " << mVectParamADC.at(i).maxE << " Value: " << h->GetBinContent(ib) << ENDM;
        }
      }
      if (mNumWADC == 0 && mNumEADC == 0) {
        mQADC = 1;
      }
      if (mNumWADC > 0) {
        mQADC = 2;
      }
      if (mNumEADC > 0) {
        mQADC = 3;
      }
    }
    mNumETDC = 0;
    mNumWTDC = 0;
    mStringWTDC = "";
    mStringETDC = "";
    if (mo->getName() == "h_summmary_TDC") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      // dumpVecParam((int)h->GetNbinsX(),(int)mVectParamTDC.size());
      if ((int)h->GetNbinsX() != (int)mVectParamTDC.size()) {
        return Quality::Null;
      }
      for (int i = 0; i < h->GetNbinsX(); i++) {
        ib = i + 1;
        if ((((float)h->GetBinContent(ib) < (float)mVectParamTDC.at(i).minW && (float)h->GetBinContent(ib) >= (float)mVectParamTDC.at(i).minE)) || ((float)h->GetBinContent(ib) > (float)mVectParamTDC.at(i).maxW && (float)h->GetBinContent(ib) < (float)mVectParamTDC.at(i).maxE)) {
          mNumWTDC += 1;
          mStringWTDC = mStringWTDC + mVectParamTDC.at(i).ch + " ";
          ILOG(Warning, Support) << "Rec Warning in " << mVectParamTDC.at(i).ch << " intervall: " << mVectParamTDC.at(i).minW << " - " << mVectParamTDC.at(i).maxW << " Value: " << h->GetBinContent(ib) << ENDM;
        }
        if (((float)h->GetBinContent(ib) < (float)mVectParamTDC.at(i).minE) || ((float)h->GetBinContent(ib) > (float)mVectParamTDC.at(i).maxE)) {
          mNumETDC += 1;
          mStringETDC = mStringETDC + mVectParamTDC.at(i).ch + " ";
          ILOG(Error, Support) << "Rec Error in " << mVectParamTDC.at(i).ch << " intervall: " << mVectParamTDC.at(i).minE << " - " << mVectParamTDC.at(i).maxE << " Value: " << h->GetBinContent(ib) << ENDM;
        }
      }
      if (mNumWTDC == 0 && mNumETDC == 0) {
        mQTDC = 1;
      }
      if (mNumWTDC > 0) {
        mQTDC = 2;
      }
      if (mNumETDC > 0) {
        mQTDC = 3;
      }

      if (mQADC == 1 && mQTDC == 1) {
        result = Quality::Good;
      } else if (mQADC == 3 || mQTDC == 3) {
        result = Quality::Bad;
        if (mQADC == 3) {
          result.addReason(FlagReasonFactory::Unknown(),
                           "Task quality is bad because in ADC Summary " + std::to_string(mNumWADC) + " channels:" + mStringEADC + "have a value in the bad range");
        }
        if (mQTDC == 3) {
          result.addReason(FlagReasonFactory::Unknown(),
                           "It is bad because in TDC Summary" + std::to_string(mNumWTDC) + " channels:" + mStringWTDC + "have a value in the bad range");
        }
      } else {
        result = Quality::Medium;
        if (mQADC == 2) {
          result.addReason(FlagReasonFactory::Unknown(),
                           "It is medium because in ADC Summary " + std::to_string(mNumWADC) + " channels:" + mStringWADC + "have a value in the medium range");
        }
        if (mQTDC == 2) {
          result.addReason(FlagReasonFactory::Unknown(),
                           "It is medium because in TDC Summary " + std::to_string(mNumWTDC) + " channels:" + mStringWTDC + "have a value in the medium range");
        }
      }
    }
  }
  return result;
}

std::string ZDCRecDataCheck::getAcceptedType() { return "TH1"; }

void ZDCRecDataCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "h_summmary_ADC") {
    if (mQADC == 1) {
      setQualityInfo(mo, kGreen, getCurrentDataTime() + "ADC OK");
    } else if (mQADC == 3) {
      std::string errorSt = getCurrentDataTime() + "Error ADC value in the channels: " + mStringEADC + "is not correct. Call the expert";
      setQualityInfo(mo, kRed, errorSt);
    } else if (mQADC == 2) {
      std::string errorSt = getCurrentDataTime() + "Warning ADC value in the channels: " + mStringWADC + "is not correct. Send mail to the expert";
      setQualityInfo(mo, kOrange, errorSt);
    }
  }
  if (mo->getName() == "h_summmary_TDC") {
    if (mQTDC == 1) {
      setQualityInfo(mo, kGreen, getCurrentDataTime() + "TDC OK");
    } else if (mQTDC == 3) {
      std::string errorSt = getCurrentDataTime() + "Error TDC value in the channels: " + mStringETDC + "is not correct. Call the expert";
      setQualityInfo(mo, kRed, errorSt);
    } else if (mQTDC == 2) {
      std::string errorSt = getCurrentDataTime() + "Warning TDC value in the channels: " + mStringWTDC + "is not correct. Send mail to the expert";
      setQualityInfo(mo, kOrange, errorSt);
    }
  }
}

void ZDCRecDataCheck::setQualityInfo(std::shared_ptr<MonitorObject> mo, int color, std::string text)
{
  auto* h = dynamic_cast<TH1F*>(mo->getObject());
  TLatex* msg = new TLatex(mPosMsgADCX, mPosMsgADCY, text.c_str());
  msg->SetNDC();
  msg->SetTextSize(16);
  msg->SetTextFont(43);
  h->GetListOfFunctions()->Add(msg);
  h->SetFillColor(color);
  msg->SetTextColor(color);
  msg->Draw();
  h->SetLineColor(kBlack);
}

void ZDCRecDataCheck::init()
{
  mVectParamADC.clear();
  setChName("ADC_ZNAC", "ADC");
  setChName("ADC_ZNA1", "ADC");
  setChName("ADC_ZNA2", "ADC");
  setChName("ADC_ZNA3", "ADC");
  setChName("ADC_ZNA4", "ADC");
  setChName("ADC_ZNAS", "ADC");

  setChName("ADC_ZPAC", "ADC");
  setChName("ADC_ZPA1", "ADC");
  setChName("ADC_ZPA2", "ADC");
  setChName("ADC_ZPA3", "ADC");
  setChName("ADC_ZPA4", "ADC");
  setChName("ADC_ZPAS", "ADC");

  setChName("ADC_ZEM1", "ADC");
  setChName("ADC_ZEM2", "ADC");

  setChName("ADC_ZNCC", "ADC");
  setChName("ADC_ZNC1", "ADC");
  setChName("ADC_ZNC2", "ADC");
  setChName("ADC_ZNC3", "ADC");
  setChName("ADC_ZNC4", "ADC");
  setChName("ADC_ZNCS", "ADC");

  setChName("ADC_ZPCC", "ADC");
  setChName("ADC_ZPC1", "ADC");
  setChName("ADC_ZPC2", "ADC");
  setChName("ADC_ZPC3", "ADC");
  setChName("ADC_ZPC4", "ADC");
  setChName("ADC_ZPCS", "ADC");

  setChName("TDC_ZNAC", "TDC");
  setChName("TDC_ZNAS", "TDC");
  setChName("TDC_ZPAC", "TDC");
  setChName("TDC_ZPAS", "TDC");
  setChName("TDC_ZEM1", "TDC");
  setChName("TDC_ZEM2", "TDC");
  setChName("TDC_ZNCC", "TDC");
  setChName("TDC_ZNCS", "TDC");
  setChName("TDC_ZPCC", "TDC");
  setChName("TDC_ZPCS", "TDC");

  for (int i = 0; i < (int)mVectParamADC.size(); i++) {
    setChCheck(i, "ADC");
  }
  for (int i = 0; i < (int)mVectParamTDC.size(); i++) {
    setChCheck(i, "TDC");
  }
  if (const auto param = mCustomParameters.find("ADC_POS_MSG_X"); param != mCustomParameters.end()) {
    mPosMsgADCX = atof(param->second.c_str());
  }
  if (const auto param = mCustomParameters.find("ADC_POS_MSG_Y"); param != mCustomParameters.end()) {
    mPosMsgADCY = atof(param->second.c_str());
  }
  if (const auto param = mCustomParameters.find("TDC_POS_MSG_X"); param != mCustomParameters.end()) {
    mPosMsgTDCX = atof(param->second.c_str());
  }
  if (const auto param = mCustomParameters.find("TDC_POS_MSG_Y"); param != mCustomParameters.end()) {
    mPosMsgTDCY = atof(param->second.c_str());
  }
}

void ZDCRecDataCheck::setChName(std::string channel, std::string type)
{
  sCheck chCheck;
  chCheck.ch = channel;
  if (type.compare("ADC") == 0) {
    mVectParamADC.push_back(chCheck);
  }
  if (type.compare("TDC") == 0) {
    mVectParamTDC.push_back(chCheck);
  }
}

void ZDCRecDataCheck::setChCheck(int index, std::string type)
{
  std::vector<std::string> tokenString;
  if (type.compare("ADC") == 0 && index < (int)mVectParamADC.size()) {
    if (const auto param = mCustomParameters.find(mVectParamADC.at(index).ch); param != mCustomParameters.end()) {
      tokenString = tokenLine(param->second, ";");

      mVectParamADC.at(index).minW = atof(tokenString.at(0).c_str()) - atof(tokenString.at(1).c_str());
      mVectParamADC.at(index).maxW = atof(tokenString.at(0).c_str()) + atof(tokenString.at(1).c_str());
      mVectParamADC.at(index).minE = atof(tokenString.at(0).c_str()) - atof(tokenString.at(2).c_str());
      mVectParamADC.at(index).maxE = atof(tokenString.at(0).c_str()) + atof(tokenString.at(2).c_str());
    }
  }
  if (type.compare("TDC") == 0 && index < (int)mVectParamTDC.size()) {
    if (const auto param = mCustomParameters.find(mVectParamTDC.at(index).ch); param != mCustomParameters.end()) {
      tokenString = tokenLine(param->second, ";");

      mVectParamTDC.at(index).minW = atof(tokenString.at(0).c_str()) - atof(tokenString.at(1).c_str());
      mVectParamTDC.at(index).maxW = atof(tokenString.at(0).c_str()) + atof(tokenString.at(1).c_str());
      mVectParamTDC.at(index).minE = atof(tokenString.at(0).c_str()) - atof(tokenString.at(2).c_str());
      mVectParamTDC.at(index).maxE = atof(tokenString.at(0).c_str()) + atof(tokenString.at(2).c_str());
    }
  }
}

std::vector<std::string> ZDCRecDataCheck::tokenLine(std::string Line, std::string Delimiter)
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

std::string ZDCRecDataCheck::getCurrentDataTime()
{
  auto now = std::chrono::system_clock::now();
  auto in_time_t = std::chrono::system_clock::to_time_t(now);

  std::stringstream ss;
  ss << std::put_time(std::localtime(&in_time_t), "%d-%m-%Y %X");
  return ss.str();
}

void ZDCRecDataCheck::dumpVecParam(int numBinHisto, int num_ch)
{
  std::ofstream dumpFile;
  dumpFile.open("dumpStructuresRecCheck.txt");
  if (dumpFile.good()) {
    dumpFile << "\n Vector Param ADC \n";
    for (int i = 0; i < (int)mVectParamADC.size(); i++) {
      dumpFile << mVectParamADC.at(i).ch << " \t" << std::to_string(mVectParamADC.at(i).minW) << " \t" << std::to_string(mVectParamADC.at(i).maxW) << " \t" << std::to_string(mVectParamADC.at(i).minE) << " \t" << std::to_string(mVectParamADC.at(i).maxE) << " \n";
    }
    dumpFile << "\n Vector Param TDC \n";
    for (int i = 0; i < (int)mVectParamTDC.size(); i++) {
      dumpFile << mVectParamTDC.at(i).ch << " \t" << std::to_string(mVectParamTDC.at(i).minW) << " \t" << std::to_string(mVectParamTDC.at(i).maxW) << " \t" << std::to_string(mVectParamTDC.at(i).minE) << " \t" << std::to_string(mVectParamTDC.at(i).maxE) << " \n";
    }
    dumpFile << "Num Bin Histo " << std::to_string(numBinHisto) << " \n";
    dumpFile << "Num ch " << std::to_string(num_ch) << " \n";
    dumpFile.close();
  }
}

} // namespace o2::quality_control_modules::zdc
