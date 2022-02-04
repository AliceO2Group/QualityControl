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

#include "MCHMappingInterface/Segmentation.h"
#include "MCHMappingSegContour/CathodeSegmentationContours.h"
#include "MCH/PhysicsPreclustersCheck.h"

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

namespace o2::quality_control_modules::muonchambers
{

PhysicsPreclustersCheck::PhysicsPreclustersCheck()
{
  mPrintLevel = 1;
  minPseudoeff = 0.5;
  maxPseudoeff = 1.0;
  minMPV = 300;
  maxMPV = 700;
}

PhysicsPreclustersCheck::~PhysicsPreclustersCheck() {}

void PhysicsPreclustersCheck::configure()
{
  //   if (AliRecoParam::ConvertIndex(specie) == AliRecoParam::kCosmic) {
  //     minTOFrawTime = 150.; //ns
  //     maxTOFrawTime = 250.; //ns
  //   }
}

Quality PhysicsPreclustersCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  //  std::cout << "=================================" << std::endl;
  //  std::cout << "PhysicsOccupancyCheck::check() called" << std::endl;
  //  std::cout << "=================================" << std::endl;
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;
    //   std::cout << mo->getName() <<endl;
    if (mo->getName().find("QcMuonChambers_MeanPseudoeff_Mergeable_DoesGoodBHaveSomethingNB") != std::string::npos) {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      std::cout << mo->ClassName() << endl;
      if (!h) {
        std::cout << "Pas trouvé l'objet" << std::endl;
        return result;
      }

      if (h->GetEntries() == 0) {
        result = Quality::Medium;
      } else {
        int nbinsx = h->GetXaxis()->GetNbins();
        int nbad = 0;
        for (int i = 1; i <= nbinsx; i++) {
          Float_t occ = h->GetBinContent(i);
          if (occ < minPseudoeff || occ > maxPseudoeff) {
            nbad += 1;
          }
        }
        if (nbad < 1) {
          result = Quality::Good;
          std::cout << "GOOD" << endl;
        } else {
          result = Quality::Bad;
          std::cout << "BAD" << endl;
        }
      }
    }
  }
  for (auto& [moName, mo] : *moMap) {
    if (mo->getName().find("QcMuonChambers_MeanPseudoeff_Mergeable_DoesGoodNBHaveSomethingB") != std::string::npos) {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      std::cout << mo->ClassName() << endl;
      if (!h) {
        std::cout << "Pas trouvé l'objet" << std::endl;
        return result;
      }

      if (h->GetEntries() == 0) {
        result = Quality::Medium;
      } else {
        int nbinsx = h->GetXaxis()->GetNbins();
        int nbad = 0;
        for (int i = 1; i <= nbinsx; i++) {
          Float_t occ = h->GetBinContent(i);
          if (occ < minPseudoeff || occ > maxPseudoeff) {
            nbad += 1;
          }
        }
        if (nbad < 1) {
          result = Quality::Good;
          std::cout << "GOOD" << endl;
        } else {
          result = Quality::Bad;
          std::cout << "BAD" << endl;
        }
      }
    }
  }

  for (auto& [moName, mo] : *moMap) {

    (void)moName;
    //  std::cout << mo->getName() <<endl;
    if (mo->getName().find("mMeanPseudoeffPerDE_DoesGoodBHaveSomethingNB_OnCycle") != std::string::npos) {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      std::cout << mo->ClassName() << endl;
      if (!h) {
        std::cout << "Pas trouvé l'objet" << std::endl;
        return result;
      }

      if (h->GetEntries() == 0) {
        result = Quality::Medium;
      } else {
        int nbinsx = h->GetXaxis()->GetNbins();
        int nbad = 0;
        for (int i = 1; i <= nbinsx; i++) {
          Float_t occ = h->GetBinContent(i);
          if (occ < minPseudoeff || occ > maxPseudoeff) {
            nbad += 1;
          }
        }
        if (nbad < 1) {
          result = Quality::Good;
          std::cout << "GOOD" << endl;
        } else {
          result = Quality::Bad;
          std::cout << "BAD" << endl;
        }
      }
    }
  }

  for (auto& [moName, mo] : *moMap) {

    (void)moName;
    //  std::cout << mo->getName() <<endl;
    if (mo->getName().find("mMeanPseudoeffPerDE_DoesGoodNBHaveSomethingB_OnCycle") != std::string::npos) {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      std::cout << mo->ClassName() << endl;
      if (!h) {
        std::cout << "Pas trouvé l'objet" << std::endl;
        return result;
      }

      if (h->GetEntries() == 0) {
        result = Quality::Medium;
      } else {
        int nbinsx = h->GetXaxis()->GetNbins();
        int nbad = 0;
        for (int i = 1; i <= nbinsx; i++) {
          Float_t occ = h->GetBinContent(i);
          if (occ < minPseudoeff || occ > maxPseudoeff) {
            nbad += 1;
          }
        }
        if (nbad < 1) {
          result = Quality::Good;
          std::cout << "GOOD" << endl;
        } else {
          result = Quality::Bad;
          std::cout << "BAD" << endl;
        }
      }
    }
  }

  for (auto& [moName, mo] : *moMap) {

    (void)moName;
    //     std::cout << mo->getName() <<endl;
    if (mo->getName().find("QcMuonChambers_MPV_Mergeable_OnCycle") != std::string::npos) {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      std::cout << mo->ClassName() << endl;
      if (!h) {
        std::cout << "Pas trouvé l'objet" << std::endl;
        return result;
      }

      if (h->GetEntries() == 0) {
        result = Quality::Medium;
      } else {
        int nbinsx = h->GetXaxis()->GetNbins();
        int nbad = 0;
        for (int i = 1; i <= nbinsx; i++) {
          Float_t occ = h->GetBinContent(i);
          if (occ < minMPV || occ > maxMPV) {
            nbad += 1;
          }
        }
        if (nbad < 1) {
          result = Quality::Good;
          std::cout << "GOOD" << endl;
        } else {
          result = Quality::Bad;
          std::cout << "BAD" << endl;
        }
      }
    }
  }

  return result;
}

