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
 * The task checks the two histograms: h_summary_ADC and h_summary_TDC .
 * - h_summary_ADC: Each bin contains the average value of an ADC channel. Checking this histogram verifies that all ADC channels have a value within a threshold defined in the json file.
 * - h_summary_TDC: Each bin contains the average value of an TDC channel. Checking this histogram verifies that all TDC channels have a value within a threshold defined in the json file.
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

#include <DataFormatsQualityControl/FlagType.h>
#include <DataFormatsQualityControl/FlagTypeFactory.h>

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

void ZDCRecDataCheck::startOfActivity(const Activity& activity)
{
  // ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
  init(activity);
}

Quality ZDCRecDataCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;
  // ADC
  mNumEADC = 0;
  mNumWADC = 0;
  mStringWADC = "";
  mStringEADC = "";
  int ib = 0;
  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if (mo->getName() == "h_summary_ADC") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      if (h == nullptr) {
        ILOG(Error, Support) << "could not cast '" << mo->getName() << "' to TH1*" << ENDM;
        return Quality::Null;
      }
      // dumpVecParam((int)h->GetNbinsX(),(int)mVectParamADC.size());
      if ((int)h->GetNbinsX() != (int)mVectParamADC.size()) {
        return Quality::Null;
      }
      for (int i = 0; i < h->GetNbinsX(); i++) {
        ib = i + 1;
        if ((((float)h->GetBinContent(ib) < (float)mVectParamADC.at(i).minW && (float)h->GetBinContent(ib) >= (float)mVectParamADC.at(i).minE)) || ((float)h->GetBinContent(ib) > (float)mVectParamADC.at(i).maxW && (float)h->GetBinContent(ib) < (float)mVectParamADC.at(i).maxE)) {
          mNumWADC += 1;
          mStringWADC = mStringWADC + mVectParamADC.at(i).ch + " ";
          //  ILOG(Warning, Support) << "Rec Warning in " << mVectParamADC.at(i).ch << " intervall: " << mVectParamADC.at(i).minW << " - " << mVectParamADC.at(i).maxW << " Value: " << h->GetBinContent(ib) << ENDM;
        }
        if (((float)h->GetBinContent(ib) < (float)mVectParamADC.at(i).minE) || ((float)h->GetBinContent(ib) > (float)mVectParamADC.at(i).maxE)) {
          mNumEADC += 1;
          mStringEADC = mStringEADC + mVectParamADC.at(i).ch + " ";
          //    ILOG(Error, Support) << "Rec Error in " << mVectParamADC.at(i).ch << " intervall: " << mVectParamADC.at(i).minE << " - " << mVectParamADC.at(i).maxE << " Value: " << h->GetBinContent(ib) << ENDM;
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

    // TDC TIME
    mNumETDC = 0;
    mNumWTDC = 0;
    mStringWTDC = "";
    mStringETDC = "";
    if (mo->getName() == "h_summary_TDC") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      if (h == nullptr) {
        ILOG(Error, Support) << "could not cast '" << mo->getName() << "' to TH1*" << ENDM;
        return Quality::Null;
      }
      // dumpVecParam((int)h->GetNbinsX(),(int)mVectParamTDC.size());
      if ((int)h->GetNbinsX() != (int)mVectParamTDC.size()) {
        return Quality::Null;
      }
      for (int i = 0; i < h->GetNbinsX(); i++) {
        ib = i + 1;
        if ((((float)h->GetBinContent(ib) < (float)mVectParamTDC.at(i).minW && (float)h->GetBinContent(ib) >= (float)mVectParamTDC.at(i).minE)) || ((float)h->GetBinContent(ib) > (float)mVectParamTDC.at(i).maxW && (float)h->GetBinContent(ib) < (float)mVectParamTDC.at(i).maxE)) {
          mNumWTDC += 1;
          mStringWTDC = mStringWTDC + mVectParamTDC.at(i).ch + " ";
          //  ILOG(Warning, Support) << "Rec Warning in " << mVectParamTDC.at(i).ch << " intervall: " << mVectParamTDC.at(i).minW << " - " << mVectParamTDC.at(i).maxW << " Value: " << h->GetBinContent(ib) << ENDM;
        }
        if (((float)h->GetBinContent(ib) < (float)mVectParamTDC.at(i).minE) || ((float)h->GetBinContent(ib) > (float)mVectParamTDC.at(i).maxE)) {
          mNumETDC += 1;
          mStringETDC = mStringETDC + mVectParamTDC.at(i).ch + " ";
          //  ILOG(Error, Support) << "Rec Error in " << mVectParamTDC.at(i).ch << " intervall: " << mVectParamTDC.at(i).minE << " - " << mVectParamTDC.at(i).maxE << " Value: " << h->GetBinContent(ib) << ENDM;
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
    }

    // summary TDC Amplitude
    mNumETDCA = 0;
    mNumWTDCA = 0;
    mStringWTDCA = "";
    mStringETDCA = "";
    if (mo->getName() == "h_summary_TDCA") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      if (h == nullptr) {
        ILOG(Error, Support) << "could not cast '" << mo->getName() << "' to TH1*" << ENDM;
        return Quality::Null;
      }
      // dumpVecParam((int)h->GetNbinsX(),(int)mVectParamTDC.size());
      if ((int)h->GetNbinsX() != (int)mVectParamTDCA.size()) {
        return Quality::Null;
      }
      for (int i = 0; i < h->GetNbinsX(); i++) {
        ib = i + 1;
        if ((((float)h->GetBinContent(ib) < (float)mVectParamTDCA.at(i).minW && (float)h->GetBinContent(ib) >= (float)mVectParamTDCA.at(i).minE)) || ((float)h->GetBinContent(ib) > (float)mVectParamTDCA.at(i).maxW && (float)h->GetBinContent(ib) < (float)mVectParamTDCA.at(i).maxE)) {
          mNumWTDCA += 1;
          mStringWTDCA = mStringWTDCA + mVectParamTDCA.at(i).ch + " ";
          //  ILOG(Warning, Support) << "Rec Warning in " << mVectParamTDCA.at(i).ch << " intervall: " << mVectParamTDCA.at(i).minW << " - " << mVectParamTDCA.at(i).maxW << " Value: " << h->GetBinContent(ib) << ENDM;
        }
        if (((float)h->GetBinContent(ib) < (float)mVectParamTDCA.at(i).minE) || ((float)h->GetBinContent(ib) > (float)mVectParamTDCA.at(i).maxE)) {
          mNumETDCA += 1;
          mStringETDCA = mStringETDCA + mVectParamTDCA.at(i).ch + " ";
          //  ILOG(Error, Support) << "Rec Error in " << mVectParamTDCA.at(i).ch << " intervall: " << mVectParamTDCA.at(i).minE << " - " << mVectParamTDCA.at(i).maxE << " Value: " << h->GetBinContent(ib) << ENDM;
        }
      }
      if (mNumWTDCA == 0 && mNumETDCA == 0) {
        mQTDCA = 1;
      }
      if (mNumWTDCA > 0) {
        mQTDCA = 2;
      }
      if (mNumETDCA > 0) {
        mQTDCA = 3;
      }
    }

    // summary TDC Amplitude Peak 1n
    mNumEPeak1n = 0;
    mNumWPeak1n = 0;
    mStringWPeak1n = "";
    mStringEPeak1n = "";
    if (mo->getName() == "h_summary_Peak1n") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      if (h == nullptr) {
        ILOG(Error, Support) << "could not cast '" << mo->getName() << "' to TH1*" << ENDM;
        return Quality::Null;
      }
      // dumpVecParam((int)h->GetNbinsX(),(int)mVectParamTDC.size());
      if ((int)h->GetNbinsX() != (int)mVectParamPeak1n.size()) {
        return Quality::Null;
      }
      for (int i = 0; i < h->GetNbinsX(); i++) {
        ib = i + 1;
        if ((((float)h->GetBinContent(ib) < (float)mVectParamPeak1n.at(i).minW && (float)h->GetBinContent(ib) >= (float)mVectParamPeak1n.at(i).minE)) || ((float)h->GetBinContent(ib) > (float)mVectParamPeak1n.at(i).maxW && (float)h->GetBinContent(ib) < (float)mVectParamPeak1n.at(i).maxE)) {
          mNumWPeak1n += 1;
          mStringWPeak1n = mStringWPeak1n + mVectParamPeak1n.at(i).ch + " ";
          //  ILOG(Warning, Support) << "Rec Warning in " << mVectParamPeak1n.at(i).ch << " intervall: " << mVectParamPeak1n.at(i).minW << " - " << mVectParamPeak1n.at(i).maxW << " Value: " << h->GetBinContent(ib) << ENDM;
        }
        if (((float)h->GetBinContent(ib) < (float)mVectParamPeak1n.at(i).minE) || ((float)h->GetBinContent(ib) > (float)mVectParamPeak1n.at(i).maxE)) {
          mNumEPeak1n += 1;
          mStringEPeak1n = mStringEPeak1n + mVectParamPeak1n.at(i).ch + " ";
          //  ILOG(Error, Support) << "Rec Error in " << mVectParamPeak1n.at(i).ch << " intervall: " << mVectParamPeak1n.at(i).minE << " - " << mVectParamPeak1n.at(i).maxE << " Value: " << h->GetBinContent(ib) << ENDM;
        }
      }
      if (mNumWPeak1n == 0 && mNumEPeak1n == 0) {
        mQPeak1n = 1;
      }
      if (mNumWPeak1n > 0) {
        mQPeak1n = 2;
      }
      if (mNumEPeak1n > 0) {
        mQPeak1n = 3;
      }
    }

    // summary TDC Amplitude peak 1p
    mNumEPeak1p = 0;
    mNumWPeak1p = 0;
    mStringWPeak1p = "";
    mStringEPeak1p = "";
    if (mo->getName() == "h_summary_Peak1p") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      if (h == nullptr) {
        ILOG(Error, Support) << "could not cast '" << mo->getName() << "' to TH1*" << ENDM;
        return Quality::Null;
      }
      // dumpVecParam((int)h->GetNbinsX(),(int)mVectParamTDC.size());
      if ((int)h->GetNbinsX() != (int)mVectParamPeak1p.size()) {
        return Quality::Null;
      }
      for (int i = 0; i < h->GetNbinsX(); i++) {
        ib = i + 1;
        if ((((float)h->GetBinContent(ib) < (float)mVectParamPeak1p.at(i).minW && (float)h->GetBinContent(ib) >= (float)mVectParamPeak1p.at(i).minE)) || ((float)h->GetBinContent(ib) > (float)mVectParamPeak1p.at(i).maxW && (float)h->GetBinContent(ib) < (float)mVectParamPeak1p.at(i).maxE)) {
          mNumWPeak1p += 1;
          mStringWPeak1p = mStringWPeak1p + mVectParamPeak1p.at(i).ch + " ";
          // ILOG(Warning, Support) << "Rec Warning in " << mVectParamPeak1p.at(i).ch << " intervall: " << mVectParamPeak1p.at(i).minW << " - " << mVectParamPeak1p.at(i).maxW << " Value: " << h->GetBinContent(ib) << ENDM;
        }
        if (((float)h->GetBinContent(ib) < (float)mVectParamPeak1p.at(i).minE) || ((float)h->GetBinContent(ib) > (float)mVectParamPeak1p.at(i).maxE)) {
          mNumEPeak1p += 1;
          mStringEPeak1p = mStringEPeak1p + mVectParamPeak1p.at(i).ch + " ";
          // ILOG(Error, Support) << "Rec Error in " << mVectParamPeak1p.at(i).ch << " intervall: " << mVectParamPeak1p.at(i).minE << " - " << mVectParamPeak1p.at(i).maxE << " Value: " << h->GetBinContent(ib) << ENDM;
        }
      }
      if (mNumWPeak1p == 0 && mNumEPeak1p == 0) {
        mQPeak1p = 1;
      }
      if (mNumWPeak1p > 0) {
        mQPeak1p = 2;
      }
      if (mNumEPeak1p > 0) {
        mQPeak1p = 3;
      }
    }

    // Global Check
    if (mQADC == 1 && mQTDC == 1 && mQTDCA == 1 && mQPeak1n == 1 && mQPeak1p == 1) {
      result = Quality::Good;
    } else if (mQADC == 3 || mQTDC == 3 || mQTDCA == 3 && mQPeak1n == 3 && mQPeak1p == 3) {
      result = Quality::Bad;
      if (mQADC == 3) {
        result.addFlag(FlagTypeFactory::Unknown(),
                       "Task quality is bad because in ADC Summary " + std::to_string(mNumWADC) + " channels:" + mStringEADC + "have a value in the bad range");
      }

      if (mQTDC == 3) {
        result.addFlag(FlagTypeFactory::Unknown(),
                       "It is bad because in TDC  Summary" + std::to_string(mNumWTDC) + " channels:" + mStringETDC + "have a value in the bad range");
      }
      if (mQTDCA == 3) {
        result.addFlag(FlagTypeFactory::Unknown(),
                       "It is bad because in TDC Amplitude Summary" + std::to_string(mNumWTDCA) + " channels:" + mStringETDCA + "have a value in the bad range");
      }
      if (mQPeak1n == 3) {
        result.addFlag(FlagTypeFactory::Unknown(),
                       "It is bad because in TDC Amplitude Peak1n Summary" + std::to_string(mNumWPeak1n) + " channels:" + mStringEPeak1n + "have a value in the bad range");
      }
      if (mQPeak1p == 3) {
        result.addFlag(FlagTypeFactory::Unknown(),
                       "It is bad because in TDC Amplitude Peak1p Summary" + std::to_string(mNumWPeak1p) + " channels:" + mStringEPeak1p + "have a value in the bad range");
      }
    } else {
      result = Quality::Medium;
      if (mQADC == 2) {
        result.addFlag(FlagTypeFactory::Unknown(),
                       "It is medium because in ADC Summary " + std::to_string(mNumWADC) + " channels:" + mStringWADC + "have a value in the medium range");
      }
      if (mQTDC == 2) {
        result.addFlag(FlagTypeFactory::Unknown(),
                       "It is medium because in TDC  Summary " + std::to_string(mNumWTDC) + " channels:" + mStringWTDC + "have a value in the medium range");
      }
      if (mQTDCA == 2) {
        result.addFlag(FlagTypeFactory::Unknown(),
                       "It is medium because in TDC Amplitude  Summary " + std::to_string(mNumWTDCA) + " channels:" + mStringWTDCA + "have a value in the medium range");
      }
      if (mQPeak1n == 2) {
        result.addFlag(FlagTypeFactory::Unknown(),
                       "It is medium because in TDC Amplitude Peak1n Summary " + std::to_string(mNumWPeak1n) + " channels:" + mStringWPeak1n + "have a value in the medium range");
      }
      if (mQPeak1p == 2) {
        result.addFlag(FlagTypeFactory::Unknown(),
                       "It is medium because in TDC Amplitude Peak1p Summary " + std::to_string(mNumWPeak1p) + " channels:" + mStringWPeak1p + "have a value in the medium range");
      }
    }
  }
  return result;
}



void ZDCRecDataCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "h_summary_ADC") {
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

  if (mo->getName() == "h_summary_TDC") {
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

  if (mo->getName() == "h_summary_TDCA") {
    if (mQTDCA == 1) {
      setQualityInfo(mo, kGreen, getCurrentDataTime() + "TDC Amplitude OK");
    } else if (mQTDCA == 3) {
      std::string errorSt = getCurrentDataTime() + "Error TDC Amplitude  value in the channels: " + mStringETDCA + "is not correct. Call the expert";
      setQualityInfo(mo, kRed, errorSt);
    } else if (mQTDCA == 2) {
      std::string errorSt = getCurrentDataTime() + "Warning TDC Amplitude  value in the channels: " + mStringWTDCA + "is not correct. Send mail to the expert";
      setQualityInfo(mo, kOrange, errorSt);
    }
  }

  if (mo->getName() == "h_summary_Peak1n") {
    if (mQPeak1n == 1) {
      setQualityInfo(mo, kGreen, getCurrentDataTime() + "TDC Amplitude Peak1n OK");
    } else if (mQPeak1n == 3) {
      std::string errorSt = getCurrentDataTime() + "Error TDC Amplitude Peak1n value in the channels: " + mStringEPeak1n + "is not correct. Call the expert";
      setQualityInfo(mo, kRed, errorSt);
    } else if (mQPeak1n == 2) {
      std::string errorSt = getCurrentDataTime() + "Warning TDC Amplitude Peak1n value in the channels: " + mStringWPeak1n + "is not correct. Send mail to the expert";
      setQualityInfo(mo, kOrange, errorSt);
    }
  }

  if (mo->getName() == "h_summary_Peak1p") {
    if (mQPeak1p == 1) {
      setQualityInfo(mo, kGreen, getCurrentDataTime() + "TDC Amplitude OK");
    } else if (mQPeak1p == 3) {
      std::string errorSt = getCurrentDataTime() + "Error TDC Amplitude Peak1p value in the channels: " + mStringEPeak1p + "is not correct. Call the expert";
      setQualityInfo(mo, kRed, errorSt);
    } else if (mQPeak1p == 2) {
      std::string errorSt = getCurrentDataTime() + "Warning TDC Amplitude Peak1p value in the channels: " + mStringWPeak1p + "is not correct. Send mail to the expert";
      setQualityInfo(mo, kOrange, errorSt);
    }
  }
}
void ZDCRecDataCheck::setQualityInfo(std::shared_ptr<MonitorObject> mo, int color, std::string text)
{
  auto* h = dynamic_cast<TH1F*>(mo->getObject());
  if (h == nullptr) {
    ILOG(Error, Support) << "could not cast '" << mo->getName() << "' to TH1*" << ENDM;
    return;
  }
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

void ZDCRecDataCheck::init(const Activity& activity)
{
  mVectParamADC.clear();
  mVectParamTDC.clear();
  mVectParamTDCA.clear();
  mVectParamPeak1n.clear();
  mVectParamPeak1p.clear();

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

  setChName("TDCA_ZNAC", "TDCA");
  setChName("TDCA_ZNAS", "TDCA");
  setChName("TDCA_ZPAC", "TDCA");
  setChName("TDCA_ZPAS", "TDCA");
  setChName("TDCA_ZEM1", "TDCA");
  setChName("TDCA_ZEM2", "TDCA");
  setChName("TDCA_ZNCC", "TDCA");
  setChName("TDCA_ZNCS", "TDCA");
  setChName("TDCA_ZPCC", "TDCA");
  setChName("TDCA_ZPCS", "TDCA");

  setChName("PEAK1N_ZNAC", "PEAK1N");
  setChName("PEAK1N_ZNAS", "PEAK1N");
  setChName("PEAK1N_ZNCC", "PEAK1N");
  setChName("PEAK1N_ZNCS", "PEAK1N");

  setChName("PEAK1P_ZPAC", "PEAK1P");
  setChName("PEAK1P_ZPAS", "PEAK1P");
  setChName("PEAK1P_ZPCC", "PEAK1P");
  setChName("PEAK1P_ZPCS", "PEAK1P");

  for (int i = 0; i < (int)mVectParamADC.size(); i++) {
    setChCheck(i, "ADC", activity);
  }
  for (int i = 0; i < (int)mVectParamTDC.size(); i++) {
    setChCheck(i, "TDC", activity);
  }
  for (int i = 0; i < (int)mVectParamTDCA.size(); i++) {
    setChCheck(i, "TDCA", activity);
  }
  for (int i = 0; i < (int)mVectParamPeak1n.size(); i++) {
    setChCheck(i, "PEAK1N", activity);
  }
  for (int i = 0; i < (int)mVectParamPeak1p.size(); i++) {
    setChCheck(i, "PEAK1P", activity);
  }
  if (auto param = mCustomParameters.atOptional("ADC_POS_MSG_X", activity)) {
    mPosMsgADCX = atof(param.value().c_str());
  }
  if (auto param = mCustomParameters.atOptional("ADC_POS_MSG_Y", activity)) {
    mPosMsgADCY = atof(param.value().c_str());
  }
  if (auto param = mCustomParameters.atOptional("TDC_POS_MSG_X", activity)) {
    mPosMsgTDCX = atof(param.value().c_str());
  }
  if (auto param = mCustomParameters.atOptional("TDC_POS_MSG_Y", activity)) {
    mPosMsgTDCY = atof(param.value().c_str());
  }
  if (auto param = mCustomParameters.atOptional("TDCA_POS_MSG_X", activity)) {
    mPosMsgTDCAX = atof(param.value().c_str());
  }
  if (auto param = mCustomParameters.atOptional("TDCA_POS_MSG_Y", activity)) {
    mPosMsgTDCAY = atof(param.value().c_str());
  }
  if (auto param = mCustomParameters.atOptional("PEAK1N_POS_MSG_X", activity)) {
    mPosMsgPeak1nX = atof(param.value().c_str());
  }
  if (auto param = mCustomParameters.atOptional("PEAK1N_POS_MSG_Y", activity)) {
    mPosMsgPeak1nY = atof(param.value().c_str());
  }
  if (auto param = mCustomParameters.atOptional("PEAK1P_POS_MSG_X", activity)) {
    mPosMsgPeak1pX = atof(param.value().c_str());
  }
  if (auto param = mCustomParameters.atOptional("PEAK1P_POS_MSG_Y", activity)) {
    mPosMsgPeak1pY = atof(param.value().c_str());
  }
}

void ZDCRecDataCheck::setChName(std::string channel, std::string type)
{
  sCheck chCheck;
  chCheck.ch = channel;
  if (type == "ADC") {
    mVectParamADC.push_back(chCheck);
  }
  if (type == "TDC") {
    mVectParamTDC.push_back(chCheck);
  }
  if (type == "TDCA") {
    mVectParamTDCA.push_back(chCheck);
  }
  if (type == "PEAK1N") {
    mVectParamPeak1n.push_back(chCheck);
  }
  if (type == "PEAK1P") {
    mVectParamPeak1p.push_back(chCheck);
  }
}

void ZDCRecDataCheck::setChCheck(int index, std::string type, const Activity& activity)
{
  std::vector<std::string> tokenString;
  if (type == "ADC" && index < (int)mVectParamADC.size()) {
    if (auto param = mCustomParameters.atOptional(mVectParamADC.at(index).ch, activity)) {
      tokenString = tokenLine(param.value(), ";");

      mVectParamADC.at(index).minW = atof(tokenString.at(0).c_str()) - atof(tokenString.at(1).c_str());
      mVectParamADC.at(index).maxW = atof(tokenString.at(0).c_str()) + atof(tokenString.at(1).c_str());
      mVectParamADC.at(index).minE = atof(tokenString.at(0).c_str()) - atof(tokenString.at(2).c_str());
      mVectParamADC.at(index).maxE = atof(tokenString.at(0).c_str()) + atof(tokenString.at(2).c_str());
    }
  }
  if (type == "TDC" && index < (int)mVectParamTDC.size()) {
    if (auto param = mCustomParameters.atOptional(mVectParamTDC.at(index).ch, activity)) {
      tokenString = tokenLine(param.value(), ";");

      mVectParamTDC.at(index).minW = atof(tokenString.at(0).c_str()) - atof(tokenString.at(1).c_str());
      mVectParamTDC.at(index).maxW = atof(tokenString.at(0).c_str()) + atof(tokenString.at(1).c_str());
      mVectParamTDC.at(index).minE = atof(tokenString.at(0).c_str()) - atof(tokenString.at(2).c_str());
      mVectParamTDC.at(index).maxE = atof(tokenString.at(0).c_str()) + atof(tokenString.at(2).c_str());
    }
  }
  if (type == "TDCA" && index < (int)mVectParamTDCA.size()) {
    if (auto param = mCustomParameters.atOptional(mVectParamTDCA.at(index).ch, activity)) {
      tokenString = tokenLine(param.value(), ";");

      mVectParamTDCA.at(index).minW = atof(tokenString.at(0).c_str()) - atof(tokenString.at(1).c_str());
      mVectParamTDCA.at(index).maxW = atof(tokenString.at(0).c_str()) + atof(tokenString.at(1).c_str());
      mVectParamTDCA.at(index).minE = atof(tokenString.at(0).c_str()) - atof(tokenString.at(2).c_str());
      mVectParamTDCA.at(index).maxE = atof(tokenString.at(0).c_str()) + atof(tokenString.at(2).c_str());
    }
  }
  if (type == "PEAK1N" && index < (int)mVectParamPeak1n.size()) {
    if (auto param = mCustomParameters.atOptional(mVectParamPeak1n.at(index).ch, activity)) {
      tokenString = tokenLine(param.value(), ";");

      mVectParamPeak1n.at(index).minW = atof(tokenString.at(0).c_str()) - atof(tokenString.at(1).c_str());
      mVectParamPeak1n.at(index).maxW = atof(tokenString.at(0).c_str()) + atof(tokenString.at(1).c_str());
      mVectParamPeak1n.at(index).minE = atof(tokenString.at(0).c_str()) - atof(tokenString.at(2).c_str());
      mVectParamPeak1n.at(index).maxE = atof(tokenString.at(0).c_str()) + atof(tokenString.at(2).c_str());
    }
  }
  if (type == "PEAK1P" && index < (int)mVectParamPeak1p.size()) {
    if (auto param = mCustomParameters.atOptional(mVectParamPeak1p.at(index).ch, activity)) {
      tokenString = tokenLine(param.value(), ";");

      mVectParamPeak1p.at(index).minW = atof(tokenString.at(0).c_str()) - atof(tokenString.at(1).c_str());
      mVectParamPeak1p.at(index).maxW = atof(tokenString.at(0).c_str()) + atof(tokenString.at(1).c_str());
      mVectParamPeak1p.at(index).minE = atof(tokenString.at(0).c_str()) - atof(tokenString.at(2).c_str());
      mVectParamPeak1p.at(index).maxE = atof(tokenString.at(0).c_str()) + atof(tokenString.at(2).c_str());
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
    dumpFile << "\n Vector Param TDCA \n";
    for (int i = 0; i < (int)mVectParamTDCA.size(); i++) {
      dumpFile << mVectParamTDCA.at(i).ch << " \t" << std::to_string(mVectParamTDCA.at(i).minW) << " \t" << std::to_string(mVectParamTDCA.at(i).maxW) << " \t" << std::to_string(mVectParamTDCA.at(i).minE) << " \t" << std::to_string(mVectParamTDCA.at(i).maxE) << " \n";
    }
    dumpFile << "\n Vector Param PEAK1N \n";
    for (int i = 0; i < (int)mVectParamPeak1n.size(); i++) {
      dumpFile << mVectParamPeak1n.at(i).ch << " \t" << std::to_string(mVectParamPeak1n.at(i).minW) << " \t" << std::to_string(mVectParamPeak1n.at(i).maxW) << " \t" << std::to_string(mVectParamPeak1n.at(i).minE) << " \t" << std::to_string(mVectParamPeak1n.at(i).maxE) << " \n";
    }

    dumpFile << "\n Vector Param PEAK1P \n";
    for (int i = 0; i < (int)mVectParamPeak1n.size(); i++) {
      dumpFile << mVectParamPeak1p.at(i).ch << " \t" << std::to_string(mVectParamPeak1p.at(i).minW) << " \t" << std::to_string(mVectParamPeak1p.at(i).maxW) << " \t" << std::to_string(mVectParamPeak1p.at(i).minE) << " \t" << std::to_string(mVectParamPeak1p.at(i).maxE) << " \n";
    }

    dumpFile << "Num Bin Histo " << std::to_string(numBinHisto) << " \n";
    dumpFile << "Num ch " << std::to_string(num_ch) << " \n";
    dumpFile.close();
  }
}
} // namespace o2::quality_control_modules::zdc
