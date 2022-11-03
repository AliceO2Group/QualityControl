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
/// \file   ZDCRawDataCheck.cxx
/// \author Carlo Puggioni
///

#include "ZDC/ZDCRawDataCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TH1.h>
#include <TLatex.h>
#include <TList.h>
#include <TLine.h>

#include <DataFormatsQualityControl/FlagReasons.h>

#include <string>

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::zdc
{

Quality ZDCRawDataCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{

  Quality result = Quality::Null;
  init();
  mNumE = 0;
  mNumW = 0;
  mStringW = "";
  mStringE = "";
  int ib = 0;
  for (auto& [moName, mo] : *moMap) {

    (void)moName;
    if (mo->getName() == "hpedSummary") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      // dumpVecParam((int)h->GetNbinsX(),(int)mVectParam.size());
      if ((int)h->GetNbinsX() != (int)mVectParam.size())
        return Quality::Null;
      for (int i = 0; i < h->GetNbinsX(); i++) {
        ib = i + 1;
        if ((((float)h->GetBinContent(ib) < (float)mVectParam.at(i).minW && (float)h->GetBinContent(ib) >= (float)mVectParam.at(i).minE)) || ((float)h->GetBinContent(ib) > (float)mVectParam.at(i).maxW && (float)h->GetBinContent(ib) < (float)mVectParam.at(i).maxE)) {
          mNumW += 1;
          mStringW = mStringW + mVectParam.at(i).ch + " ";
          ILOG(Warning, Support) << "Raw Warning in " << mVectParam.at(i).ch << " intervall: " << mVectParam.at(i).minW << " - " << mVectParam.at(i).maxW << " Value: " << h->GetBinContent(ib) << ENDM;
        }
        if (((float)h->GetBinContent(ib) < (float)mVectParam.at(i).minE) || ((float)h->GetBinContent(ib) > (float)mVectParam.at(i).maxE)) {
          mNumE += 1;
          mStringE = mStringE + mVectParam.at(i).ch + " ";
          ILOG(Error, Support) << "Raw Error in " << mVectParam.at(i).ch << " intervall: " << mVectParam.at(i).minE << " - " << mVectParam.at(i).maxE << " Value: " << h->GetBinContent(ib) << ENDM;
        }
      }
      if (mNumW == 0 && mNumE == 0) {
        result = Quality::Good;
      }
      if (mNumW > 0) {
        result = Quality::Medium;
        result.addReason(FlagReasonFactory::Unknown(),
                         "It is medium because  " + std::to_string(mNumW) + " channels:" + mStringW + "have a value in the medium range");
      }
      if (mNumE > 0) {
        result = Quality::Bad;
        result.addReason(FlagReasonFactory::Unknown(),
                         "It is bad because  " + std::to_string(mNumW) + " channels:" + mStringE + "have a value in the bad range");
      }
    }
  }
  return result;
}

std::string ZDCRawDataCheck::getAcceptedType() { return "TH1"; }

void ZDCRawDataCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "hpedSummary") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());

    if (checkResult == Quality::Good) {
      TLatex* msg = new TLatex(mPosMsgX, mPosMsgY, "Baseline OK");
      msg->SetNDC();
      msg->SetTextSize(16);
      msg->SetTextFont(43);
      msg->SetTextColor(kGreen);
      h->GetListOfFunctions()->Add(msg);
      h->SetFillColor(kGreen);
      msg->Draw();
    } else if (checkResult == Quality::Bad) {
      std::string errorSt = "Error baseline value in the channels: " + mStringE + "is not correct. Call the expert";
      TLatex* msg = new TLatex(mPosMsgX, mPosMsgY, errorSt.c_str());
      msg->SetNDC();
      msg->SetTextSize(16);
      msg->SetTextFont(43);
      msg->SetTextColor(kRed);
      h->GetListOfFunctions()->Add(msg);
      h->SetFillColor(kRed);
      msg->Draw();
    } else if (checkResult == Quality::Medium) {
      std::string errorSt = "Warning baseline value in the channels: " + mStringW + "is not correct. Send mail to the expert";
      TLatex* msg = new TLatex(mPosMsgX, mPosMsgY, errorSt.c_str());
      msg->SetNDC();
      msg->SetTextSize(16);
      msg->SetTextFont(43);
      msg->SetTextColor(kOrange);
      h->GetListOfFunctions()->Add(msg);
      h->SetFillColor(kOrange);
      msg->Draw();
    }
    h->SetLineColor(kBlack);
  }
}

