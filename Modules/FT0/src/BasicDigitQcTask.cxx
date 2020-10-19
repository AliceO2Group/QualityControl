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
/// \file   BasicDigitQcTask.cxx
/// \author My Name
///

#include "TCanvas.h"
#include "TH1.h"
#include "TFile.h"
#include "TGraph.h"

#include "QualityControl/QcInfoLogger.h"
#include "FT0/BasicDigitQcTask.h"
#include "FT0/Utilities.h"
#include "DataFormatsFT0/Digit.h"
#include "DataFormatsFT0/ChannelData.h"

namespace o2::quality_control_modules::ft0
{

BasicDigitQcTask::~BasicDigitQcTask()
{

}

void BasicDigitQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize BasicDigitQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  // if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
  //   ILOG(Info, Devel) << "Custom parameter - myOwnKey: " << param->second << ENDM;
  // }

  mChargeHistogram = std::make_unique<TH1F>("Charge", "Charge", 200, 0, 200);
  mTimeHistogram = std::make_unique<TH1F>("Time", "Time", 200, 0, 200);
  mAmplitudeAndTime = std::make_unique<TH2F>("ChargeAndTime", "ChargeAndTime", 10, 0, 200, 10, 0, 200);
  mAmplitudeByTime = std::make_unique<TMultiGraph>("MultiHisto", "Amplitude as a function of CFDTime");
  mTTree = std::make_unique<TTree>("EventTree", "EventTree");


  getObjectsManager()->startPublishing(mChargeHistogram.get());
  getObjectsManager()->startPublishing(mTimeHistogram.get());
  getObjectsManager()->startPublishing(mTTree.get());
  getObjectsManager()->startPublishing(mAmplitudeByTime.get());
  getObjectsManager()->startPublishing(mAmplitudeAndTime.get());
  // try {
  //   getObjectsManager()->addMetadata(mHistogram->GetName(), "custom", "34");
  // } catch (...) {
  //   // some methods can throw exceptions, it is indicated in their doxygen.
  //   // In case it is recoverable, it is recommended to catch them and do something meaningful.
  //   // Here we don't care that the metadata was not added and just log the event.
  //   ILOG(Warning, Support) << "Metadata could not be added to " << mHistogram->GetName() << ENDM;
  // }
}

void BasicDigitQcTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity" << activity.mId << ENDM;
  mTimeHistogram->Reset();
  mChargeHistogram->Reset();
  mTTree->Reset();
}

void BasicDigitQcTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}



void BasicDigitQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // In this function you can access data inputs specified in the JSON config file, for example:
  //   "query": "random:ITS/RAWDATA/0"
  // which is correspondingly <binding>:<dataOrigin>/<dataDescription>/<subSpecification
  // One can also access conditions from CCDB, via separate API (see point 3)

  // Use Framework/DataRefUtils.h or Framework/InputRecord.h to access and unpack inputs (both are documented)
  // One can find additional examples at:
  // https://github.com/AliceO2Group/AliceO2/blob/dev/Framework/Core/README.md#using-inputs---the-inputrecord-api

  // Some examples:

  // 1. In a loop
  // for (auto&& input : ctx.inputs()) {
  //   // get message header
  //   // ILOG(Info, Support) << "Getting inputs: " << ENDM;
  //   // ILOG(Info, Support) << "Type: " <<  typeid(input).name() << ENDM;



  // }



  auto channels = ctx.inputs().get<gsl::span< o2::ft0::ChannelData> >("channels");
  auto digits = ctx.inputs().get<gsl::span< o2::ft0::Digit> >("digits");

  ILOG(Info, Support) << "Channels size: " << channels.size() << " Digits size: " << digits.size() << ENDM;

  std::vector<o2::ft0::ChannelData> channelDataCopy(channels.begin(), channels.end());
  std::vector<o2::ft0::Digit> digitDataCopy(digits.begin(), digits.end());

  EventWithChannelData event;
  mTTree->Branch("EventWithChannelData", &event);
  std::vector< TGraph* > graphs;
  int toSaveCounter = 0;

  for(auto& digit : digits){
    auto currentChannels = digit.getBunchChannelData(channels);
    auto timestamp = o2::InteractionRecord::bc2ns(digit.getBC(), digit.getOrbit());
    event = EventWithChannelData{digit.getEventID(), digit.getBC(), digit.getOrbit(), timestamp, std::vector<o2::ft0::ChannelData>(currentChannels.begin(), currentChannels.end()) };
    mTTree->Fill();


    if(toSaveCounter++ < 1){
      graphs.emplace_back( new TGraph(currentChannels.size()));
      unsigned counter = 0;
      for(auto& channel : currentChannels)
      {
        
        mChargeHistogram->Fill(channel.QTCAmpl);
        mTimeHistogram->Fill(channel.CFDTime);
        graphs.back()->SetPoint(counter++, channel.CFDTime, channel.QTCAmpl);
      }
      graphs.back()->SetName(std::string("Event: " + std::to_string(digit.getEventID())).c_str());
    }


    for(auto& channel : currentChannels){
       mAmplitudeAndTime->Fill(channel.QTCAmpl, channel.CFDTime);

    }

    
  }
  
  mTTree->Print();
  for(auto& graph : graphs){
    mAmplitudeByTime->Add(graph, "PL");
  }

  mAmplitudeByTime->Draw("A pmc plc");

  auto hFile = std::make_unique<TFile>("debugTree.root", "RECREATE");
  auto treeCopy = mTTree->CloneTree();
  treeCopy->SetEntries();
  treeCopy->Write();
  mTTree->Print();
  hFile->Close();


  // int dummyValue = 5;
  // mTTree->Branch("DummyBranch", &dummyValue);
  // for(int i = 0; i < 10; ++i){
  //   mTTree->Fill();
  // }

  // mTTree->Print();
  // auto hFile = std::make_unique<TFile>("debugTree.root", "RECREATE");
  // mTTree->Write();
  // hFile->Close();



  // hFile = std::make_unique<TFile>("debugTree.root", "OLD");
  // if (!hFile->IsOpen()) {
  //   throw std::runtime_error("cannot reopen my root tree");
  // }

  // std::unique_ptr<TTree> reopenedTree;
  // reopenedTree.reset((TTree*)hFile->Get("EventTree"));
  // if (!reopenedTree) {
  //   throw std::runtime_error("Cannot find defined branch in my root tree");
  // }


  // ILOG(Info, Support) << "Entries: " << reopenedTree->GetEntries() << ENDM;


  
  // mTTree->Write();

  // for (const auto& one_digit : digits) {
  //   mChargeHistogram->Fill(one_digit.QTCAmpl);
  //   mTimeHistogram->Fill(one_digit.CFDTime);
  // }



  

  // 2. Using get("<binding>")

  // get the payload of a specific input, which is a char array. "random" is the binding specified in the config file.
  //   auto payload = ctx.inputs().get("random").payload;

  // get payload of a specific input, which is a structure array:
  //  const auto* header = header::get<header::DataHeader*>(ctx.inputs().get("random").header);
  //  struct s {int a; double b;};
  //  auto array = ctx.inputs().get<s*>("random");
  //  for (int j = 0; j < header->payloadSize / sizeof(s); ++j) {
  //    int i = array.get()[j].a;
  //  }

  // get payload of a specific input, which is a root object
  //   auto h = ctx.inputs().get<TH1F*>("histos");
  //   Double_t stats[4];
  //   h->GetStats(stats);
  //   auto s = ctx.inputs().get<TObjString*>("string");
  //   LOG(INFO) << "String is " << s->GetString().Data();

  // 3. Access CCDB. If it is enough to retrieve it once, do it in initialize().
  // Remember to delete the object when the pointer goes out of scope or it is no longer needed.
  //   TObject* condition = TaskInterface::retrieveCondition("QcTask/example"); // put a valid condition path here
  //   if (condition) {
  //     LOG(INFO) << "Retrieved " << condition->ClassName();
  //     delete condition;
  //   }
}

void BasicDigitQcTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void BasicDigitQcTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void BasicDigitQcTask::reset()
{
  // clean all the monitor objects here

  mTimeHistogram->Reset();
  mChargeHistogram->Reset();
  mTTree->Reset();
  mAmplitudeAndTime->Reset();
}

} // namespace o2::quality_control_modules::ft0
