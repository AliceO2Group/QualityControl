// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   PhysicsPreclustersCheck.cxx
/// \author Andrea Ferrero, Sebastien Perrin
///

#include "MCH/PhysicsPreclustersCheck.h"
#include "MCHMappingInterface/Segmentation.h"
#include "MCHMappingSegContour/CathodeSegmentationContours.h"
#include "MCH/GlobalHistogram.h"
#include "MUONCommon/MergeableTH2Ratio.h"

// ROOT
#include <fairlogger/Logger.h>
#include <TH1.h>
#include <TH2.h>
#include <TList.h>
#include <TLine.h>
#include <TMath.h>
#include <TPaveText.h>
#include <iostream>
#include <fstream>
#include <string>

using namespace std;
using namespace o2::quality_control_modules::muon;

namespace o2::quality_control_modules::muonchambers
{

PhysicsPreclustersCheck::PhysicsPreclustersCheck() : mMinPseudoeff(0.5), mMaxPseudoeff(1.0)
{
  mElec2DetMapper = o2::mch::raw::createElec2DetMapper<o2::mch::raw::ElectronicMapperGenerated>();
  mDet2ElecMapper = o2::mch::raw::createDet2ElecMapper<o2::mch::raw::ElectronicMapperGenerated>();
  mFeeLink2SolarMapper = o2::mch::raw::createFeeLink2SolarMapper<o2::mch::raw::ElectronicMapperGenerated>();
  mSolar2FeeLinkMapper = o2::mch::raw::createSolar2FeeLinkMapper<o2::mch::raw::ElectronicMapperGenerated>();

  mDePseudoeff[0].resize(getDEindexMax() + 1);
  mDePseudoeff[1].resize(getDEindexMax() + 1);
}

PhysicsPreclustersCheck::~PhysicsPreclustersCheck() {}

void PhysicsPreclustersCheck::configure()
{
  if (auto param = mCustomParameters.find("MinPseudoefficiency"); param != mCustomParameters.end()) {
    mMinPseudoeff = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("MaxPseudoefficiency"); param != mCustomParameters.end()) {
    mMaxPseudoeff = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("MinGoodFraction"); param != mCustomParameters.end()) {
    mMinGoodFraction = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("PseudoeffPlotScaleMin"); param != mCustomParameters.end()) {
    mPseudoeffPlotScaleMin = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("PseudoeffPlotScaleMax"); param != mCustomParameters.end()) {
    mPseudoeffPlotScaleMax = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("Verbose"); param != mCustomParameters.end()) {
    if (param->second == "true" || param->second == "True" || param->second == "TRUE") {
      mVerbose = true;
    }
  }
}

int PhysicsPreclustersCheck::checkPadMapping(uint16_t feeId, uint8_t linkId, uint8_t eLinkId, o2::mch::raw::DualSampaChannelId channel, int& cathode)
{
  uint16_t solarId = -1;
  int deId = -1;
  int dsIddet = -1;
  int padId = -1;
  cathode = -1;

  o2::mch::raw::FeeLinkId feeLinkId{ feeId, linkId };

  if (auto opt = mFeeLink2SolarMapper(feeLinkId); opt.has_value()) {
    solarId = opt.value();
  }
  if (solarId < 0 || solarId > 1023) {
    return -1;
  }

  o2::mch::raw::DsElecId dsElecId{ solarId, static_cast<uint8_t>(eLinkId / 5), static_cast<uint8_t>(eLinkId % 5) };

  if (auto opt = mElec2DetMapper(dsElecId); opt.has_value()) {
    o2::mch::raw::DsDetId dsDetId = opt.value();
    dsIddet = dsDetId.dsId();
    deId = dsDetId.deId();
  }

  if (deId < 0 || dsIddet < 0) {
    return -1;
  }

  const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(deId);
  padId = segment.findPadByFEE(dsIddet, int(channel));

  if (padId < 0) {
    return -1;
  }

  cathode = segment.isBendingPad(padId) ? 0 : 1;

  return deId;
}

Quality PhysicsPreclustersCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;
    if (mo->getName().find("Pseudoeff_Elec") != std::string::npos) {

      // cumulative numerators and denominators for the computation of
      // the average pseudoeff over one detection element
      std::vector<double> dePseudoeffNum[2];
      std::vector<double> dePseudoeffDen[2];
      for (int i = 0; i < 2; i++) {
        dePseudoeffNum[i].resize(getDEindexMax() + 1);
        dePseudoeffDen[i].resize(getDEindexMax() + 1);
        std::fill(dePseudoeffNum[i].begin(), dePseudoeffNum[i].end(), 0);
        std::fill(dePseudoeffDen[i].begin(), dePseudoeffDen[i].end(), 0);
      }

      auto* hr = dynamic_cast<MergeableTH2Ratio*>(mo->getObject());
      if (!hr) {
        return result;
      }
      auto* h = dynamic_cast<TH2F*>(mo->getObject());
      if (!h) {
        return result;
      }

      if (h->GetEntries() == 0) {
        result = Quality::Medium;
      } else {
        int nbinsx = h->GetXaxis()->GetNbins();
        int nbinsy = h->GetYaxis()->GetNbins();
        int ngood = 0;
        int npads = 0;
        for (int i = 1; i <= nbinsx; i++) {
          int index = i - 1;
          int ds_addr = (index % 40);
          int link_id = (index / 40) % 12;
          int fee_id = index / (12 * 40);

          for (int j = 1; j <= nbinsy; j++) {
            int chan_addr = j - 1;

            int cathode;
            int de = checkPadMapping(fee_id, link_id, ds_addr, chan_addr, cathode);
            if (de < 0) {
              continue;
            }

            Float_t stat = hr->getDen()->GetBinContent(i, j);
            if (stat < 1) {
              continue;
            }

            Float_t pseudoeff = h->GetBinContent(i, j);
            npads += 1;
            if (pseudoeff >= mMinPseudoeff && pseudoeff <= mMaxPseudoeff) {
              ngood += 1;
            }

            int deIndex = getDEindex(de);
            if (deIndex >= 0) {
              dePseudoeffNum[cathode][deIndex] += pseudoeff;
              dePseudoeffDen[cathode][deIndex] += 1;
            }
          }
        }
        if (mVerbose) {
          LOGP(debug, "Npads {}  Ngood {}   Frac {}", npads, ngood, float(ngood) / float(npads));
        }

        if (ngood >= mMinGoodFraction * npads)
          result = Quality::Good;
        else
          result = Quality::Bad;
      }

      // update the average pseudoeff values that will be copied
      // into the histogram bins in the beutify() method
      for (int i = 0; i < 2; i++) {
        for (size_t de = 0; de < dePseudoeffDen[i].size(); de++) {
          if (dePseudoeffDen[i][de] > 0) {
            mDePseudoeff[i][de] = dePseudoeffNum[i][de] / dePseudoeffDen[i][de];
          } else {
            mDePseudoeff[i][de] = 0;
          }
        }
      }
    }
  }

  return result;
}