void ZDCRawDataCheck::init()
{
  mVectParam.clear();
  setChName("ZNAC");
  setChName("ZNA1");
  setChName("ZNA2");
  setChName("ZNAS");
  setChName("ZNA3");
  setChName("ZNA4");
  setChName("ZNCC");
  setChName("ZNC1");
  setChName("ZNC2");
  setChName("ZNCS");
  setChName("ZNC3");
  setChName("ZNC4");
  setChName("ZPAC");
  setChName("ZEM1");
  setChName("ZPA1");
  setChName("ZPA2");
  setChName("ZPAS");
  setChName("ZPA3");
  setChName("ZPA4");
  setChName("ZPCC");
  setChName("ZEM2");
  setChName("ZPC3");
  setChName("ZPC4");
  setChName("ZPCS");
  setChName("ZPC1");
  setChName("ZPC2");
  for (int i = 0; i < (int)mVectParam.size(); i++) {
    setChCheck(i);
  }
  if (const auto param = mCustomParameters.find("POS_MSG_X"); param != mCustomParameters.end()) {
    mPosMsgX = atof(param->second.c_str());
  }
  if (const auto param = mCustomParameters.find("POS_MSG_Y"); param != mCustomParameters.end()) {
    mPosMsgY = atof(param->second.c_str());
  }
}

void ZDCRawDataCheck::setChName(std::string channel)
{
  sCheck chCheck;
  chCheck.ch = channel;
  mVectParam.push_back(chCheck);
}

void ZDCRawDataCheck::setChCheck(int index)
{
  std::vector<std::string> tokenString;
  if (const auto param = mCustomParameters.find(mVectParam.at(index).ch); param != mCustomParameters.end()) {
    tokenString = tokenLine(param->second, ";");
    mVectParam.at(index).minW = atof(tokenString.at(0).c_str()) - atof(tokenString.at(1).c_str());
    mVectParam.at(index).maxW = atof(tokenString.at(0).c_str()) + atof(tokenString.at(1).c_str());
    mVectParam.at(index).minE = atof(tokenString.at(0).c_str()) - atof(tokenString.at(2).c_str());
    mVectParam.at(index).maxE = atof(tokenString.at(0).c_str()) + atof(tokenString.at(2).c_str());
  }
}

std::vector<std::string> ZDCRawDataCheck::tokenLine(std::string Line, std::string Delimiter)
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

void ZDCRawDataCheck::dumpVecParam(int numBinHisto, int num_ch)
{
  std::ofstream dumpFile;
  dumpFile.open("dumpStructuresRawCheck.txt");
  if (dumpFile) {
    dumpFile << "\n Vector Param\n";
    for (int i = 0; i < (int)mVectParam.size(); i++) {
      dumpFile << mVectParam.at(i).ch << " \t" << std::to_string(mVectParam.at(i).minW) << " \t" << std::to_string(mVectParam.at(i).maxW) << " \t" << std::to_string(mVectParam.at(i).minE) << " \t" << std::to_string(mVectParam.at(i).maxE) << " \n";
    }
    dumpFile << "Num Bin Histo " << std::to_string(numBinHisto) << " \n";
    dumpFile << "Num ch " << std::to_string(num_ch) << " \n";
    dumpFile.close();
  }
}

} // namespace o2::quality_control_modules::zdc
