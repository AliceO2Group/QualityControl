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
#include "FT0/DigitsCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include "DataFormatsFT0/Digit.h"
#include "DataFormatsFT0/ChannelData.h"
#include "FT0/Utilities.h"

using namespace std;

namespace o2::quality_control_modules::ft0
{

void DigitsCheck::configure(std::string) {}

Quality DigitsCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
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
        if(  event.getEventID() < 0
          || event.getBC() == o2::InteractionRecord::DummyBC
          || event.getOrbit() == o2::InteractionRecord::DummyOrbit
          || event.getTimestamp() == 0
          || event.getChannels().empty())
        {
          ILOG(Info, Support) << i << " " << event.getEventID() << " " << event.getBC() << " " << event.getOrbit() << " " << event.getTimestamp() << " " << event.getChannels().empty() << ENDM;

          return Quality::Bad;
        }
      }


      // ILOG(Info, Support) << "Entries: " << tree->GetEntries() << ENDM;

      // std::vector<o2::ft0::Digit> digits, *pdigits = &digits;
      // std::vector<o2::ft0::ChannelData> channels, *pchannels = &channels;

      // tree->SetBranchAddress("Test_Digits", &pdigits);
      // tree->SetBranchAddress("Test_Channels", &pchannels);

      // tree->GetEntry(0);
      
      // gsl::span<o2::ft0::Digit> spanDigits(&digits[0], digits.size());
      // gsl::span<o2::ft0::ChannelData> spanChannels(&channels[0], channels.size());

      // ILOG(Info, Support) << "Digits: " << digits.size() << " Channels: " << channels.size() << ENDM;
      // for(const auto& digit : digits){


      //   ILOG(Info, Support) << "==============" << ENDM;
      //   ILOG(Info, Support) << "EventID: " << digit.getEventID() << " BC: " << digit.getBC()  << " Orbit: " << digit.getOrbit() << ENDM;
      //   auto currentChannelData = digit.getBunchChannelData(spanChannels);

      //   ILOG(Info, Support) << "ChannelData size: " <<currentChannelData.size() << ENDM;
      //   ILOG(Info, Support) << "==============" << ENDM;
      // }
      return Quality::Good;
    }
  }

  return Quality::Bad;
}

std::string DigitsCheck::getAcceptedType() { return "TH1"; }

void DigitsCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{ 
  
}

} // namespace o2::quality_control_modules::mft
