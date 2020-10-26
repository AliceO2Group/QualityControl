// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.



// Fair
#include <fairlogger/Logger.h>
// ROOT
#include "TH1.h"
#include "TTree.h"
// Quality Control
#include "FT0/ChannelsCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include "DataFormatsFT0/Digit.h"
#include "DataFormatsFT0/ChannelData.h"
#include "FT0/Utilities.h"

using namespace std;

namespace o2::quality_control_modules::ft0
{

void ChannelsCheck::configure(std::string) {}

Quality ChannelsCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  for (auto [name, obj] : *moMap)
  {

    if(obj->getName() == "EventTree"){
      TTree* tree =  dynamic_cast<TTree*>(obj->getObject());
      if(tree->GetEntries() == 0){
        return Quality::Bad;
      }

      EventWithChannelData event, *pEvent = &event;
      tree->SetBranchAddress("EventWithChannelData", &pEvent);
      for(unsigned int i = 0; i < tree->GetEntries(); ++i){
        tree->GetEntry(i);
        const auto& channels = event.getChannels();
        
        if(channels.empty()){
          return Quality::Bad;
        }

        for(const auto& channel : channels){
          if(   channel.ChId == 0xff
             || channel.ChainQTC == 0xff
             || channel.CFDTime == -1000
             || channel.QTCAmpl == -1000){
               return Quality::Bad;
             }
        }

      }

      return Quality::Good;
    }
  }

  return Quality::Bad;
}

std::string ChannelsCheck::getAcceptedType() { return "TH1"; }

void ChannelsCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{ 
  
}

} // namespace o2::quality_control_modules::mft
