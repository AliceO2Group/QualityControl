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
/// \file   ITSFeeCheck.cxx
/// \author LiAng Zhang
/// \author Jian Liu
/// \author Antonio Palasciano
///

#include "ITS/ITSFeeCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include <DataFormatsQualityControl/FlagTypeFactory.h>

#include <fairlogger/Logger.h>
#include <TH2.h>
#include "Common/Utils.h"

namespace o2::quality_control_modules::its
{

Quality ITSFeeCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{

  Quality result = Quality::Null;
  bool badStaveCount, badStaveIB, badStaveML, badStaveOL;

  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    for (int iflag = 0; iflag < NFlags; iflag++) {
      if (mo->getName() == Form("LaneStatus/laneStatusFlag%s", mLaneStatusFlag[iflag].c_str())) {
        result = Quality::Good;
        auto* h = dynamic_cast<TH2I*>(mo->getObject());
        if (h == nullptr) {
          ILOG(Error, Support) << "could not cast LaneStatusFlag to TH2I*" << ENDM;
          continue;
        }
        if (h->GetMaximum() > 0) {
          result.set(Quality::Bad);
        }
      }
      if (mo->getName() == Form("LaneStatus/laneStatusOverviewFlag%s", mLaneStatusFlag[iflag].c_str())) {
        result.set(Quality::Good);
        auto* hp = dynamic_cast<TH2Poly*>(mo->getObject());
        if (hp == nullptr) {
          ILOG(Error, Support) << "could not cast laneStatusOverview to THPollyF*" << ENDM;
          continue;
        }
        badStaveIB = false;
        badStaveML = false;
        badStaveOL = false;
        // Initialization of metaData for IB, ML, OL
        result.addMetadata("IB", "good");
        result.addMetadata("ML", "good");
        result.addMetadata("OL", "good");
        for (int ilayer = 0; ilayer < NLayer; ilayer++) {
          int countStave = 0;
          badStaveCount = false;
          for (int ibin = StaveBoundary[ilayer] + 1; ibin <= StaveBoundary[ilayer + 1]; ++ibin) {
            if (ibin <= StaveBoundary[3]) {
              // Check if there are staves in the IB with lane in Bad (bins are filled with %)
              maxbadchipsIB = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "maxbadchipsIB", maxbadchipsIB);
              if (hp->GetBinContent(ibin) > maxbadchipsIB / 9.) {
                badStaveIB = true;
                result.updateMetadata("IB", "medium");
                countStave++;
                TString text = "Medium:IB stave has many NOK chips;";
                if (!checkReason(result, text))
                  result.addFlag(o2::quality_control::FlagTypeFactory::Unknown(), text.Data());
              }
            } else if (ibin <= StaveBoundary[5]) {
              // Check if there are staves in the MLs with at least 4 lanes in Bad (bins are filled with %)
              maxbadlanesML = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "maxbadlanesML", maxbadlanesML);
              if (hp->GetBinContent(ibin) > maxbadlanesML / (1. * NLanePerStaveLayer[ilayer])) {
                badStaveML = true;
                result.updateMetadata("ML", "medium");
                countStave++;
                TString text = "Medium:ML stave has many NOK chips;";
                if (!checkReason(result, text))
                  result.addFlag(o2::quality_control::FlagTypeFactory::Unknown(), text.Data());
              }
            } else if (ibin <= StaveBoundary[7]) {
              // Check if there are staves in the OLs with at least 7 lanes in Bad (bins are filled with %)
              maxbadlanesOL = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "maxbadlanesOL", maxbadlanesOL);
              if (hp->GetBinContent(ibin) > maxbadlanesOL / (1. * NLanePerStaveLayer[ilayer])) {
                badStaveOL = true;
                result.updateMetadata("OL", "medium");
                countStave++;
                TString text = "Medium:OL stave has many NOK chips;";
                if (!checkReason(result, text))
                  result.addFlag(o2::quality_control::FlagTypeFactory::Unknown(), text.Data());
              }
            }
          } // end loop bins (staves)
          // Initialize metadata for the 7 layers
          result.addMetadata(Form("Layer%d", ilayer), "good");
          // Check if there are more than 25% staves in Bad per layer
          if (countStave > 0.25 * NStaves[ilayer]) {
            badStaveCount = true;
            result.updateMetadata(Form("Layer%d", ilayer), "bad");
            result.addFlag(o2::quality_control::FlagTypeFactory::Unknown(), Form("BAD:Layer%d has many NOK staves;", ilayer));
          }
        } // end loop over layers
        if (badStaveIB || badStaveML || badStaveOL) {
          result.set(Quality::Medium);
        }
        if (badStaveCount) {
          result.set(Quality::Bad);
        }
      } // end lanestatusOverview
    }
    // Adding summary Plots Checks (General)
    if (mo->getName() == "LaneStatusSummary/LaneStatusSummaryGlobal") {
      result = Quality::Good;
      auto* h = dynamic_cast<TH1D*>(mo->getObject());
      if (h == nullptr) {
        ILOG(Error, Support) << "could not cast LaneStatusSummaryGlobal to TH1D*" << ENDM;
        continue;
      }
      result.addMetadata("SummaryGlobal", "good");
      maxfractionbadlanes = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "maxfractionbadlanes", maxfractionbadlanes);
      if (h->GetBinContent(1) + h->GetBinContent(2) + h->GetBinContent(3) > maxfractionbadlanes) {
        result.updateMetadata("SummaryGlobal", "bad");
        result.set(Quality::Bad);
        result.addFlag(o2::quality_control::FlagTypeFactory::Unknown(), Form("BAD:>%.0f %% of the lanes are bad", (h->GetBinContent(1) + h->GetBinContent(2) + h->GetBinContent(3)) * 100));
      }
    } // end summary loop
    if (mo->getName() == Form("RDHSummary")) {
      result = Quality::Good;
      auto* h = dynamic_cast<TH2I*>(mo->getObject());
      if (h->GetMaximum() > 0) {
        result.set(Quality::Bad);
      }
    }

    if (((std::string)mo->getName()).find("TriggerVsFeeid") != std::string::npos) {
      result.set(Quality::Good);
      auto* h = dynamic_cast<TH2I*>(mo->getObject());
      if (h == nullptr) {
        ILOG(Error, Support) << "could not cast TriggerVsFeeid to TH2I*" << ENDM;
        continue;
      }

      std::vector<int> skipbins = convertToArray<int>(o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "skipbinstrg", skipbinstrg));
      std::vector<int> skipfeeid = convertToArray<int>(o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "skipfeeids", skipfeeids));
      maxtfdifference = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "maxTFdifferenceAllowed", maxtfdifference);

      for (int itrg = 1; itrg <= h->GetNbinsY(); itrg++) {
        result.addMetadata(h->GetYaxis()->GetBinLabel(itrg), "good");
      }

      TString TrgAtLeastOne = "ORBIT HB PHYSICS TF";
      TString TrgExactlyOne = "SOC";
      // The others are requested to have no entries

      int min_n_tf = INT_MAX, max_n_tf = 0;

      for (int itrg = 0; itrg < h->GetNbinsY(); itrg++) {
        if (std::find(skipbins.begin(), skipbins.end(), itrg + 1) != skipbins.end()) {
          continue;
        }

        bool badTrigger = false;

        TString trgname = (TString)(h->GetYaxis()->GetBinLabel(itrg + 1));

        for (int ifee = 1; ifee <= h->GetNbinsX(); ifee++) {

          if (std::find(skipfeeid.begin(), skipfeeid.end(), ifee) != skipfeeid.end())
            continue;

          int bincontent = (int)(h->GetBinContent(ifee, itrg + 1));

          // checking trigger flags supposed to have at least one entry
          if (TrgAtLeastOne.Contains(trgname)) {
            if (bincontent < 1) {
              badTrigger = true;
              break;
            }
          }
          // checking trigger flags supposed to have exactly one entry
          else if (TrgExactlyOne.Contains(trgname)) {
            if (bincontent != 1) {
              badTrigger = true;
              break;
            }
          }
          // checking trigger flags supposed to have no entries
          else {
            if (bincontent > 0) {
              badTrigger = true;
              break;
            }
          }

          if (trgname == "TF" && maxtfdifference > 0) {
            min_n_tf = std::min(min_n_tf, bincontent);
            max_n_tf = std::max(max_n_tf, bincontent);
          }
        }

        if (trgname == "TF" && maxtfdifference > 0 && (max_n_tf - min_n_tf > maxtfdifference))
          badTrigger = true;

        if (badTrigger) {
          result.updateMetadata(h->GetYaxis()->GetBinLabel(itrg + 1), "bad");
          result.set(Quality::Bad);
          std::string extraText = (!strcmp(h->GetYaxis()->GetBinLabel(itrg + 1), "PHYSICS")) ? "(OK if it's COSMICS/SYNTHETIC)" : "";
          result.addFlag(o2::quality_control::FlagTypeFactory::Unknown(), Form("BAD:Trigger flag %s of bad quality %s", h->GetYaxis()->GetBinLabel(itrg + 1), extraText.c_str()));
        }
      }
    }

    if (mo->getName() == "PayloadSize") {
      auto* h = dynamic_cast<TH2F*>(mo->getObject());
      if (h == nullptr) {
        ILOG(Error, Support) << "could not cast PayloadSize to TH2F*" << ENDM;
        continue;
      }
      result.set(Quality::Good);
      result.addMetadata("CheckTechnicals", "good");
      result.addMetadata("CheckTechnicalsFeeid", "good");
      std::vector<int> skipfeeid = convertToArray<int>(o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "skipfeeids", skipfeeids));
      minPayloadSize = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "minPayloadSize", minPayloadSize);
      if (h->Integral(1, 432, h->GetYaxis()->FindBin(minPayloadSize), h->GetYaxis()->GetLast()) > 0) {
        result.set(Quality::Bad);
        result.updateMetadata("CheckTechnicals", "bad");
      }
      int countFeeids = 0;
      for (int ix = 1; ix <= h->GetNbinsX(); ix++) {
        for (int iy = 1; iy <= h->GetNbinsY(); iy++) {
          if (h->GetBinContent(ix, iy) > 0) {
            countFeeids++;
            break;
          }
        }
      }
      if (countFeeids < 432 - (int)skipfeeid.size()) {
        result.set(Quality::Bad);
        result.updateMetadata("CheckTechnicalsFeeid", "bad");
      }
    }

    if (((std::string)mo->getName()).find("TrailerCount") != std::string::npos) {

      auto activity = mo->getActivity();
      auto* h = dynamic_cast<TH2I*>(mo->getObject());
      if (h == nullptr) {
        ILOG(Error, Support) << "could not cast TrailerCount to TH2I*" << ENDM;
        continue;
      }
      result.set(Quality::Good);
      result.addMetadata("CheckROFRate", "good");
      expectedROFperOrbit = stoi(mCustomParameters.at("expectedROFperOrbit", activity));
      if (h->Integral(1, 432, 1, h->GetYaxis()->FindBin(expectedROFperOrbit) - 1) > 0 || h->Integral(1, 432, h->GetYaxis()->FindBin(expectedROFperOrbit) + 1, h->GetYaxis()->GetLast()) > 0) {
        result.set(Quality::Bad);
        result.updateMetadata("expectedROFperOrbit", "bad");
        result.addFlag(o2::quality_control::FlagTypeFactory::Unknown(), "ITS seems to be misconfigured");
      }
    }
  } // end loop on MOs
  return result;
} // end check

