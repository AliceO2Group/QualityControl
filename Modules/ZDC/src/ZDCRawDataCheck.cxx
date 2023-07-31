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

#include <chrono>  // chrono::system_clock
#include <ctime>   // localtime
#include <sstream> // stringstream
#include <iomanip> // put_time
#include <string>  // string

// ROOT
#include <TH1.h>
#include <TH2.h>
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
  int ib = 0;
  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    // For -> Histo to check
    for (int ih = 0; ih < (int)mVectHistoCheck.size(); ih++) {
      mVectHistoCheck.at(ih).numE = 0;
      mVectHistoCheck.at(ih).numW = 0;
      mVectHistoCheck.at(ih).stringW = "";
      mVectHistoCheck.at(ih).stringE = "";
      if (mo->getName() == mVectHistoCheck.at(ih).nameHisto) {
        if ((mo->getName() == "hpedSummary")) {
          auto* h = dynamic_cast<TH1*>(mo->getObject());
          if ((int)h->GetNbinsX() != (int)mVectHistoCheck.at(ih).paramch.size()) {
            return Quality::Null;
          }
          // check all channels
          for (int i = 0; i < h->GetNbinsX(); i++) {
            ib = i + 1;
            if ((((float)h->GetBinContent(ib) < (float)mVectHistoCheck.at(ih).paramch.at(i).minW && (float)h->GetBinContent(ib) >= (float)mVectHistoCheck.at(ih).paramch.at(i).minE)) || ((float)h->GetBinContent(ib) > (float)mVectHistoCheck.at(ih).paramch.at(i).maxW && (float)h->GetBinContent(ib) < (float)mVectHistoCheck.at(ih).paramch.at(i).maxE)) {
              mVectHistoCheck.at(ih).numW += 1;
              mVectHistoCheck.at(ih).stringW = mVectHistoCheck.at(ih).stringW + mVectHistoCheck.at(ih).paramch.at(i).ch + " ";
              ILOG(Warning, Support) << "Baseline Warning in " << mVectHistoCheck.at(ih).paramch.at(i).param << " intervall: " << mVectHistoCheck.at(ih).paramch.at(i).minW << " - " << mVectHistoCheck.at(ih).paramch.at(i).maxW << " Value: " << h->GetBinContent(ib) << ENDM;
            }
            if (((float)h->GetBinContent(ib) < (float)mVectHistoCheck.at(ih).paramch.at(i).minE) || ((float)h->GetBinContent(ib) > (float)mVectHistoCheck.at(ih).paramch.at(i).maxE)) {
              mVectHistoCheck.at(ih).numE += 1;
              mVectHistoCheck.at(ih).stringE = mVectHistoCheck.at(ih).stringE + mVectHistoCheck.at(ih).paramch.at(i).ch + " ";
              ILOG(Error, Support) << "Baseline Error in " << mVectHistoCheck.at(ih).paramch.at(i).param << " intervall: " << mVectHistoCheck.at(ih).paramch.at(i).minE << " - " << mVectHistoCheck.at(ih).paramch.at(i).maxE << " Value: " << h->GetBinContent(ib) << ENDM;
            }
          }
        }

        if ((mo->getName() == "hAlignPlot") || (mo->getName() == "hAlignPlotShift")) {
          int flag_ch_empty = 1;
          int flag_all_ch_empty = 1;
          auto* h = dynamic_cast<TH2*>(mo->getObject());
          if ((int)h->GetNbinsX() != (int)mVectHistoCheck.at(ih).paramch.size()) {
            return Quality::Null;
          }
          for (int x = 0; x < h->GetNbinsX(); x++) {
            for (int y = 0; y < h->GetNbinsY(); y++) {
              if (h->GetBinContent(x + 1, y + 1) > 0) {
                flag_ch_empty = 0;
                flag_all_ch_empty = 0;
                if ((((float)y < (float)mVectHistoCheck.at(ih).paramch.at(x).minW && (float)y >= (float)mVectHistoCheck.at(ih).paramch.at(x).minE)) || ((float)y > (float)mVectHistoCheck.at(ih).paramch.at(x).maxW && (float)y < (float)mVectHistoCheck.at(ih).paramch.at(x).maxE)) {
                  mVectHistoCheck.at(ih).numW += 1;
                  mVectHistoCheck.at(ih).stringW = mVectHistoCheck.at(ih).stringW + mVectHistoCheck.at(ih).paramch.at(x).ch + " ";
                  ILOG(Warning, Support) << "Alignment warning:" << mVectHistoCheck.at(ih).paramch.at(x).param << " intervall: " << mVectHistoCheck.at(ih).paramch.at(x).minW << " - " << mVectHistoCheck.at(ih).paramch.at(x).maxW << " Value: " << y << ENDM;
                }
                if (((float)y < (float)mVectHistoCheck.at(ih).paramch.at(x).minE) || ((float)y > (float)mVectHistoCheck.at(ih).paramch.at(x).maxE)) {
                  mVectHistoCheck.at(ih).numE += 1;
                  mVectHistoCheck.at(ih).stringE = mVectHistoCheck.at(ih).stringE + mVectHistoCheck.at(ih).paramch.at(x).ch + " ";
                  ILOG(Error, Support) << "Alignment error:" << mVectHistoCheck.at(ih).paramch.at(x).param << " intervall: " << mVectHistoCheck.at(ih).paramch.at(x).minE << " - " << mVectHistoCheck.at(ih).paramch.at(x).maxE << " Value: " << y << ENDM;
                }
              }
            }
            if (flag_ch_empty == 1) {
              mVectHistoCheck.at(ih).numE += 1;
              mVectHistoCheck.at(ih).stringE = mVectHistoCheck.at(ih).stringE + mVectHistoCheck.at(ih).paramch.at(x).ch + "(Empty) ";
              ILOG(Error, Support) << "Alignment error: Channel" << mVectHistoCheck.at(ih).paramch.at(x).ch << " is empty. Ignore if is the first cycle " << ENDM;
            }
            flag_ch_empty = 1;
          }
          if (flag_all_ch_empty == 1) {
            mVectHistoCheck.at(ih).numE = 0;
            mVectHistoCheck.at(ih).stringE = "";
          }
        }

        if (mo->getName() == "herrorSummary") {
          int flag_ch_empty = 1;
          auto* h = dynamic_cast<TH2*>(mo->getObject());
          if ((int)h->GetNbinsX() != (int)mVectHistoCheck.at(ih).paramch.size()) {
            return Quality::Null;
          }
          for (int x = 0; x < h->GetNbinsX(); x++) {
            for (int y = 0; y < h->GetNbinsY(); y++) {
              if (h->GetBinContent(x + 1, y + 1) > 0) {
                flag_ch_empty = 0;
                if (y == 0) {
                  mVectHistoCheck.at(ih).numE += 1;
                  mVectHistoCheck.at(ih).stringE = mVectHistoCheck.at(ih).stringE + mVectHistoCheck.at(ih).paramch.at(x).ch + " ";
                  ILOG(Error, Support) << "Error Bit:" << mVectHistoCheck.at(ih).paramch.at(x).ch << " Value: " << h->GetBinContent(x + 1, y + 1) << ENDM;
                }
                if (y == 1) {
                  mVectHistoCheck.at(ih).numW += 1;
                  mVectHistoCheck.at(ih).stringW = mVectHistoCheck.at(ih).stringW + mVectHistoCheck.at(ih).paramch.at(x).ch + " ";
                  ILOG(Warning, Support) << "Data Loss warning:" << mVectHistoCheck.at(ih).paramch.at(x).ch << " Value: " << h->GetBinContent(x + 1, y + 1) << ENDM;
                }
                if (y == 2) {
                  mVectHistoCheck.at(ih).numW += 1;
                  mVectHistoCheck.at(ih).stringW = mVectHistoCheck.at(ih).stringW + mVectHistoCheck.at(ih).paramch.at(x).ch + " ";
                  ILOG(Warning, Support) << "Data Corrupted warning:" << mVectHistoCheck.at(ih).paramch.at(x).ch << " Value: " << h->GetBinContent(x + 1, y + 1) << ENDM;
                }
              }
            }
            flag_ch_empty = 1;
          }
        }
        // check result check
        if (mVectHistoCheck.at(ih).numW == 0 && mVectHistoCheck.at(ih).numE == 0) {
          result = Quality::Good;
          mVectHistoCheck.at(ih).quality = 1;
        }
        if (mVectHistoCheck.at(ih).numW > 0) {
          result = Quality::Medium;
          result.addReason(FlagReasonFactory::Unknown(),
                           "It is medium because  " + std::to_string(mVectHistoCheck.at(ih).numW) + " channels:" + mVectHistoCheck.at(ih).stringW + "have a value in the medium range");
          mVectHistoCheck.at(ih).quality = 2;
        }
        if (mVectHistoCheck.at(ih).numE > 0) {
          result = Quality::Bad;
          result.addReason(FlagReasonFactory::Unknown(),
                           "It is bad because  " + std::to_string(mVectHistoCheck.at(ih).numW) + " channels:" + mVectHistoCheck.at(ih).stringE + "have a value in the bad range");
          mVectHistoCheck.at(ih).quality = 3;
        }
      }
    }
  }
  return result;
}

