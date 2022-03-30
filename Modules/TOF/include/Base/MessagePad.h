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
#include "QualityControl/QcInfoLogger.h"

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
  std::vector<std::string> mMessages{}; /// Message to print on the pad, this is reset at each call of MakeMessagePad
  TPaveText* mMessagePad = nullptr;     /// Text pad with the messages
  int mEnabledFlag = 1;                 /// Flag to enable or disable the pad
  const std::string mName = "";         /// Name of the message pad, can be used to identify the pad if multiple are used

  MessagePad(const std::string name = "",
             const float padLowX = 0.6, const float padLowY = 0.5,
             const float padHighX = 0.9, const float padHighY = 0.75) : mName(name)
  {
    ILOG(Info, Support) << "Making new message pad " << mName << ", " << mPadLowX << padLowY << ", " << padHighX << ", " << padHighY << ENDM;
    setPosition(padLowX, padLowY, padHighX, padHighY);
  }

  /// Function configure the message pad based on the input configuration
  template <typename T>
  void configure(const T& CustomParameters)
  {
    if (auto param = CustomParameters.find(mName + "PadLowX"); param != CustomParameters.end()) {
      mPadLowX = ::atof(param->second.c_str());
      ILOG(Info, Support) << "Setting message pad " << mName << " mPadLowX to " << mPadLowX << ENDM;
    }
    if (auto param = CustomParameters.find(mName + "PadLowY"); param != CustomParameters.end()) {
      mPadLowY = ::atof(param->second.c_str());
      ILOG(Info, Support) << "Setting message pad " << mName << " mPadLowY to " << mPadLowY << ENDM;
    }
    if (auto param = CustomParameters.find(mName + "PadHighX"); param != CustomParameters.end()) {
      mPadHighX = ::atof(param->second.c_str());
      ILOG(Info, Support) << "Setting message pad " << mName << " mPadHighX to " << mPadHighX << ENDM;
    }
    if (auto param = CustomParameters.find(mName + "PadHighY"); param != CustomParameters.end()) {
      mPadHighY = ::atof(param->second.c_str());
      ILOG(Info, Support) << "Setting message pad " << mName << " mPadHighY to " << mPadHighY << ENDM;
    }
    configureEnabledFlag(CustomParameters);
  }

  /// Function to set the enabled flag based on the input configuration
  template <typename T>
  void configureEnabledFlag(const T& CustomParameters)
  {
    if (auto param = CustomParameters.find(mName + "EnabledFlag"); param != CustomParameters.end()) {
      mEnabledFlag = ::atoi(param->second.c_str());
      ILOG(Info, Support) << "Setting message pad " << mName << " mEnabledFlag to " << mEnabledFlag << ENDM;
    }
  }

  /// Function configure the message pad position
  void setPosition(const float& padLowX, const float& padLowY, const float& padHighX, const float& padHighY)
  {
    mPadLowX = padLowX;
    mPadLowY = padLowY;
    mPadHighX = padHighX;
    mPadHighY = padHighY;
  }

  /// Function to add a message that will be reported in the pad, will only add the message if the flag mEnabledFlag is on
  void AddMessage(const std::string& message)
  {
    if (!mEnabledFlag) {
      return;
    }
    mMessages.push_back(message);
  }

  /// Function to add the message pad to the histogram. Returns nullptr if the message pad is disabled
  template <typename T>
  TPaveText* MakeMessagePad(T* histogram, const Quality& quality, Option_t* padOpt = "blNDC")
  {
    if (!mEnabledFlag) {
      mMessages.clear();
      ILOG(Info, Devel) << "Message pad " << mName << " is disabled" << ENDM;
      return nullptr;
    }
    ILOG(Info, Devel) << "Message pad " << mName << " is enabled" << ENDM;

    mMessagePad = new TPaveText(mPadLowX, mPadLowY, mPadHighX, mPadHighY, padOpt);
    mMessagePad->SetName(Form("%s_msg", histogram->GetName()));
    histogram->GetListOfFunctions()->Add(mMessagePad);
    mMessagePad->SetBorderSize(1);
    mMessagePad->SetTextColor(kBlack);
    if (quality == Quality::Good) {
      mMessagePad->SetFillColor(kGreen);
    } else if (quality == Quality::Medium) {
      mMessagePad->SetFillColor(kYellow);
    } else if (quality == Quality::Bad) {
      mMessagePad->SetFillColor(kRed);
    } else if (quality == Quality::Null) {
      mMessagePad->SetTextColor(kWhite);
      mMessagePad->SetFillStyle(3001);
      mMessagePad->SetFillColor(kBlack);
      mMessagePad->AddText("No quality established");
    }
    for (const auto& line : mMessages) {
      mMessagePad->AddText(line.c_str());
    }
    // Clear the messages for next usage
    mMessages.clear();
    return mMessagePad;
  }
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_MESSAGEPAD_H
