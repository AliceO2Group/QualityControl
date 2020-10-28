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
/// \file   PostProcessingInterface.cxx
/// \author My Name
///

#include  "FT0/TreeReaderPostProcessing.h"
#include "QualityControl/QcInfoLogger.h"
#include "FT0/Utilities.h"

#include <TH1F.h>
#include <TTree.h>

using namespace o2::quality_control::postprocessing;

namespace o2::quality_control_modules::ft0
{

TreeReaderPostProcessing::~TreeReaderPostProcessing()
{
  
}

void TreeReaderPostProcessing::initialize(Trigger, framework::ServiceRegistry& services)
{
  mChargeHistogram  = std::make_unique<TH1F>("ChargeHistogram", "ChargeHistogram", 100, 0, 200);
  getObjectsManager()->startPublishing(mChargeHistogram.get());
  mDatabase = &services.get<o2::quality_control::repository::DatabaseInterface>();
}

void TreeReaderPostProcessing::update(Trigger t, framework::ServiceRegistry&)
{
  mChargeHistogram->Reset();
  auto mo = mDatabase->retrieveMO("qc/FT0/MO/BasicDigitQcTask", "EventTree");
  TTree* moTree = static_cast<TTree*>(mo ? mo->getObject() : nullptr);


  if(moTree){
    EventWithChannelData event, *pEvent = &event;
    moTree->SetBranchAddress("EventWithChannelData", &pEvent);
    for(unsigned int i = 0; i < moTree->GetEntries(); ++i){

      moTree->GetEntry(i);
      for(const auto& channel : event.getChannels()){
        mChargeHistogram->Fill(channel.QTCAmpl);

      }
    }
  }



}

void TreeReaderPostProcessing::finalize(Trigger, framework::ServiceRegistry&)
{
  getObjectsManager()->stopPublishing(mChargeHistogram.get());
}

} // namespace o2::quality_control_modules::skeleton
