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
/// \author Zhen Zhang
///

#include "ITS/ITSFeeCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

#include <fairlogger/Logger.h>
#include <TList.h>
#include <TH2.h>
#include <iostream>

namespace o2::quality_control_modules::its
{

Quality ITSFeeCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;
  bool badStaveCount, badStaveIB, badStaveML, badStaveOL;

  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if (mo->getName() == "General/LinkErrorPlots") {
      result = Quality::Good;
      auto* h = dynamic_cast<TH1D*>(mo->getObject());
      if (h->GetMaximum() > 0) {
        result.set(Quality::Bad);
      }
    }
    if (mo->getName() == "General/ChipErrorPlots") {
      result = Quality::Good;
      auto* h = dynamic_cast<TH1D*>(mo->getObject());
      if (h->GetMaximum() > 0) {
        result.set(Quality::Bad);
      }
    }
    for (int iflag = 0; iflag < NFlags; iflag++) {
      if (mo->getName() == Form("LaneStatus/laneStatusFlag%s", mLaneStatusFlag[iflag].c_str())) {
        result = Quality::Good;
        auto* h = dynamic_cast<TH2I*>(mo->getObject());
        if (h->GetMaximum() > 0) {
          result.set(Quality::Bad);
        }
      }
      if (mo->getName() == Form("LaneStatus/laneStatusOverviewFlag%s", mLaneStatusFlag[iflag].c_str())) {
        result = Quality::Good;
        auto* hp = dynamic_cast<TH2Poly*>(mo->getObject());
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
          for (int ibin = StaveBoundary[ilayer]; ibin < StaveBoundary[ilayer + 1]; ++ibin) {
            if (hp->GetBinContent(ibin) > 0.) {
              countStave++;
            }
            if (ibin < StaveBoundary[3]) {
              // Check if there are staves in the IB with lane in Bad (bins are filled with %)
              if (hp->GetBinContent(ibin) > 0.) {
                badStaveIB = true;
                result.updateMetadata("IB", "bad");
              }
            } else if (ibin < StaveBoundary[5]) {
              // Check if there are staves in the MLs with at least 4 lanes in Bad (bins are filled with %)
              if (hp->GetBinContent(ibin) > 4. / NLanePerStaveLayer[ilayer]) {
                badStaveML = true;
                result.updateMetadata("ML", "bad");
              }
            } else if (ibin < StaveBoundary[7]) {
              // Check if there are staves in the OLs with at least 7 lanes in Bad (bins are filled with %)
              if (hp->GetBinContent(ibin) > 7. / NLanePerStaveLayer[ilayer]) {
                badStaveOL = true;
                result.updateMetadata("OL", "bad");
              }
            }
          } // end loop bins (staves)
          // Initialize metadata for the 7 layers
          result.addMetadata(Form("Layer%d", ilayer), "good");
          // Check if there are more than 25% staves in Bad per layer
          if (countStave > 0.25 * NStaves[ilayer]) {
            badStaveCount = true;
            result.updateMetadata(Form("Layer%d", ilayer), "bad");
          }
        } // end loop over layers
        if (badStaveIB || badStaveML || badStaveOL || badStaveCount) {
          result.set(Quality::Bad);
        }
      } // end lanestatusOverview
    }
    // Adding summary Plots Checks (General, IB, ML, OL)
    for (int isummary = 0; isummary < NSummary; isummary++) {
      bool StatusSummary = true;
      if (mo->getName() == Form("LaneStatusSummary/LaneStatusSummary%s", mSummaryPlots[isummary].c_str())) {
        result = Quality::Good;
        for (int iflag = 0; iflag < 3; iflag++) {
          result.addMetadata(Form("Flag%s", (mLaneStatusFlag[iflag]).c_str()), "good");
          auto* h = dynamic_cast<TH1I*>(mo->getObject());
          if (strcmp(mSummaryPlots[isummary].c_str(), "IB") == 0) {
            if (h->GetBinContent(iflag + 1) > 0) {
              result.updateMetadata(Form("Flag%s", (mLaneStatusFlag[iflag]).c_str()), "bad");
              StatusSummary = false;
            }
          }      // end IB
          else { // General, ML, OL: if #lane > 25% than bad status assigned
            if (h->GetBinContent(iflag + 1) > 0.25 * laneMaxSummaryPlots[isummary]) {
              result.updateMetadata(Form("Flag%s", (mLaneStatusFlag[iflag]).c_str()), "bad");
              StatusSummary = false;
            }
          }
        }
      } // end flag plot loop
      if (!StatusSummary) {
        result.set(Quality::Bad);
      }
    } // end summary loop
    if (mo->getName() == Form("RDHSummary")) {
      result = Quality::Good;
      auto* h = dynamic_cast<TH2I*>(mo->getObject());
      if (h->GetMaximum() > 0) {
        result.set(Quality::Bad);
      }
    }
  }
  return result;
} // end check