std::string ITSFeeCheck::getAcceptedType() { return "TH2I, TH2Poly"; }

void ITSFeeCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
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

  for (int iflag = 0; iflag < NFlags; iflag++) {
    if (mo->getName() == Form("LaneStatus/laneStatusFlag%s", mLaneStatusFlag[iflag].c_str())) {
      auto* h = dynamic_cast<TH2I*>(mo->getObject());
      if (h == nullptr) {
        ILOG(Error, Support) << "could not cast LaneSatusFlag to TH2I*" << ENDM;
        continue;
      }
      if (checkResult == Quality::Good) {
        status = "Quality::GOOD";
        textColor = kGreen;
      } else if (checkResult == Quality::Bad) {
        status = "Quality::BAD (call expert)";
        textColor = kRed;
      }
      tInfo = std::make_shared<TLatex>(0.12, 0.835, Form("#bf{%s}", status.Data()));
      tInfo->SetTextColor(textColor);
      tInfo->SetTextSize(0.04);
      tInfo->SetTextFont(43);
      tInfo->SetNDC();
      h->GetListOfFunctions()->Add(tInfo->Clone());
      if (ShifterInfoText[mo->getName()] != "")
        h->GetListOfFunctions()->Add(tShifterInfo->Clone());
    }
    if (mo->getName() == Form("LaneStatus/laneStatusOverviewFlag%s", mLaneStatusFlag[iflag].c_str())) {
      auto* hp = dynamic_cast<TH2Poly*>(mo->getObject());
      if (hp == nullptr) {
        ILOG(Error, Support) << "could not cast LaneSatusOverview to THPolly*" << ENDM;
        continue;
      }
      if (checkResult == Quality::Good) {
        status = "Quality::GOOD";
        textColor = kGreen;
      } else if (checkResult == Quality::Bad || checkResult == Quality::Medium) {
        status = "Quality::BAD (call expert)";
        textColor = kRed;
        if (strcmp(checkResult.getMetadata("IB").c_str(), "medium") == 0) {
          status = "Quality::Medium (do not call, inform expert on MM)";
          textColor = kOrange;
          maxbadchipsIB = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "maxbadchipsIB", maxbadchipsIB);
          tInfoIB = std::make_shared<TLatex>(0.40, 0.55, Form("Inner Barrel has stave(s) with >%d chips in %s", maxbadchipsIB, mLaneStatusFlag[iflag].c_str()));
          tInfoIB->SetTextColor(kOrange);
          tInfoIB->SetTextSize(0.03);
          tInfoIB->SetTextFont(43);
          tInfoIB->SetNDC();
          hp->GetListOfFunctions()->Add(tInfoIB->Clone());
        }
        if (strcmp(checkResult.getMetadata("ML").c_str(), "medium") == 0) {
          status = "Quality::Medium (do not call, inform expert on MM)";
          textColor = kOrange;
          maxbadlanesML = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "maxbadlanesML", maxbadlanesML);
          tInfoML = std::make_shared<TLatex>(0.42, 0.62, Form("ML have stave(s) with >%d lanes in %s", maxbadlanesML, mLaneStatusFlag[iflag].c_str()));
          tInfoML->SetTextColor(kOrange);
          tInfoML->SetTextSize(0.03);
          tInfoML->SetTextFont(43);
          tInfoML->SetNDC();
          hp->GetListOfFunctions()->Add(tInfoML->Clone());
        }
        if (strcmp(checkResult.getMetadata("OL").c_str(), "medium") == 0) {
          status = "Quality::Medium (do not call, inform expert on MM)";
          textColor = kOrange;
          maxbadlanesOL = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "maxbadlanesOL", maxbadlanesOL);
          tInfoOL = std::make_shared<TLatex>(0.415, 0.78, Form("OL have staves with >%d lanes in %s", maxbadlanesOL, mLaneStatusFlag[iflag].c_str()));
          tInfoOL->SetTextColor(kOrange);
          tInfoOL->SetTextSize(0.03);
          tInfoOL->SetTextFont(43);
          tInfoOL->SetNDC();
          hp->GetListOfFunctions()->Add(tInfoOL->Clone());
        }
        for (int ilayer = 0; ilayer < NLayer; ilayer++) {
          if (strcmp(checkResult.getMetadata(Form("Layer%d", ilayer)).c_str(), "bad") == 0) {
            status = "Quality::Bad (call expert)";
            textColor = kRed;
            maxbadchipsIB = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "maxbadchipsIB", maxbadchipsIB);
            maxbadlanesML = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "maxbadlanesML", maxbadlanesML);
            maxbadlanesOL = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "maxbadlanesOL", maxbadlanesOL);

            int cut = ilayer < 3 ? maxbadchipsIB : ilayer < 5 ? maxbadlanesML
                                                              : maxbadlanesOL;
            tInfoLayers[ilayer] = std::make_shared<TLatex>(0.37, minTextPosY[ilayer], Form("Layer %d has > 25%% staves with >%d %s in %s", ilayer, cut, ilayer < 3 ? "chips" : "lanes", mLaneStatusFlag[iflag].c_str()));
            tInfoLayers[ilayer]->SetTextColor(kRed);
            tInfoLayers[ilayer]->SetTextSize(0.03);
            tInfoLayers[ilayer]->SetTextFont(43);
            tInfoLayers[ilayer]->SetNDC();
            hp->GetListOfFunctions()->Add(tInfoLayers[ilayer]->Clone());
          } // end check result over layer
        }   // end of loop over layers
      }
      tInfo = std::make_shared<TLatex>(0.05, 0.95, Form("#bf{%s}", status.Data()));
      tInfo->SetTextColor(textColor);
      tInfo->SetTextSize(0.06);
      tInfo->SetTextFont(43);
      tInfo->SetNDC();
      hp->GetListOfFunctions()->Add(tInfo->Clone());
      if (ShifterInfoText[mo->getName()] != "")
        hp->GetListOfFunctions()->Add(tShifterInfo->Clone());
    }
  } // end flags
  if (mo->getName() == "LaneStatusSummary/LaneStatusSummaryGlobal") {
    auto* h = dynamic_cast<TH1D*>(mo->getObject());
    if (h == nullptr) {
      ILOG(Error, Support) << "could not cast LaneSatutsSummaryGlobal to TH1D*" << ENDM;
      return;
    }
    if (checkResult == Quality::Good) {
      status = "Quality::GOOD";
      textColor = kGreen;
    } else if (checkResult == Quality::Bad) {
      status = "Quality::BAD (call expert)";
      textColor = kRed;
      if (strcmp(checkResult.getMetadata("SummaryGlobal").c_str(), "bad") == 0) {
        maxfractionbadlanes = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "maxfractionbadlanes", maxfractionbadlanes);
        tInfoSummary = std::make_shared<TLatex>(0.12, 0.5, Form(">%.0f %% of the lanes are bad", maxfractionbadlanes * 100));
        tInfoSummary->SetTextColor(kRed);
        tInfoSummary->SetTextSize(0.05);
        tInfoSummary->SetTextFont(43);
        tInfoSummary->SetNDC();
        h->GetListOfFunctions()->Add(tInfoSummary->Clone());
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
  if (mo->getName() == Form("RDHSummary")) {
    auto* h = dynamic_cast<TH2I*>(mo->getObject());
    if (h == nullptr) {
      ILOG(Error, Support) << "could not cast RDHSummary to TH2I*" << ENDM;
      return;
    }
    if (checkResult == Quality::Good) {
      status = "Quality::GOOD";
      textColor = kGreen;
    } else if (checkResult == Quality::Bad) {
      status = "Quality::BAD (call expert)";
      textColor = kRed;
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

  // trigger plot
  if (((std::string)mo->getName()).find("TriggerVsFeeid") != std::string::npos) {
    auto* h = dynamic_cast<TH2I*>(mo->getObject());
    if (h == nullptr) {
      ILOG(Error, Support) << "could not cast TriggerVsFeeId to TH2I*" << ENDM;
      return;
    }
    if (checkResult == Quality::Good) {
      status = "Quality::GOOD";
      textColor = kGreen;
    } else if (checkResult == Quality::Bad) {
      status = "Quality::BAD (call expert)";
      textColor = kRed;
      for (int itrg = 1; itrg <= h->GetNbinsY(); itrg++) {
        ILOG(Debug, Devel) << checkResult.getMetadata(h->GetYaxis()->GetBinLabel(itrg)).c_str() << ENDM;
        if (strcmp(checkResult.getMetadata(h->GetYaxis()->GetBinLabel(itrg)).c_str(), "bad") == 0) {
          std::string extraText = (!strcmp(h->GetYaxis()->GetBinLabel(itrg), "PHYSICS")) ? "(OK if it's COSMICS/SYNTHETIC)" : "";
          tInfoTrg[itrg - 1] = std::make_shared<TLatex>(0.3, 0.1 + 0.05 * (itrg - 1), Form("Trigger flag %s of bad quality %s", h->GetYaxis()->GetBinLabel(itrg), extraText.c_str()));
          tInfoTrg[itrg - 1]->SetTextColor(kRed);
          tInfoTrg[itrg - 1]->SetTextSize(0.03);
          tInfoTrg[itrg - 1]->SetTextFont(43);
          tInfoTrg[itrg - 1]->SetNDC();
          h->GetListOfFunctions()->Add(tInfoTrg[itrg - 1]->Clone());
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

  // payload size plot
  if (mo->getName() == "PayloadSize") {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    if (h == nullptr) {
      ILOG(Error, Support) << "could not cast PayloadSize to TH2F*" << ENDM;
      return;
    }
    if (checkResult == Quality::Good) {
      status = "Quality::GOOD";
      textColor = kGreen;
    } else {
      status = "Quality::Bad (call expert)";
      textColor = kRed;
      if (strcmp(checkResult.getMetadata("CheckTechnicals").c_str(), "bad") == 0) {
        tInfoPL[0] = std::make_shared<TLatex>(0.3, 0.6, "Payload size too large for technical runs");
        tInfoPL[0]->SetNDC();
        tInfoPL[0]->SetTextFont(43);
        tInfoPL[0]->SetTextSize(0.04);
        tInfoPL[0]->SetTextColor(kRed);
        h->GetListOfFunctions()->Add(tInfoPL[0]->Clone());
      }
      if (strcmp(checkResult.getMetadata("CheckTechnicalsFeeid").c_str(), "bad") == 0) {
        tInfoPL[1] = std::make_shared<TLatex>(0.3, 0.55, "Payload size is missing for some FeeIDs");
        tInfoPL[1]->SetNDC();
        tInfoPL[1]->SetTextFont(43);
        tInfoPL[1]->SetTextSize(0.04);
        tInfoPL[1]->SetTextColor(kRed);
        h->GetListOfFunctions()->Add(tInfoPL[1]->Clone());
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

  if (((std::string)mo->getName()).find("TrailerCount") != std::string::npos) {
    auto* h = dynamic_cast<TH2I*>(mo->getObject());
    if (h == nullptr) {
      ILOG(Error, Support) << "could not cast TrailerCount to TH2F*" << ENDM;
      return;
    }
    if (checkResult == Quality::Good) {
      status = "Quality::GOOD";
      textColor = kGreen;
    } else {
      status = "Quality::Bad (call expert)";
      textColor = kRed;
      tInfoPL[1] = std::make_shared<TLatex>(0.3, 0.55, "MISCONFIGURATION. CALL EXPERTS.");
      tInfoPL[1]->SetNDC();
      tInfoPL[1]->SetTextFont(43);
      tInfoPL[1]->SetTextSize(0.04);
      tInfoPL[1]->SetTextColor(kRed);
      h->GetListOfFunctions()->Add(tInfoPL[1]->Clone());
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
}

bool ITSFeeCheck::checkReason(Quality checkResult, TString text)
{
  auto flags = checkResult.getFlags();
  for (int i = 0; i < int(flags.size()); i++) {
    if (text.Contains(flags[i].second.c_str()))
      return true;
  }
  return false;
}

} // namespace o2::quality_control_modules::its
