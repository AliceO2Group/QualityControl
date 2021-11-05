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
/// \file   ITSClusterCheck.cxx
/// \author Artem Isakov
/// \author LiAng Zhang
/// \author Jian Liu
///

#include "ITS/ITSClusterCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

#include <fairlogger/Logger.h>
#include <TList.h>
#include <TH2.h>
#include <string.h>
#include <TPaveText.h>
#include <iostream>

namespace o2::quality_control_modules::its
{

void ITSClusterCheck::configure(std::string) {}

Quality ITSClusterCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = 0;
  std::map<std::string, std::shared_ptr<MonitorObject>>::iterator iter;
  for (iter = moMap->begin(); iter != moMap->end(); ++iter) {

    if (iter->second->getName().find("AverageClusterSize") != std::string::npos) {
      auto* h = dynamic_cast<TH2D*>(iter->second->getObject());

      if (iter->second->getName().find("Layer0") != std::string::npos && h->GetMaximum() > 20) result = result.getLevel()^(1<<0);
      if (iter->second->getName().find("Layer1") != std::string::npos && h->GetMaximum() > 20) result = result.getLevel()^(1<<1);
      if (iter->second->getName().find("Layer2") != std::string::npos && h->GetMaximum() > 20) result = result.getLevel()^(1<<2);
      if (iter->second->getName().find("Layer3") != std::string::npos && h->GetMaximum() > 20) result = result.getLevel()^(1<<3);
      if (iter->second->getName().find("Layer4") != std::string::npos && h->GetMaximum() > 20) result = result.getLevel()^(1<<4);
      if (iter->second->getName().find("Layer5") != std::string::npos && h->GetMaximum() > 20) result = result.getLevel()^(1<<5);
      if (iter->second->getName().find("Layer6") != std::string::npos && h->GetMaximum() > 20) result = result.getLevel()^(1<<6); 

    }

    if (iter->second->getName().find("ClusterOccupation") != std::string::npos) {
      auto* h = dynamic_cast<TH2D*>(iter->second->getObject());

      if (iter->second->getName().find("Layer0") != std::string::npos && h->GetMaximum() > 40) result = result.getLevel()^(1<<7);
      if (iter->second->getName().find("Layer1") != std::string::npos && h->GetMaximum() > 30) result = result.getLevel()^(1<<8);
      if (iter->second->getName().find("Layer2") != std::string::npos && h->GetMaximum() > 20) result = result.getLevel()^(1<<9);
      if (iter->second->getName().find("Layer3") != std::string::npos && h->GetMaximum() > 30) result = result.getLevel()^(1<<10);
      if (iter->second->getName().find("Layer4") != std::string::npos && h->GetMaximum() > 25) result = result.getLevel()^(1<<11);
      if (iter->second->getName().find("Layer5") != std::string::npos && h->GetMaximum() > 15) result = result.getLevel()^(1<<12);
      if (iter->second->getName().find("Layer6") != std::string::npos && h->GetMaximum() > 14) result = result.getLevel()^(1<<13);
    }
  }
  return result;
}

std::string ITSClusterCheck::getAcceptedType() { return "TH2D"; }

void ITSClusterCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{

  std::cout<<"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! checkResults" << checkResult <<std::endl;

  if (mo->getName().find("AverageClusterSize") != std::string::npos) {
    auto* h = dynamic_cast<TH2D*>(mo->getObject());
    

    std::cout<<"-------------- Quality of " << mo->getName();



    std::string histoName = mo->getName();
    //int iLayer = std::stoi(histoName[histoName.find("Layer")+5]);  //5 is length of "Layer" next symbol is Layer number
     int iLayer = histoName[histoName.find("Layer")+5]-48;
   // std::cout<<histoName << " Layer is : "<< iLayer<<std::endl;

    
    auto* msg = new TPaveText(0.15, 0.7, 0.6, 0.85, "NDC");
    msg->SetName(Form("%s_msg", mo->GetName()));
    msg->Clear();


    if ( (int)(checkResult.getLevel()&(1<<iLayer)) == 0){

       std::cout<<" is GOOD: "<< checkResult.getLevel() << " checkResult.getLevel()&(1<<iLayer) " <<  (int) ( checkResult.getLevel()&(1<<iLayer)) <<std::endl;	    
       msg->AddText("Quality::GOOD");
       msg->SetTextColor(kGreen);
    } else {
       
       std::cout<<" is BAD: "<< checkResult.getLevel() << " checkResult.getLevel()&(1<<iLayer) " << (int) (checkResult.getLevel()&(1<<iLayer)) << std::endl;	    
       msg->AddText("INFO: large clusters - do not call expert");
       msg->SetTextColor(kRed);
    }
    
    msg->SetTextSize(17);
    msg->SetBorderSize(0);
    h->GetListOfFunctions()->Add(msg);
  }


  




  if (mo->getName().find("ClusterOccupation") != std::string::npos) {
    auto* h = dynamic_cast<TH2D*>(mo->getObject());

    std::string histoName = mo->getName();
    //int iLayer = std::stoi(histoName[histoName.find("Layer")+5]);  //5 is length of "Layer" next symbol is Layer number
    int iLayer = histoName[histoName.find("Layer")+5]-48;
    std::cout<<histoName << " Layer is : "<< iLayer<<std::endl;
    auto* msg = new TPaveText(0.15, 0.7, 0.6, 0.85, "NDC");
    msg->SetName(Form("%s_msg", mo->GetName()));
    msg->Clear();

    
    if ( (int) (checkResult.getLevel()&(1<<(7+iLayer))) == 0){
       msg->AddText("Quality::GOOD");
       msg->SetTextColor(kGreen);
    } else {
       msg->AddText("INFO: large cluster occupancy, call expert");
       msg->SetTextColor(kRed);
    }
    
    msg->SetTextSize(17);
    msg->SetBorderSize(0);
    h->GetListOfFunctions()->Add(msg);
  }







}


} // namespace o2::quality_control_modules::its