std::string ITSFeeCheck::getAcceptedType() { return "TH2I, TH2Poly"; }

void ITSFeeCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  TString status;
  int textColor;

  if (mo->getName() == "General/LinkErrorPlots") {
    auto* h = dynamic_cast<TH1D*>(mo->getObject());
    if (checkResult == Quality::Good) {
      status = "Quality::Good";
      textColor = kGreen;
    } else if (checkResult == Quality::Bad) {
      status = "Quality::BAD (call expert)";
      textColor = kRed;
    }
    tInfoLinkErr = std::make_shared<TLatex>(0.12, 0.835, Form("#bf{%s}", status.Data()));
    tInfoLinkErr->SetTextColor(textColor);
    tInfoLinkErr->SetTextSize(0.08);
    tInfoLinkErr->SetTextFont(23);
    tInfoLinkErr->SetNDC();
    h->GetListOfFunctions()->Add(tInfoLinkErr->Clone());
  }
  if (mo->getName() == "General/ChipErrorPlots") {
    auto* h = dynamic_cast<TH1D*>(mo->getObject());
    if (checkResult == Quality::Good) {
      status = "Quality::Good";
      textColor = kGreen;
    } else if (checkResult == Quality::Bad) {
      status = "Quality::BAD (call expert)";
      textColor = kRed;
    }
    tInfoChipErr = std::make_shared<TLatex>(0.12, 0.835, Form("#bf{%s}", status.Data()));
    tInfoChipErr->SetTextColor(textColor);
    tInfoChipErr->SetTextSize(0.08);
    tInfoChipErr->SetTextFont(23);
    tInfoChipErr->SetNDC();
    h->GetListOfFunctions()->Add(tInfoChipErr->Clone());
  }

  for (int iflag = 0; iflag < NFlags; iflag++) {
    if (mo->getName() == Form("LaneStatus/laneStatusFlag%s", mLaneStatusFlag[iflag].c_str())) {
      auto* h = dynamic_cast<TH2I*>(mo->getObject());
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
    }
    if (mo->getName() == Form("LaneStatus/laneStatusOverviewFlag%s", mLaneStatusFlag[iflag].c_str())) {
      auto* hp = dynamic_cast<TH2Poly*>(mo->getObject());
      if (checkResult == Quality::Good) {
        status = "Quality::GOOD";
        textColor = kGreen;
      } else if (checkResult == Quality::Bad) {
        status = "Quality::BAD (call expert)";
        textColor = kRed;
        for (int ilayer = 0; ilayer < NLayer; ilayer++) {
          if (strcmp(checkResult.getMetadata(Form("Layer%d", ilayer)).c_str(), "bad") == 0) {
            // tInfoLayers[ilayer] = std::make_shared<TLatex>();
            tInfoLayers[ilayer] = std::make_shared<TLatex>(0.37, minTextPosY[ilayer], Form("Layer %d has > 25%% staves with %s in %s", ilayer, ilayer < 3 ? "chips" : "lanes", mLaneStatusFlag[iflag].c_str()));
            tInfoLayers[ilayer]->SetTextColor(kRed + 1);
            tInfoLayers[ilayer]->SetTextSize(0.03);
            tInfoLayers[ilayer]->SetTextFont(43);
            tInfoLayers[ilayer]->SetNDC();
            hp->GetListOfFunctions()->Add(tInfoLayers[ilayer]->Clone());
          } // end check result over layer
        }   // end of loop over layers
        if (strcmp(checkResult.getMetadata("IB").c_str(), "bad") == 0) {
          tInfoIB = std::make_shared<TLatex>(0.40, 0.55, Form("Inner Barrel has chips in %s", mLaneStatusFlag[iflag].c_str()));
          tInfoIB->SetTextColor(kRed + 1);
          tInfoIB->SetTextSize(0.03);
          tInfoIB->SetTextFont(43);
          tInfoIB->SetNDC();
          hp->GetListOfFunctions()->Add(tInfoIB->Clone());
        }
        if (strcmp(checkResult.getMetadata("ML").c_str(), "bad") == 0) {
          tInfoML = std::make_shared<TLatex>(0.42, 0.62, Form("ML have staves in %s", mLaneStatusFlag[iflag].c_str()));
          tInfoML->SetTextColor(kRed + 1);
          tInfoML->SetTextSize(0.03);
          tInfoML->SetTextFont(43);
          tInfoML->SetNDC();
          hp->GetListOfFunctions()->Add(tInfoML->Clone());
        }
        if (strcmp(checkResult.getMetadata("OL").c_str(), "bad") == 0) {
          tInfoOL = std::make_shared<TLatex>(0.415, 0.78, Form("OL have staves in %s", mLaneStatusFlag[iflag].c_str()));
          tInfoOL->SetTextColor(kRed + 1);
          tInfoOL->SetTextSize(0.03);
          tInfoOL->SetTextFont(43);
          tInfoOL->SetNDC();
          hp->GetListOfFunctions()->Add(tInfoOL->Clone());
        }
      }
      tInfo = std::make_shared<TLatex>(0.12, 0.835, Form("#bf{%s}", status.Data()));
      tInfo->SetTextColor(textColor);
      tInfo->SetTextSize(0.03);
      tInfo->SetTextFont(43);
      tInfo->SetNDC();
      hp->GetListOfFunctions()->Add(tInfo->Clone());
    }
  } // end flags
  for (int isummary = 0; isummary < NSummary; isummary++) {
    if (mo->getName() == Form("LaneStatusSummary/LaneStatusSummary%s", mSummaryPlots[isummary].c_str())) {
      auto* h = dynamic_cast<TH1I*>(mo->getObject());
      if (checkResult == Quality::Good) {
        status = "Quality::GOOD";
        textColor = kGreen;
      } else if (checkResult == Quality::Bad) {
        status = "Quality::BAD (call expert)";
        textColor = kRed;
        for (int iflag = 0; iflag < NFlags; iflag++) {
          if (strcmp(checkResult.getMetadata(Form("Flag%s", mLaneStatusFlag[iflag].c_str())).c_str(), "bad") == 0) {
            if (strcmp(mSummaryPlots[isummary].c_str(), "IB") == 0) {
              tInfoSummary[iflag] = std::make_shared<TLatex>(0.12, 0.75 - 0.07 * iflag, Form("%s: > 1 lane in %s", mSummaryPlots[isummary].c_str(), mLaneStatusFlag[iflag].c_str()));
            } else {
              tInfoSummary[iflag] = std::make_shared<TLatex>(0.12, 0.75 - 0.07 * iflag, Form("%s: > 25%% of staves in %s", mSummaryPlots[isummary].c_str(), mLaneStatusFlag[iflag].c_str()));
            }
            tInfoSummary[iflag]->SetTextColor(kRed + 1);
            tInfoSummary[iflag]->SetTextSize(0.04);
            tInfoSummary[iflag]->SetTextFont(43);
            h->GetListOfFunctions()->Add(tInfoSummary[iflag]->Clone());
          }
        }
      }
      tInfo = std::make_shared<TLatex>(0.12, 0.835, Form("#bf{%s}", status.Data()));
      tInfo->SetTextColor(textColor);
      tInfo->SetTextSize(0.04);
      tInfo->SetTextFont(43);
      tInfo->SetNDC();
      h->GetListOfFunctions()->Add(tInfo->Clone());
    }
  } // end summary
  if (mo->getName() == Form("RDHSummary")) {
    auto* h = dynamic_cast<TH2I*>(mo->getObject());
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
  }
} // end beautify

} // namespace o2::quality_control_modules::its
