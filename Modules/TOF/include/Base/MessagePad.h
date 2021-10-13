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
/// \file   MessagePad.h
/// \author Nicolo' Jacazio
/// \since 13/10/2021
/// \brief Utilities to contain TOF messages and to display them on QCG
///

#ifndef QC_MODULE_TOF_MESSAGEPAD_H
#define QC_MODULE_TOF_MESSAGEPAD_H

// QC includes
#include "QualityControl/Quality.h"

// ROOT includes
#include "TPaveText.h"
#include "TList.h"
#include "TH1F.h"
#include "TH2F.h"

namespace o2::quality_control_modules::tof
{

struct MessagePad {
  float mPadLowX = 0.6;                 /// Position of the message PAD in low x
  float mPadLowY = 0.5;                 /// Position of the message PAD in low y
  float mPadHighX = 0.9;                /// Position of the message PAD in high x
  float mPadHighY = 0.75;               /// Position of the message PAD in high y
  std::vector<std::string> mMessages{}; /// Message to print
  int mEnabledFlag = 1;                 /// Flag to enable or disable the pad

  template <typename T>
  void Configure(const T& CustomParameters)
  {
    if (auto param = CustomParameters.find("PadLowX"); param != CustomParameters.end()) {
      mPadLowX = ::atof(param->second.c_str());
    }
    if (auto param = CustomParameters.find("PadLowY"); param != CustomParameters.end()) {
      mPadLowY = ::atof(param->second.c_str());
    }
    if (auto param = CustomParameters.find("PadHighX"); param != CustomParameters.end()) {
      mPadHighX = ::atof(param->second.c_str());
    }
    if (auto param = CustomParameters.find("PadHighY"); param != CustomParameters.end()) {
      mPadHighY = ::atof(param->second.c_str());
    }
    if (auto param = CustomParameters.find("EnabledFlag"); param != CustomParameters.end()) {
      mEnabledFlag = ::atoi(param->second.c_str());
    }
  }

  void Configure(const float& padLowX, const float& padLowY, const float& padHighX, const float& padHighY)
  {
    mPadLowX = padLowX;
    mPadLowY = padLowY;
    mPadHighX = padHighX;
    mPadHighY = padHighY;
  }

  void AddMessage(const std::string& message)
  {
    mMessages.push_back(message);
  }

  template <typename T>
  TPaveText* MakeMessagePad(T* histogram, const Quality& quality)
  {
    TPaveText* msg = new TPaveText(mPadLowX, mPadLowY, mPadHighX, mPadHighY, "blNDC");
    if (mEnabledFlag) {
      histogram->GetListOfFunctions()->Add(msg);
    }
    msg->SetBorderSize(1);
    msg->SetTextColor(kBlack);
    if (quality == Quality::Good) {
      msg->SetFillColor(kGreen);
    } else if (quality == Quality::Medium) {
      msg->SetFillColor(kYellow);
    } else if (quality == Quality::Bad) {
      msg->SetFillColor(kRed);
    } else if (quality == Quality::Null) {
      msg->SetTextColor(kWhite);
      msg->SetFillStyle(3001);
      msg->SetFillColor(kBlack);
      msg->AddText("No quality established");
    }
    msg->SetName(Form("%s_msg", histogram->GetName()));
    for (const auto& line : mMessages) {
      msg->AddText(line.c_str());
    }
    mMessages.clear();
    return msg;
  }
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_MESSAGEPAD_H
