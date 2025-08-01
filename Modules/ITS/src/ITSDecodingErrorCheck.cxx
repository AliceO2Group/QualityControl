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
/// \file   ITSDecodingErrorCheck.cxx
/// \author Zhen Zhang
///

#include "ITS/ITSDecodingErrorCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include "ITSMFTReconstruction/DecodingStat.h"
#include <DataFormatsQualityControl/FlagTypeFactory.h>

#include <fairlogger/Logger.h>
#include "Common/Utils.h"

namespace o2::quality_control_modules::its
{

Quality ITSDecodingErrorCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  // set timer
  if (nCycle == 0) {
    start = std::chrono::high_resolution_clock::now();
    nCycle++;
  } else {
    end = std::chrono::high_resolution_clock::now();
    TIME = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
  }
  std::vector<int> vDecErrorLimits = convertToArray<int>(o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "DecLinkErrorLimits", ""));
  if (vDecErrorLimits.size() != o2::itsmft::GBTLinkDecodingStat::NErrorsDefined) {
    ILOG(Error, Support) << "Incorrect vector with DecodingError limits, check .json" << ENDM;
    doFlatCheck = true;
  }
  std::vector<float> vDecErrorLimitsRatio = convertToArray<float>(o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "DecLinkErrorLimitsRatio", ""));
  if (vDecErrorLimitsRatio.size() != o2::itsmft::GBTLinkDecodingStat::NErrorsDefined) {
    ILOG(Error, Support) << "Incorrect vector with DecodingError limits Ratio, check .json" << ENDM;
    doFlatCheck = true;
  }
  std::vector<int> vDecErrorType = convertToArray<int>(o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "DecLinkErrorType", ""));
  if (vDecErrorType.size() != o2::itsmft::GBTLinkDecodingStat::NErrorsDefined) {
    ILOG(Error, Support) << "Incorrect vector with DecodingError Type, check .json" << ENDM;
    doFlatCheck = true;
  }

  bool checkFeeIDOnce = o2::quality_control_modules::common::getFromConfig<bool>(mCustomParameters, "checkfeeIDonlyonce", "");

  Quality result = Quality::Null;
  for (auto& [moName, mo] : *moMap) {
    (void)moName;

    if ((std::string)mo->getName() == "General/ChipErrorPlots") {
      result = Quality::Good;
      auto* h = dynamic_cast<TH1D*>(mo->getObject());
      if (h == nullptr) {
        ILOG(Error, Support) << "could not cast ChipError plots to TH1D*" << ENDM;
        continue;
      }
      if (h->GetMaximum() > 200)
        result.set(Quality::Bad);
    }

    if ((std::string)mo->GetName() == "General/LinkErrorVsFeeid") {

      result = Quality::Good;
      auto* h = dynamic_cast<TH2D*>(mo->getObject());
      if (h == nullptr) {
        ILOG(Error, Support) << "could not cast LinkErrorVsFeeid plot to TH2D*" << ENDM;
        continue;
      }
      for (int ifee = 0; ifee < h->GetNbinsX(); ifee++) {
        for (int ierr = 0; ierr < h->GetNbinsY(); ierr++) {

          if (doFlatCheck && h->GetBinContent(ifee + 1, ierr + 1) < 200) { // ok if below threshold
            continue;
          }
          if (!doFlatCheck && (vDecErrorLimits[ierr] < 0 || h->GetBinContent(ifee + 1, ierr + 1) < vDecErrorLimits[ierr])) { // ok if below threshold or if threshold is -1
            continue;
          }

          if (vAlreadyCheckedFeeIDs.count(ifee) > 0) {
            std::vector<int> flagskip = vAlreadyCheckedFeeIDs[ifee];
            if (std::find(flagskip.begin(), flagskip.end(), ierr) != flagskip.end()) {
              continue; // bin to be skipped
            }
          }

          // if here, quality is BAD

          if (checkFeeIDOnce) { // gives instruction to exclude chech on this bin from now on
            if (vAlreadyCheckedFeeIDs.count(ifee) > 0) {
              vAlreadyCheckedFeeIDs[ifee].push_back(ierr);
            } else {
              vAlreadyCheckedFeeIDs[ifee] = std::vector<int>{ ierr };
            }
          }

          result.set(Quality::Bad);
          result.addFlag(o2::quality_control::FlagTypeFactory::Unknown(), Form("BAD: ID = %d, %s", ierr, std::string(statistics.ErrNames[ierr]).c_str()));

        } // end of y axis loop
      }   // end of x axis loop
    }     // end of check on General/LinkErrorVsFeeid

    if (((std::string)mo->getName()).find("General/LinkErrorPlots") != std::string::npos) {
      result = Quality::Good;
      auto* h = dynamic_cast<TH1D*>(mo->getObject());
      if (h == nullptr) {
        ILOG(Error, Support) << "could not cast LinkErrorPlots to TH1D*" << ENDM;
        continue;
      }

      if (doFlatCheck) {
        if (h->GetMaximum() > 200)
          result.set(Quality::Bad);

      } else {
        for (int iBin = 1; iBin <= h->GetNbinsX(); iBin++) {

          if (vDecErrorLimits[iBin - 1] < 0)
            continue; // skipping bin

          if (vDecErrorType[iBin - 1] == 1 && TIME != 0) {
            if (vDecErrorLimitsRatio[iBin - 1] <= h->GetBinContent(iBin) / TIME) {
              vListErrorIdBad.push_back(iBin - 1);
              result.set(Quality::Bad);
              result.addFlag(o2::quality_control::FlagTypeFactory::Unknown(), Form("BAD: ID = %d, %s", iBin - 1, std::string(statistics.ErrNames[iBin - 1]).c_str()));
            } else if (vDecErrorLimitsRatio[iBin - 1] / 2 < h->GetBinContent(iBin) / TIME) {
              vListErrorIdMedium.push_back(iBin - 1);
              if (result != Quality::Bad) {
                result.addFlag(o2::quality_control::FlagTypeFactory::Unknown(), Form("Medium: ID = %d, %s", iBin - 1, std::string(statistics.ErrNames[iBin - 1]).c_str()));
                result.set(Quality::Medium);
              }
            }
          } else { // normal check, as we have in the code now
            if (vDecErrorLimits[iBin - 1] <= h->GetBinContent(iBin)) {
              vListErrorIdBad.push_back(iBin - 1);
              result.set(Quality::Bad);
              result.addFlag(o2::quality_control::FlagTypeFactory::Unknown(), Form("BAD: ID = %d, %s", iBin - 1, std::string(statistics.ErrNames[iBin - 1]).c_str()));
            } else if (vDecErrorLimits[iBin - 1] / 2 < h->GetBinContent(iBin)) {
              vListErrorIdMedium.push_back(iBin - 1);
              if (result != Quality::Bad) {
                result.addFlag(o2::quality_control::FlagTypeFactory::Unknown(), Form("Medium: ID = %d, %s", iBin - 1, std::string(statistics.ErrNames[iBin - 1]).c_str()));
                result.set(Quality::Medium);
              }
            }
          }
        }
      }
    }
  }
  return result;
}

void ITSDecodingErrorCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  std::vector<std::string> vPlotWithTextMessage = convertToArray<std::string>(o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "plotWithTextMessage", ""));
  std::vector<std::string> vTextMessage = convertToArray<std::string>(o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "textMessage", ""));
  std::map<std::string, std::string> ShifterInfoText;

  if ((int)vTextMessage.size() == (int)vPlotWithTextMessage.size()) {
    for (int i = 0; i < (int)vTextMessage.size(); i++) {
      ShifterInfoText[vPlotWithTextMessage[i]] = vTextMessage[i];
    }
  } else
    ILOG(Warning, Support) << "Bad list of plot with TextMessages for shifter, check .json" << ENDM;

  std::shared_ptr<TLatex> tShifterInfo = std::make_shared<TLatex>(0.005, 0.006, Form("#bf{%s}", TString(ShifterInfoText[mo->getName()]).Data()));
  tShifterInfo->SetTextSize(0.04);
  tShifterInfo->SetTextFont(43);
  tShifterInfo->SetNDC();

  TString status;
  int textColor;

  if (((std::string)mo->GetName()).find("General/LinkErrorVsFeeid") != std::string::npos) {

    auto* h = dynamic_cast<TH2D*>(mo->getObject());
    if (h == nullptr) {
      ILOG(Error, Support) << "could not cast LinkErrorVsFeeid plot to TH2D*" << ENDM;
      return;
    }
    if (checkResult == Quality::Good) {
      status = "Quality::GOOD";
      textColor = kGreen;
    } else if (checkResult == Quality::Bad) {
      status = "Quality::BAD (call expert)";
      textColor = kRed + 2;
    }

    tInfo = std::make_shared<TLatex>(0.05, 0.95, Form("#bf{%s}", status.Data()));
    tInfo->SetTextColor(textColor);
    tInfo->SetTextSize(0.06);
    tInfo->SetTextFont(43);
    tInfo->SetNDC();
    h->GetListOfFunctions()->Add(tInfo->Clone());

  } // end of beautify LinkErrorVsFeeid

  if ((((std::string)mo->getName()).find("General/LinkErrorPlots") != std::string::npos) || (mo->getName() == "General/ChipErrorPlots")) {
    auto* h = dynamic_cast<TH1D*>(mo->getObject());
    if (h == nullptr) {
      ILOG(Error, Support) << "could not cast LinkErrorPlots to TH1D*" << ENDM;
      return;
    }
    if (checkResult == Quality::Good) {
      status = "Quality::GOOD";
      textColor = kGreen;
    } else {

      if (checkResult == Quality::Bad) {
        status = "Quality::BAD (call expert)";
        for (int id = 0; id < vListErrorIdBad.size(); id++) {
          int currentError = vListErrorIdBad[id];
          tInfo = std::make_shared<TLatex>(0.12, 0.835 - 0.04 * (id + 1), Form("BAD: ID = %d, %s", currentError, std::string(statistics.ErrNames[currentError]).c_str()));
          tInfo->SetTextColor(kRed + 2);
          tInfo->SetTextSize(0.04);
          tInfo->SetTextFont(43);
          tInfo->SetNDC();
          h->GetListOfFunctions()->Add(tInfo->Clone());
        }
        textColor = kRed + 2;
      }
      if (vListErrorIdMedium.size() > 0) {
        if (checkResult == Quality::Medium) {
          status = "Quality::Medium";
          textColor = kOrange;
        }
        for (int id = 0; id < vListErrorIdMedium.size(); id++) {
          int currentError = vListErrorIdMedium[id];
          tInfo = std::make_shared<TLatex>(0.12, 0.6 - 0.04 * (id + 1), Form("Medium: ID = %d, %s", currentError, std::string(statistics.ErrNames[currentError]).c_str()));
          tInfo->SetTextColor(kOrange + 1);
          tInfo->SetTextSize(0.04);
          tInfo->SetTextFont(43);
          tInfo->SetNDC();
          h->GetListOfFunctions()->Add(tInfo->Clone());
        }
      }
    }
    tInfo = std::make_shared<TLatex>(0.05, 0.95, Form("#bf{%s}", status.Data()));
    tInfo->SetTextColor(textColor);
    tInfo->SetTextSize(0.06);
    tInfo->SetTextFont(43);
    tInfo->SetNDC();
    h->GetListOfFunctions()->Add(tInfo->Clone());
    if (ShifterInfoText[mo->getName()] != "")
      h->GetListOfFunctions()->Add(tShifterInfo->Clone());
  }
  vListErrorIdBad.clear();
  vListErrorIdMedium.clear();
}

} // namespace o2::quality_control_modules::its
