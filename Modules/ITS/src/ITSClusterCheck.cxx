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
#include <TText.h>
#include <string.h>


namespace o2::quality_control_modules::its
{

void ITSClusterCheck::configure(std::string) {}

Quality ITSClusterCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;
  std::map<std::string, std::shared_ptr<MonitorObject>>::iterator iter;
  for (iter = moMap->begin(); iter != moMap->end(); ++iter) {


   if (iter->second->getName().find("AverageClusterSize") != std::string::npos) {
      auto* h = dynamic_cast<TH2D*>(iter->second->getObject());
      if (h->GetMaximum() > 20) {
        result = Quality::Medium;
      } else {
        result = Quality::Good;
      }

    }

   if (iter->second->getName().find("ClusterOccupation") != std::string::npos) {
      auto* h = dynamic_cast<TH2D*>(iter->second->getObject());
      
       if ( iter->second->getName().find("Layer0") != std::string::npos && h->GetMaximum() > 40) result = Quality::Bad;
       else result = Quality::Good;

       if ( iter->second->getName().find("Layer1") != std::string::npos && h->GetMaximum() > 30) result = Quality::Bad;
       else result = Quality::Good;

       if ( iter->second->getName().find("Layer2") != std::string::npos && h->GetMaximum() > 20) result = Quality::Bad;
       else result = Quality::Good;

       if ( iter->second->getName().find("Layer3") != std::string::npos && h->GetMaximum() > 30) result = Quality::Bad;
       else result = Quality::Good;

       if ( iter->second->getName().find("Layer4") != std::string::npos && h->GetMaximum() > 25) result = Quality::Bad;
       else result = Quality::Good;

       if ( iter->second->getName().find("Layer5") != std::string::npos && h->GetMaximum() > 10) result = Quality::Bad;
       else result = Quality::Good;
      
    }





  }
  return result;
}

std::string ITSClusterCheck::getAcceptedType() { return "TH2D"; }

void ITSClusterCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{

   if (mo->getName().find("AverageClusterSize") != std::string::npos) {
      auto* h = dynamic_cast<TH2D*>(mo->getObject());
      auto* tInfo = new TText();

      if (checkResult == Quality::Medium) {
         tInfo->SetText(0.1, 0.8, "Info: large clusters - do not call expert");
         tInfo->SetTextColor(kOrange);
      }else if (checkResult == Quality::Good) {
        tInfo->SetText(0.1, 0.8, "Quality::GOOD");
        tInfo->SetTextColor(kGreen);
     } 

 
      tInfo->SetTextSize(17);
      tInfo->SetNDC();
      h->GetListOfFunctions()->Add(tInfo);
   }
   if (mo->getName().find("ClusterOccupation") != std::string::npos) {
      auto* h = dynamic_cast<TH2D*>(mo->getObject());
      auto* tInfo = new TText();

      if (checkResult == Quality::Bad) {
         tInfo->SetText(0.1, 0.8, "Info: large cluster occupancy, call expert");
         tInfo->SetTextColor(kRed);
      }else if (checkResult == Quality::Good) {
        tInfo->SetText(0.1, 0.8, "Quality::GOOD");
        tInfo->SetTextColor(kGreen);
     }
      tInfo->SetTextSize(17);
      tInfo->SetNDC();
      h->GetListOfFunctions()->Add(tInfo);
   }



}

} // namespace o2::quality_control_modules::its