std::string PhysicsPreclustersCheck::getAcceptedType() { return "TH1"; }

void PhysicsPreclustersCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  //  std::cout<<"===================================="<<std::endl;
  //  std::cout<<"PhysicsOccupancyCheck::beautify() called"<<std::endl;
  //  std::cout<<"===================================="<<std::endl;
  if (mo->getName().find("QcMuonChambers_MeanPseudoeff_Mergeable_DoesGoodBHaveSomethingNB") != std::string::npos) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.3, 0.9, 0.7, 0.95, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));

    TLine* lmin = new TLine(0, minPseudoeff, 1100, minPseudoeff);
    TLine* lmax = new TLine(0, maxPseudoeff, 1100, maxPseudoeff);

    h->GetListOfFunctions()->Add(lmin);
    h->GetListOfFunctions()->Add(lmax);

    //      lmin->Draw();
    //      lmax->Draw();
    if (checkResult == Quality::Good) {
      msg->Clear();
      msg->AddText("Pseudo-efficiency consistently within limits: OK!!!");
      msg->SetFillColor(kGreen);
      //
      //   h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      LOG(info) << "Quality::Bad, setting to red";
      //
      msg->Clear();
      msg->AddText("Call MCH on-call.");
      msg->SetFillColor(kRed);
      //
      //  h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      LOG(info) << "Quality::medium, setting to orange";
      //
      msg->Clear();
      msg->AddText("No entries. If MCH in the run, check MCH TWiki");
      msg->SetFillColor(kYellow);
      //   h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);

    for (Int_t i = 1; i <= h->GetNbinsX(); i += 1) {
      TBox* b = new TBox(h->GetBinLowEdge(i),
                         h->GetMinimum(),
                         h->GetBinWidth(i) + h->GetBinLowEdge(i),
                         h->GetBinContent(i));
      b->SetFillColor(kGreen);
      if (h->GetBinContent(i) < minPseudoeff || h->GetBinContent(i) > maxPseudoeff) {
        b->SetFillColor(kRed);
      }
      h->GetListOfFunctions()->Add(b);
    }
  }

  if (mo->getName().find("QcMuonChambers_MeanPseudoeff_Mergeable_DoesGoodNBHaveSomethingB") != std::string::npos) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.3, 0.9, 0.7, 0.95, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));

    TLine* lmin = new TLine(0, minPseudoeff, 1100, minPseudoeff);
    TLine* lmax = new TLine(0, maxPseudoeff, 1100, maxPseudoeff);

    h->GetListOfFunctions()->Add(lmin);
    h->GetListOfFunctions()->Add(lmax);

    //      lmin->Draw();
    //      lmax->Draw();
    if (checkResult == Quality::Good) {
      msg->Clear();
      msg->AddText("Pseudo-efficiency consistently within limits: OK!!!");
      msg->SetFillColor(kGreen);
      //
      //   h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      LOG(info) << "Quality::Bad, setting to red";
      //
      msg->Clear();
      msg->AddText("Call MCH on-call.");
      msg->SetFillColor(kRed);
      //
      //  h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      LOG(info) << "Quality::medium, setting to orange";
      //
      msg->Clear();
      msg->AddText("No entries. If MCH in the run, check MCH TWiki");
      msg->SetFillColor(kYellow);
      //   h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);

    for (Int_t i = 1; i <= h->GetNbinsX(); i += 1) {
      TBox* b = new TBox(h->GetBinLowEdge(i),
                         h->GetMinimum(),
                         h->GetBinWidth(i) + h->GetBinLowEdge(i),
                         h->GetBinContent(i));
      b->SetFillColor(kGreen);
      if (h->GetBinContent(i) < minPseudoeff || h->GetBinContent(i) > maxPseudoeff) {
        b->SetFillColor(kRed);
      }
      h->GetListOfFunctions()->Add(b);
    }
  }

  if (mo->getName().find("mMeanPseudoeffPerDE_DoesGoodBHaveSomethingNB_OnCycle") != std::string::npos) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.3, 0.9, 0.7, 0.95, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));

    TLine* lmin = new TLine(0, minPseudoeff, 1100, minPseudoeff);
    TLine* lmax = new TLine(0, maxPseudoeff, 1100, maxPseudoeff);

    h->GetListOfFunctions()->Add(lmin);
    h->GetListOfFunctions()->Add(lmax);

    //      lmin->Draw();
    //      lmax->Draw();
    if (checkResult == Quality::Good) {
      msg->Clear();
      msg->AddText("Pseudo-efficiency consistently within limits: OK!!!");
      msg->SetFillColor(kGreen);
      //
      //   h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      LOG(info) << "Quality::Bad, setting to red";
      //
      msg->Clear();
      msg->AddText("Call MCH on-call.");
      msg->SetFillColor(kRed);
      //
      //  h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      LOG(info) << "Quality::medium, setting to orange";
      //
      msg->Clear();
      msg->AddText("No entries. If MCH in the run, check MCH TWiki");
      msg->SetFillColor(kYellow);
      //   h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);

    for (Int_t i = 1; i <= h->GetNbinsX(); i += 1) {
      TBox* b = new TBox(h->GetBinLowEdge(i),
                         h->GetMinimum(),
                         h->GetBinWidth(i) + h->GetBinLowEdge(i),
                         h->GetBinContent(i));
      b->SetFillColor(kGreen);
      if (h->GetBinContent(i) < minPseudoeff || h->GetBinContent(i) > maxPseudoeff) {
        b->SetFillColor(kRed);
      }
      h->GetListOfFunctions()->Add(b);
    }
  }

  if (mo->getName().find("mMeanPseudoeffPerDE_DoesGoodNBHaveSomethingB_OnCycle") != std::string::npos) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.3, 0.9, 0.7, 0.95, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));

    TLine* lmin = new TLine(0, minPseudoeff, 1100, minPseudoeff);
    TLine* lmax = new TLine(0, maxPseudoeff, 1100, maxPseudoeff);

    h->GetListOfFunctions()->Add(lmin);
    h->GetListOfFunctions()->Add(lmax);

    //      lmin->Draw();
    //      lmax->Draw();
    if (checkResult == Quality::Good) {
      msg->Clear();
      msg->AddText("Pseudo-efficiency consistently within limits: OK!!!");
      msg->SetFillColor(kGreen);
      //
      //   h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      LOG(info) << "Quality::Bad, setting to red";
      //
      msg->Clear();
      msg->AddText("Call MCH on-call.");
      msg->SetFillColor(kRed);
      //
      //  h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      LOG(info) << "Quality::medium, setting to orange";
      //
      msg->Clear();
      msg->AddText("No entries. If MCH in the run, check MCH TWiki");
      msg->SetFillColor(kYellow);
      //   h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);

    for (Int_t i = 1; i <= h->GetNbinsX(); i += 1) {
      TBox* b = new TBox(h->GetBinLowEdge(i),
                         h->GetMinimum(),
                         h->GetBinWidth(i) + h->GetBinLowEdge(i),
                         h->GetBinContent(i));
      b->SetFillColor(kGreen);
      if (h->GetBinContent(i) < minPseudoeff || h->GetBinContent(i) > maxPseudoeff) {
        b->SetFillColor(kRed);
      }
      h->GetListOfFunctions()->Add(b);
    }
  }

  if (mo->getName().find("QcMuonChambers_MPV_Mergeable_OnCycle") != std::string::npos) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.3, 0.9, 0.7, 0.95, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));

    TLine* lmin = new TLine(0, minMPV, 1100, minMPV);
    TLine* lmax = new TLine(0, maxMPV, 1100, maxMPV);

    h->GetListOfFunctions()->Add(lmin);
    h->GetListOfFunctions()->Add(lmax);

    //      lmin->Draw();
    //      lmax->Draw();
    if (checkResult == Quality::Good) {
      msg->Clear();
      msg->AddText("MPV consistently within limits: OK!!!");
      msg->SetFillColor(kGreen);
      //
      //   h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      LOG(info) << "Quality::Bad, setting to red";
      //
      msg->Clear();
      msg->AddText("Call MCH on-call.");
      msg->SetFillColor(kRed);
      //
      //  h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      LOG(info) << "Quality::medium, setting to orange";
      //
      msg->Clear();
      msg->AddText("No entries. If MCH in the run, check MCH TWiki");
      msg->SetFillColor(kYellow);
      //   h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);

    for (Int_t i = 1; i <= h->GetNbinsX(); i += 1) {
      TBox* b = new TBox(h->GetBinLowEdge(i),
                         h->GetMinimum(),
                         h->GetBinWidth(i) + h->GetBinLowEdge(i),
                         h->GetBinContent(i));
      b->SetFillColor(kGreen);
      if (h->GetBinContent(i) < minMPV || h->GetBinContent(i) >= maxMPV) {
        b->SetFillColor(kRed);
      }
      h->GetListOfFunctions()->Add(b);
    }
  }
}

} // namespace o2::quality_control_modules::muonchambers