std::string ZDCRawDataCheck::getAcceptedType() { return "TH1"; }

void ZDCRawDataCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  // dumpStruct();

  for (int ih = 0; ih < (int)mVectHistoCheck.size(); ih++) {

    if (mo->getName() == mVectHistoCheck.at(ih).nameHisto) {
      auto* h = dynamic_cast<TH1*>(mo->getObject());

      if (mVectHistoCheck.at(ih).quality == 1) {
        std::string errorSt = getCurrentDataTime() + " Ok";
        TLatex* msg = new TLatex(mVectHistoCheck.at(ih).posMsgX, mVectHistoCheck.at(ih).posMsgY, errorSt.c_str());
        msg->SetNDC();
        msg->SetTextSize(16);
        msg->SetTextFont(43);
        msg->SetTextColor(kGreen);
        h->GetListOfFunctions()->Add(msg);
        h->SetFillColor(kGreen);
        msg->Draw();
      } else if (mVectHistoCheck.at(ih).quality == 3) {
        std::string errorSt = getCurrentDataTime() + " Errors --> Call the expert." + mVectHistoCheck.at(ih).stringE;
        TLatex* msg = new TLatex(mVectHistoCheck.at(ih).posMsgX, mVectHistoCheck.at(ih).posMsgY, errorSt.c_str());
        msg->SetNDC();
        msg->SetTextSize(16);
        msg->SetTextFont(43);
        msg->SetTextColor(kRed);
        h->GetListOfFunctions()->Add(msg);
        h->SetFillColor(kRed);
        msg->Draw();
      } else if (mVectHistoCheck.at(ih).quality == 2) {
        std::string errorSt = getCurrentDataTime() + " Warning --> Send mail to the expert." + mVectHistoCheck.at(ih).stringW;
        TLatex* msg = new TLatex(mVectHistoCheck.at(ih).posMsgX, mVectHistoCheck.at(ih).posMsgY, errorSt.c_str());
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
}

void ZDCRawDataCheck::init()
{
  mVectch.clear();
  mVectHistoCheck.clear();
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
  setChCheck("hpedSummary", "TH1F", "PED", "PED_POS_MSG_X", "PED_POS_MSG_Y");
  setChCheck("hAlignPlotShift", "TH2F", "ALIGN", "ALIGN_POS_MSG_X", "ALIGN_POS_MSG_Y");
  setChCheck("herrorSummary", "TH2F", "ERROR", "ERROR_POS_MSG_X", "ERROR_POS_MSG_Y");
  // dumpStruct();
}

void ZDCRawDataCheck::setChName(std::string channel)
{
  mVectch.push_back(channel);
}

void ZDCRawDataCheck::setChCheck(std::string histoName, std::string typeHisto, std::string typeCheck, std::string paramPosMsgX, std::string paramPosMsgY)
{
  sCheck chCheck;
  sHistoCheck histoCheck;
  std::vector<std::string> tokenString;
  // General info check and histo
  histoCheck.nameHisto = histoName;
  histoCheck.typeHisto = typeHisto;
  histoCheck.typecheck = typeCheck;
  histoCheck.paramPosMsgX = paramPosMsgX;
  histoCheck.paramPosMsgY = paramPosMsgY;
  if (const auto param = mCustomParameters.find(histoCheck.paramPosMsgX); param != mCustomParameters.end()) {
    histoCheck.posMsgX = atof(param->second.c_str());
  }
  if (const auto param = mCustomParameters.find(histoCheck.paramPosMsgY); param != mCustomParameters.end()) {
    histoCheck.posMsgY = atof(param->second.c_str());
  }
  // For each ZDC Channel
  for (int i = 0; i < (int)mVectch.size(); i++) {
    chCheck.ch = mVectch.at(i);
    chCheck.param = histoCheck.typecheck + "_" + chCheck.ch;
    if (const auto param = mCustomParameters.find(chCheck.param); param != mCustomParameters.end()) {
      tokenString = tokenLine(param->second, ";");
      chCheck.minW = atof(tokenString.at(0).c_str()) - atof(tokenString.at(1).c_str());
      chCheck.maxW = atof(tokenString.at(0).c_str()) + atof(tokenString.at(1).c_str());
      chCheck.minE = atof(tokenString.at(0).c_str()) - atof(tokenString.at(2).c_str());
      chCheck.maxE = atof(tokenString.at(0).c_str()) + atof(tokenString.at(2).c_str());
    }
    histoCheck.paramch.push_back(chCheck);
  }
  mVectHistoCheck.push_back(histoCheck);
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

void ZDCRawDataCheck::dumpVecParam(int id_histo, int numBinHisto, int num_ch)
{
  std::ofstream dumpFile;
  dumpFile.open("dumpStructuresRawCheck.txt");
  if (dumpFile.good()) {
    dumpFile << "\n Vector Param\n";
    for (int i = 0; i < (int)mVectHistoCheck.at(id_histo).paramch.size(); i++) {
      dumpFile << mVectHistoCheck.at(id_histo).paramch.at(i).param << " \t" << std::to_string(mVectHistoCheck.at(id_histo).paramch.at(i).minW) << " \t" << std::to_string(mVectHistoCheck.at(id_histo).paramch.at(i).maxW) << " \t" << std::to_string(mVectHistoCheck.at(id_histo).paramch.at(i).minE) << " \t" << std::to_string(mVectHistoCheck.at(id_histo).paramch.at(i).maxE) << " \n";
    }
    dumpFile << "Num Bin Histo " << std::to_string(numBinHisto) << " \n";
    dumpFile << "Num ch " << std::to_string(num_ch) << " \n";
    dumpFile.close();
  }
}

std::string ZDCRawDataCheck::getCurrentDataTime()
{
  auto now = std::chrono::system_clock::now();
  auto in_time_t = std::chrono::system_clock::to_time_t(now);

  std::stringstream ss;
  ss << std::put_time(std::localtime(&in_time_t), "%d-%m-%Y %X");
  return ss.str();
}

void ZDCRawDataCheck::dumpStruct()
{
  std::ofstream dumpFile;
  dumpFile.open("dumpStructuresRawCheck2.txt");
  if (dumpFile.good()) {
    dumpFile << "\n Vector Param\n";
    dumpFile << "NumHisto " << std::to_string((int)mVectHistoCheck.size()) << " \n";
    for (int id_histo = 0; id_histo < (int)mVectHistoCheck.size(); id_histo++) {
      dumpFile << "Name Histo " << mVectHistoCheck.at(id_histo).nameHisto << " \n";
      dumpFile << "Type Histo " << mVectHistoCheck.at(id_histo).typeHisto << " \n";
      dumpFile << "Type Check " << mVectHistoCheck.at(id_histo).typecheck << " \n";
      dumpFile << "paramPosMsgX " << mVectHistoCheck.at(id_histo).paramPosMsgX << " \n";
      dumpFile << "paramPosMsgY " << mVectHistoCheck.at(id_histo).paramPosMsgY << " \n";
      dumpFile << "posMsgX " << std::to_string(mVectHistoCheck.at(id_histo).posMsgX) << " \n";
      dumpFile << "posMsgY " << std::to_string(mVectHistoCheck.at(id_histo).posMsgY) << " \n";
      dumpFile << "numW " << std::to_string(mVectHistoCheck.at(id_histo).numW) << " \n";
      dumpFile << "numE " << std::to_string(mVectHistoCheck.at(id_histo).numE) << " \n";
      dumpFile << "stringW " << mVectHistoCheck.at(id_histo).stringW << " \n";
      dumpFile << "stringE " << mVectHistoCheck.at(id_histo).stringE << " \n";
      dumpFile << "quality " << mVectHistoCheck.at(id_histo).quality << " \n";
      for (int i = 0; i < (int)mVectHistoCheck.at(id_histo).paramch.size(); i++) {
        dumpFile << mVectHistoCheck.at(id_histo).paramch.at(i).param << " \t" << std::to_string(mVectHistoCheck.at(id_histo).paramch.at(i).minW) << " \t" << std::to_string(mVectHistoCheck.at(id_histo).paramch.at(i).maxW) << " \t" << std::to_string(mVectHistoCheck.at(id_histo).paramch.at(i).minE) << " \t" << std::to_string(mVectHistoCheck.at(id_histo).paramch.at(i).maxE) << " \n";
      }
    }

    dumpFile.close();
  }
}

} // namespace o2::quality_control_modules::zdc
