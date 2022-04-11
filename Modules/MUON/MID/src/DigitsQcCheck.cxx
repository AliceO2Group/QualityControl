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
/// \file   DigitsQcCheck.cxx
/// \author Bogdan Vulpescu
/// \author Valerie Ramillien
///

#include "MID/DigitsQcCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TH1.h>
#include <TPaveText.h>
#include <TLatex.h>

#define MID_HITMULT_LIMIT 100

using namespace std;

namespace o2::quality_control_modules::mid
{

void DigitsQcCheck::configure() {}

Quality DigitsQcCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;
  int mean = 0;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;
    // Bend Multiplicity Histo ::
    if (mo->getName() == "mMultHitMT11B") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      resultBMT11 = Quality::Good;
      mean = h->GetMean();
      std::cout << "check :: BMT11 mean =>>  " << mean << std::endl;
      if (mean > MID_HITMULT_LIMIT)
        resultBMT11 = Quality::Bad;
      else if (mean > MID_HITMULT_LIMIT / 2)
        resultBMT11 = Quality::Medium;
    } // end mMultHitMT11B check

    if (mo->getName() == "mMultHitMT12B") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      resultBMT12 = Quality::Good;
      mean = h->GetMean();
      if (mean > MID_HITMULT_LIMIT)
        resultBMT12 = Quality::Bad;
      else if (mean > MID_HITMULT_LIMIT / 2)
        resultBMT12 = Quality::Medium;
    } // end mMultHitMT12B check

    if (mo->getName() == "mMultHitMT21B") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      resultBMT21 = Quality::Good;
      mean = h->GetMean();
      if (mean > MID_HITMULT_LIMIT)
        resultBMT21 = Quality::Bad;
      else if (mean > MID_HITMULT_LIMIT / 2)
        resultBMT21 = Quality::Medium;
    } // end mMultHitMT21B check

    if (mo->getName() == "mMultHitMT22B") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      resultBMT22 = Quality::Good;
      mean = h->GetMean();
      if (mean > MID_HITMULT_LIMIT)
        resultBMT22 = Quality::Bad;
      else if (mean > MID_HITMULT_LIMIT / 2)
        resultBMT22 = Quality::Medium;
    } // end mMultHitMT22B check

    // Non-Bend Multiplicity Histo ::
    if (mo->getName() == "mMultHitMT11NB") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      resultNBMT11 = Quality::Good;
      mean = h->GetMean();
      // std::cout << "check :: NBMT11 mean =>>  " << mean << std::endl;
      if (mean > MID_HITMULT_LIMIT)
        resultNBMT11 = Quality::Bad;
      else if (mean > MID_HITMULT_LIMIT / 2)
        resultNBMT11 = Quality::Medium;
    } // end mMultHitMT11NB check

    if (mo->getName() == "mMultHitMT12NB") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      resultNBMT12 = Quality::Good;
      mean = h->GetMean();
      if (mean > MID_HITMULT_LIMIT)
        resultNBMT12 = Quality::Bad;
      else if (mean > MID_HITMULT_LIMIT / 2)
        resultNBMT12 = Quality::Medium;
    } // end mMultHitMT12NB check

    if (mo->getName() == "mMultHitMT21NB") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      resultNBMT21 = Quality::Good;
      mean = h->GetMean();
      if (mean > MID_HITMULT_LIMIT)
        resultNBMT21 = Quality::Bad;
      else if (mean > MID_HITMULT_LIMIT / 2)
        resultNBMT21 = Quality::Medium;
    } // end mMultHitMT21NB check

    if (mo->getName() == "mMultHitMT22NB") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      resultNBMT22 = Quality::Good;
      mean = h->GetMean();
      if (mean > MID_HITMULT_LIMIT)
        resultNBMT22 = Quality::Bad;
      else if (mean > MID_HITMULT_LIMIT / 2)
        resultNBMT22 = Quality::Medium;
    } // end mMultHitMT22NB check
  }
  return result;
}

std::string DigitsQcCheck::getAcceptedType() { return "TH1"; }

void DigitsQcCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{

  TPaveText* msg = new TPaveText(0.5, 0.5, 0.9, 0.75, "NDC");

  // Bend Multiplicity Histo ::
  if (mo->getName() == "mMultHitMT11B") {
    msg->SetName(Form("%s_msg", mo->GetName()));
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    if (resultBMT11 == Quality::Good) {
      std::cout << "beautify :: BMT11 mean =>>  " << h->GetMean() << std::endl;
      h->SetFillColor(kGreen);
      msg->Clear();
      msg->AddText("Quality::Good");
      msg->AddText(Form("Mean value = %lu is under the limit ", h->GetMean()));
      msg->SetFillColor(kGreen);
    } else if (resultBMT11 == Quality::Bad) {
      ILOG(Info) << "Quality::Bad, setting to red";
      h->SetFillColor(kRed);
      msg->Clear();
      msg->AddText("Quality::Bad");
      msg->AddText(Form("Mean value = %lu is upper the limit ", h->GetMean()));
      msg->AddText("call expert");
      msg->SetFillColor(kRed);
    } else if (resultBMT11 == Quality::Medium) {
      ILOG(Info) << "Quality::medium, setting to orange";
      h->SetFillColor(kOrange);
      msg->AddText("Quality::Medium");
      msg->AddText(Form("Mean value = %lu ; Quality::Medium", h->GetMean()));
      msg->AddText("PLEASE IGNORE.");
      msg->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
  if (mo->getName() == "mMultHitMT12B") {
    msg->SetName(Form("%s_msg", mo->GetName()));
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    if (resultBMT12 == Quality::Good) {
      h->SetFillColor(kGreen);
      msg->Clear();
      msg->AddText("Quality::Good");
      msg->AddText(Form("Mean value = %lu is under the limit ", h->GetMean()));
      msg->SetFillColor(kGreen);
    } else if (resultBMT12 == Quality::Bad) {
      ILOG(Info) << "Quality::Bad, setting to red";
      h->SetFillColor(kRed);
      msg->Clear();
      msg->AddText("Quality::Bad");
      msg->AddText(Form("Mean value = %lu is upper the limit ", h->GetMean()));
      msg->AddText("call expert");
      msg->SetFillColor(kRed);
    } else if (resultBMT12 == Quality::Medium) {
      ILOG(Info) << "Quality::medium, setting to orange";
      h->SetFillColor(kOrange);
      msg->Clear();
      msg->AddText("Quality::Medium");
      msg->AddText(Form("Mean value = %lu ; Quality::Medium", h->GetMean()));
      msg->AddText("PLEASE IGNORE.");
      msg->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
  if (mo->getName() == "mMultHitMT21B") {
    msg->SetName(Form("%s_msg", mo->GetName()));
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    if (resultBMT21 == Quality::Good) {
      h->SetFillColor(kGreen);
      msg->Clear();
      msg->AddText("Quality::Good");
      msg->AddText("Mean value is under the limit");
      msg->SetFillColor(kGreen);
    } else if (resultBMT21 == Quality::Bad) {
      ILOG(Info) << "Quality::Bad, setting to red";
      h->SetFillColor(kRed);
      msg->Clear();
      msg->AddText("Quality::Bad");
      msg->AddText("Mean value is upper the limit");
      msg->AddText("call expert");
      msg->SetFillColor(kRed);
    } else if (resultBMT21 == Quality::Medium) {
      ILOG(Info) << "Quality::medium, setting to orange";
      h->SetFillColor(kOrange);
      msg->Clear();
      msg->AddText("Quality::Medium");
      msg->AddText("PLEASE IGNORE.");
      msg->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
  if (mo->getName() == "mMultHitMT22B") {
    msg->SetName(Form("%s_msg", mo->GetName()));
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    if (resultBMT22 == Quality::Good) {
      h->SetFillColor(kGreen);
      msg->Clear();
      msg->AddText("Quality::Good");
      msg->AddText("Mean value is under the limit");
      msg->SetFillColor(kGreen);
    } else if (resultBMT22 == Quality::Bad) {
      ILOG(Info) << "Quality::Bad, setting to red";
      h->SetFillColor(kRed);
      msg->Clear();
      msg->AddText("Quality::Bad");
      msg->AddText("Mean value is upper the limit");
      msg->AddText("call expert");
      msg->SetFillColor(kRed);
    } else if (resultBMT22 == Quality::Medium) {
      ILOG(Info) << "Quality::medium, setting to orange";
      h->SetFillColor(kOrange);
      msg->Clear();
      msg->AddText("Quality::Medium");
      msg->AddText("PLEASE IGNORE.");
      msg->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
  // Non-Bend Multiplicity Histo ::
  if (mo->getName() == "mMultHitMT11NB") {
    msg->SetName(Form("%s_msg", mo->GetName()));
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    if (resultNBMT11 == Quality::Good) {
      std::cout << "beautify :: NBMT11 mean =>>  " << h->GetMean() << std::endl;
      h->SetFillColor(kGreen);
      msg->Clear();
      msg->AddText("Quality::Good");
      msg->AddText(Form("Mean value = %lu is under the limit ", h->GetMean()));
      msg->SetFillColor(kGreen);
    } else if (resultNBMT11 == Quality::Bad) {
      ILOG(Info) << "Quality::Bad, setting to red";
      h->SetFillColor(kRed);
      msg->Clear();
      msg->AddText("Quality::Bad");
      msg->AddText(Form("Mean value = %lu is upper the limit ", h->GetMean()));
      msg->AddText("call expert");
      msg->SetFillColor(kRed);
    } else if (resultNBMT11 == Quality::Medium) {
      ILOG(Info) << "Quality::medium, setting to orange";
      h->SetFillColor(kOrange);
      msg->AddText("Quality::Medium");
      msg->AddText(Form("Mean value = %lu ; Quality::Medium", h->GetMean()));
      msg->AddText("PLEASE IGNORE.");
      msg->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
  if (mo->getName() == "mMultHitMT12NB") {
    msg->SetName(Form("%s_msg", mo->GetName()));
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    if (resultNBMT12 == Quality::Good) {
      h->SetFillColor(kGreen);
      msg->Clear();
      msg->AddText("Quality::Good");
      msg->AddText(Form("Mean value = %lu is under the limit ", h->GetMean()));
      msg->SetFillColor(kGreen);
    } else if (resultNBMT12 == Quality::Bad) {
      ILOG(Info) << "Quality::Bad, setting to red";
      h->SetFillColor(kRed);
      msg->Clear();
      msg->AddText("Quality::Bad");
      msg->AddText(Form("Mean value = %lu is upper the limit ", h->GetMean()));
      msg->AddText("call expert");
      msg->SetFillColor(kRed);
    } else if (resultNBMT12 == Quality::Medium) {
      ILOG(Info) << "Quality::medium, setting to orange";
      h->SetFillColor(kOrange);
      msg->Clear();
      msg->AddText("Quality::Medium");
      msg->AddText(Form("Mean value = %lu ; Quality::Medium", h->GetMean()));
      msg->AddText("PLEASE IGNORE.");
      msg->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
  if (mo->getName() == "mMultHitMT21NB") {
    msg->SetName(Form("%s_msg", mo->GetName()));
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    if (resultNBMT21 == Quality::Good) {
      h->SetFillColor(kGreen);
      msg->Clear();
      msg->AddText("Quality::Good");
      msg->AddText("Mean value is under the limit");
      msg->SetFillColor(kGreen);
    } else if (resultNBMT21 == Quality::Bad) {
      ILOG(Info) << "Quality::Bad, setting to red";
      h->SetFillColor(kRed);
      msg->Clear();
      msg->AddText("Quality::Bad");
      msg->AddText("Mean value is upper the limit");
      msg->AddText("call expert");
      msg->SetFillColor(kRed);
    } else if (resultNBMT21 == Quality::Medium) {
      ILOG(Info) << "Quality::medium, setting to orange";
      h->SetFillColor(kOrange);
      msg->Clear();
      msg->AddText("Quality::Medium");
      msg->AddText("PLEASE IGNORE.");
      msg->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
  if (mo->getName() == "mMultHitMT22NB") {
    msg->SetName(Form("%s_msg", mo->GetName()));
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    if (resultNBMT22 == Quality::Good) {
      h->SetFillColor(kGreen);
      msg->Clear();
      msg->AddText("Quality::Good");
      msg->AddText("Mean value is under the limit");
      msg->SetFillColor(kGreen);
    } else if (resultNBMT22 == Quality::Bad) {
      ILOG(Info) << "Quality::Bad, setting to red";
      h->SetFillColor(kRed);
      msg->Clear();
      msg->AddText("Quality::Bad");
      msg->AddText("Mean value is upper the limit");
      msg->AddText("call expert");
      msg->SetFillColor(kRed);
    } else if (resultNBMT22 == Quality::Medium) {
      ILOG(Info) << "Quality::medium, setting to orange";
      h->SetFillColor(kOrange);
      msg->Clear();
      msg->AddText("Quality::Medium");
      msg->AddText("PLEASE IGNORE.");
      msg->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
}

} // namespace o2::quality_control_modules::mid
