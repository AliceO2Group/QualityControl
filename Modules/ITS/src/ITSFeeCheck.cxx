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
///

#include "ITS/ITSFeeCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

#include <fairlogger/Logger.h>
#include <TList.h>
#include <TH2.h>
#include <TText.h>
#include <iostream> 

namespace o2::quality_control_modules::its
{

void ITSFeeCheck::configure() {}

Quality ITSFeeCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = 0;                                                                                                      //Fee Checker will check six plot in Fee Task, 
  bool badStaveCount, badStaveIB, badStaveML, badStaveOL;                                                                  // and will store the quality result in a six digits number:
  std::map<std::string, std::shared_ptr<MonitorObject>>::iterator iter;                                                    //   XXXXXX
  for (iter = moMap->begin(); iter != moMap->end(); ++iter) {                                                              //   |||||| -> laneStatusOverviewFlagFAULT
    for (int iflag = 0; iflag < NFlags; iflag++) {                                                                         //   |||||  -> laneStatusFlagFAULT
      if (iter->second->getName() == Form("LaneStatus/laneStatusFlag%s", mLaneStatusFlag[iflag].c_str())) {                //   ||||   -> laneStatusOverviewFlagERROR
        auto* h = dynamic_cast<TH2I*>(iter->second->getObject());                                                          //   |||    -> laneStatusFlagERROR
        if (h->GetMaximum() > 0) {                                                                                         //   ||     -> laneStatusOverviewFlagWARNING
          result = result.getLevel() + 1*pow(10,iflag*2);                                                                  //   |      -> laneStatusFlagWARNING
        }                                                                                                                  //  
      }                                                                                                                    //  the number for each digit correspond to
      if (iter->second->getName() == Form("LaneStatus/laneStatusOverviewFlag%s", mLaneStatusFlag[iflag].c_str())) {        //  0: Good, 1: Bad
        auto* hp = dynamic_cast<TH2Poly*>(iter->second->getObject());
        badStaveIB = false; badStaveML = false; badStaveOL=false;
        for (int ilayer=0; ilayer< NLayer; ilayer++){
          int countStave = 0; badStaveCount = false;
          for (int ibin = StaveBoundary[ilayer]; ibin < StaveBoundary[ilayer+1]; ++ibin) {
            if(hp->GetBinContent(ibin) > 0.) {
              countStave++;
            }
            if(ibin < StaveBoundary[3]){
              //Check if there are staves in the IB with lane in Bad (bin are filled with %)
              if (hp->GetBinContent(ibin) > 0.) badStaveIB=true;
            }
            else if(ibin < StaveBoundary[5]){
              //Check if there are staves in the MLs with at least 4 lanes in Bad (bin are filled with %)
              if (hp->GetBinContent(ibin) > 4./NLanePerStaveLayer[ilayer]) badStaveML=true;
            }
            else if(ibin < StaveBoundary[7]){
              //Check if there are staves in the OLs with at least 7 lanes in Bad (bin are filled with %)
              if (hp->GetBinContent(ibin) > 7./NLanePerStaveLayer[ilayer]) badStaveOL=true;
            }
          }
          //Check if there are more thhan 25% staves in Bad per layer
          if(countStave > 0.25*NStaves[ilayer]){
            badStaveCount = true;
          }
        } //end loop over layers
        if(badStaveIB || badStaveML || badStaveOL || badStaveCount){
          result = result.getLevel() + 1*pow(10,iflag*2+1);
        }
      } //end lanestatusOverview
    } //end flag loop
  } //end mop
  return result;
}//end check

std::string ITSFeeCheck::getAcceptedType() { return "TH2I"; }

void ITSFeeCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  auto* tInfo = new TText();
  for (int iflag = 0; iflag < NFlags; iflag++) {
    if (mo->getName() == Form("LaneStatus/laneStatusFlag%s", mLaneStatusFlag[iflag].c_str())) {
      auto* h = dynamic_cast<TH2I*>(mo->getObject());
      if ((int)(checkResult.getLevel() % (int)pow(10, 1+iflag*2)) / (int)pow(10, iflag*2)== 0) {
        tInfo->SetText(0.1, 0.8, "Quality::GOOD");
        tInfo->SetTextColor(kGreen);
      } else if ((int)(checkResult.getLevel() % (int)pow(10, 1+iflag*2)) / (int)pow(10, iflag*2)== 1) {
        tInfo->SetText(0.1, 0.8, "Quality::BAD(call expert)");
        tInfo->SetTextColor(kRed);
      }
      tInfo->SetTextSize(17);
      tInfo->SetNDC();
      h->GetListOfFunctions()->Add(tInfo);
    }
    if (mo->getName() == Form("LaneStatus/laneStatusOverviewFlag%s", mLaneStatusFlag[iflag].c_str())) {
      auto* hp = dynamic_cast<TH2Poly*>(mo->getObject());
      if ((int)(checkResult.getLevel() % (int)pow(10, 2+iflag*2) / (int)pow(10, 1+iflag*2) ) == 0) {
        tInfo->SetText(0.13, 0.87, "Quality::GOOD");
        tInfo->SetTextColor(kGreen);
      }
      else if ((int)(checkResult.getLevel() % (int)pow(10, 2+iflag*2) / (int)pow(10, 1+iflag*2) ) == 1) {
        tInfo->SetText(0.13, 0.87, "Quality::BAD - Call the on-call");
        tInfo->SetTextColor(kRed);
      }
      tInfo->SetTextSize(18);
      tInfo->SetNDC();
      hp->GetListOfFunctions()->Add(tInfo);
    }//end laneStatusOverviewFlag%s
  }//end flags
}

} // namespace o2::quality_control_modules::its