std::string PhysicsPreclustersCheck::getAcceptedType() { return "TH1"; }

static void updateTitle(TH1* hist, std::string suffix)
{
  if (!hist) {
    return;
  }
  TString title = hist->GetTitle();
  title.Append(" ");
  title.Append(suffix.c_str());
  hist->SetTitle(title);
}

static std::string getCurrentTime()
{
  time_t t;
  time(&t);

  struct tm* tmp;
  tmp = localtime(&t);

  char timestr[500];
  strftime(timestr, sizeof(timestr), "(%x - %X)", tmp);

  std::string result = timestr;
  return result;
}

void PhysicsPreclustersCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  auto currentTime = getCurrentTime();
  updateTitle(dynamic_cast<TH1*>(mo->getObject()), currentTime);

  if ((mo->getName().find("MeanPseudoeffPerDE_B") != std::string::npos) ||
      (mo->getName().find("MeanPseudoeffPerDE_NB") != std::string::npos) ||
      (mo->getName().find("PreclustersPerDE") != std::string::npos) ||
      (mo->getName().find("PreclustersSignalPerDE") != std::string::npos)) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    // disable ticks on axis
    h->GetXaxis()->SetTickLength(0.0);
    h->GetXaxis()->SetLabelSize(0.0);
    h->GetYaxis()->SetTickLength(0);
    h->GetYaxis()->SetTitle("efficiency");

    TText* xtitle = new TText();
    xtitle->SetNDC();
    xtitle->SetText(0.87, 0.03, "chamber #");
    xtitle->SetTextSize(15);
    h->GetListOfFunctions()->Add(xtitle);

    h->SetMinimum(0);
    if ((mo->getName().find("MeanPseudoeffPerDE_B") != std::string::npos) ||
        (mo->getName().find("MeanPseudoeffPerDE_NB") != std::string::npos)) {
      int cathode = 0;
      if (mo->getName().find("MeanPseudoeffPerDE_NB") != std::string::npos) {
        cathode = 1;
      }

      for (size_t deIndex = 0; deIndex < mDePseudoeff[cathode].size(); deIndex++) {
        h->SetBinContent(deIndex + 1, mDePseudoeff[cathode][deIndex]);
        h->SetBinError(deIndex + 1, 0);
      }

      h->SetMaximum(2);
    }

    // draw chamber delimiters
    for (int demin = 200; demin <= 1000; demin += 100) {
      float xpos = static_cast<float>(getDEindex(demin));
      TLine* delimiter = new TLine(xpos, 0, xpos, h->GetMaximum());
      delimiter->SetLineColor(kBlack);
      delimiter->SetLineStyle(kDashed);
      h->GetListOfFunctions()->Add(delimiter);
    }

    // draw x-axis labels
    for (int ch = 1; ch <= 10; ch += 1) {
      float x1 = static_cast<float>(getDEindex(ch * 100));
      float x2 = (ch < 10) ? static_cast<float>(getDEindex(ch * 100 + 100)) : h->GetXaxis()->GetXmax();
      float x0 = 0.8 * (x1 + x2) / (2 * h->GetXaxis()->GetXmax()) + 0.1;
      float y0 = 0.05;
      TText* label = new TText();
      label->SetNDC();
      label->SetText(x0, y0, TString::Format("%d", ch));
      label->SetTextSize(15);
      label->SetTextAlign(22);
      h->GetListOfFunctions()->Add(label);
    }

    TPaveText* msg = new TPaveText(0.3, 0.9, 0.7, 0.95, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));

    if (checkResult == Quality::Good) {
      msg->Clear();
      msg->AddText("Pseudo-efficiency consistently within limits: OK!!!");
      msg->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      ILOG(Debug, Devel) << "Quality::Bad, setting to red";
      msg->Clear();
      msg->AddText("Call MCH on-call.");
      msg->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      ILOG(Debug, Devel) << "Quality::medium, setting to orange";
      msg->Clear();
      msg->AddText("No entries. If MCH in the run, check MCH TWiki");
      msg->SetFillColor(kYellow);
    }
    h->SetLineColor(kBlack);
  }

  if ((mo->getName().find("Pseudoeff_ST12") != std::string::npos) ||
      (mo->getName().find("Pseudoeff_ST345") != std::string::npos) ||
      (mo->getName().find("Pseudoeff_B_XY") != std::string::npos) ||
      (mo->getName().find("Pseudoeff_NB_XY") != std::string::npos)) {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    h->SetMinimum(0);
    h->SetMaximum(1);
    h->GetXaxis()->SetTickLength(0.0);
    h->GetXaxis()->SetLabelSize(0.0);
    h->GetYaxis()->SetTickLength(0.0);
    h->GetYaxis()->SetLabelSize(0.0);
  }
}

} // namespace o2::quality_control_modules::muonchambers
