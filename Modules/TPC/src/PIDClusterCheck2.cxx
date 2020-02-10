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
/// \file   PIDClusterCheck.cxx
/// \author Laura Serksnyte
///
#include <iostream>



#include "TPC/PIDClusterCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

#include <fairlogger/Logger.h>
// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TLatex.h>

using namespace std;

namespace o2::quality_control_modules::tpc
{

void PIDClusterCheck::configure(std::string) {}

Quality PIDClusterCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;



  
  for (auto& [moName, mo] : *moMap) {

    std::cout<<"The name of the passed object: "<<mo->getName()<<std::endl;
    (void)moName;
    //result = Quality::Good;
    //beautify(mo, result);
    //result = Quality::Medium;
    Quality result = Quality::Null;

    if (mo->getName() == "hSnp") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());

      result = Quality::Good;

      for (int i = 0; i < h->GetNbinsX(); i++) {
        if ((h->GetBinContent(i)>0) && (h->GetBinCenter(i) <0 || h->GetBinCenter(i) >1) ) {
          result = Quality::Bad;
          break;
        } 
      }
    }

    if (mo->getName() == "hNClusters") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());

      result = Quality::Good;

      for (int i = 0; i < h->GetNbinsX(); i++) {
        if (h->GetBinContent(i) < 10 ) {
          result = Quality::Medium;
        } 
      }
    }

    if (mo->getName() == "hPhi") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      result = Quality::Good;
    }


    if (mo->getName() == "hdEdxVsPhi") {
      auto* h = dynamic_cast<TH2F*>(mo->getObject());
      result = Quality::Good;
    }
    if (mo->getName() == "hdEdxVsTgl") {
      auto* h = dynamic_cast<TH2F*>(mo->getObject());
      result = Quality::Medium;
    }
    if (mo->getName() == "hdEdxVsp") {
      auto* h = dynamic_cast<TH2F*>(mo->getObject());
      result = Quality::Bad;
      break;
    }
    beautify(mo, result);
    
  }
  return result;
}

std::string PIDClusterCheck::getAcceptedType() { return "TObject"; }

void PIDClusterCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  std::cout<<"BEAUTIFY FUNCTION OUTPUT! The Quality passed: "<<checkResult<<std::endl;
  auto* h = dynamic_cast<TH1F*>(mo->getObject());
  if (h){
    if (checkResult == Quality::Good) {
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      LOG(INFO) << "Quality::Bad, setting to red";
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      LOG(INFO) << "Quality::medium, setting to orange";
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
  auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
  if (h2){
    if (checkResult == Quality::Good) {
      h2->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      LOG(INFO) << "Quality::Bad, setting to red";
      h2->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      LOG(INFO) << "Quality::medium, setting to orange";
      h2->SetFillColor(kOrange);
    }
  }
  
}

} // namespace o2::quality_control_modules::tpc
