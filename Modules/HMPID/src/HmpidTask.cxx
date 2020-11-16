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
/// \file   HmpidTask.cxx
/// \author My Name
///

#include <TCanvas.h>
#include <TH1.h>
#include <TMath.h>

#include "QualityControl/QcInfoLogger.h"
#include "HMPID/HmpidDecodeRawMem.h"
#include "HMPID/HmpidTask.h"

namespace o2::quality_control_modules::hmpid
{

HmpidTask::~HmpidTask()
{
  if (hPedestalMean) {
    delete hPedestalMean;
  }

  if (hPedestalSigma) {
    delete hPedestalSigma;
  }
    
  if (hBusyTime) {
    delete hBusyTime;
  }
    
  if (hEventSize) {
    delete hEventSize;
  }
    
}

Int_t nCycles = 0;

void HmpidTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info) << "initialize HmpidTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
    ILOG(Info) << "Custom parameter - myOwnKey: " << param->second << ENDM;
  }

  hPedestalMean = new TH1F("hPedestalMean", "Pedestal Mean", 2000, 0, 2000);
  hPedestalMean->SetXTitle("Pedestal mean (ADC channel)"); hPedestalMean->SetYTitle("Entries/1 ADC");
  
  hPedestalSigma = new TH1F("hPedestalSigma", "Pedestal Sigma", 100, 0, 10);
  hPedestalSigma->SetXTitle("Pedestal sigma (ADC channel)"); hPedestalMean->SetYTitle("Entries/0.1 ADC");
  
  hBusyTime = new TH1F("hBusyTime", "Average Busy Time", 14, 0, 14);
  hBusyTime->SetXTitle("Equipment"); hBusyTime->SetYTitle("Busy time (#mus)");
  hBusyTime->SetMarkerStyle(20);
  
  hEventSize = new TH1F("hEventSize", "Average Event Size", 14, 0, 14);
  hEventSize->SetXTitle("Equipment"); hEventSize->SetYTitle("Event size (kB)");
  hEventSize->SetMarkerStyle(20);  

  getObjectsManager()->startPublishing(hPedestalMean);
  getObjectsManager()->addMetadata(hPedestalMean->GetName(), "custom", "34");
  
  getObjectsManager()->startPublishing(hPedestalSigma);
  getObjectsManager()->addMetadata(hPedestalSigma->GetName(), "custom", "34");

  getObjectsManager()->startPublishing(hBusyTime);
  getObjectsManager()->addMetadata(hBusyTime->GetName(), "custom", "34");

  getObjectsManager()->startPublishing(hEventSize);
  getObjectsManager()->addMetadata(hEventSize->GetName(), "custom", "34");  
}

void HmpidTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "startOfActivity" << ENDM;
  hPedestalMean->Reset();
  hPedestalSigma->Reset();
  hBusyTime->Reset();
  hEventSize->Reset();
}

void HmpidTask::startOfCycle()
{
  ILOG(Info) << "startOfCycle" << ENDM;
}

void HmpidTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  nCycles++;

  Printf("******************IN HMPID MONITOR DATA*********************************************");
  // In this function you can access data inputs specified in the JSON config file, for example:
  //   "query": "random:ITS/RAWDATA/0"
  // which is correspondingly <binding>:<dataOrigin>/<dataDescription>/<subSpecification
  // One can also access conditions from CCDB, via separate API (see point 3)

  // Use Framework/DataRefUtils.h or Framework/InputRecord.h to access and unpack inputs (both are documented)
  // One can find additional examples at:
  // https://github.com/AliceO2Group/AliceO2/blob/dev/Framework/Core/README.md#using-inputs---the-inputrecord-api

  // Some examples:

  // We define an array with the Equipment Ids that will be managed from the Stream
  int EqIdsArray[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14};
  int CruIdsArray[] = {1,2,3,4};
  int LinkIdsArray[] = {1,2,3,4};
  // Get a Decoder Hinstance
  HmpidDecodeRawMem *Decoder = new HmpidDecodeRawMem(14);
//  HmpidDecoder *Decoder = new HmpidDecoder(EqIdsArray, 14);
  Decoder->init();
  Decoder->setVerbosity( 7 ); // this is for Debug 
 

 // mHistogram->Fill(gRandom->Gaus(250.,100.));

  // 1. In a loop
  for (auto&& input : ctx.inputs()) {
    // get message header
    if (input.header != nullptr && input.payload != nullptr) {
      const auto* header = header::get<header::DataHeader*>(input.header);
      // get payload of a specific input, which is a char array.
      // const char* payload = input.payload;

      // for the sake of an example, let's fill the histogram with payload sizes
      //mHistogram->Fill(header->payloadSize);
      int32_t *ptrToPayload = (int32_t *)(input.payload);

      Printf("chunk size = %f", header->payloadSize);
                  
      Decoder->setUpStream(ptrToPayload, header->payloadSize);
      
      if( !Decoder->decodeBuffer() ) {
        ILOG(Error) << "Error decoding the Superpage !" << ENDM;
      }
      
      for(Int_t eq = 0; eq<14; eq++) {
        
        if(Decoder->getAverageEventSize(eq) > 0.) {hEventSize->SetBinContent(eq+1,Decoder->getAverageEventSize(eq)/1000.); hEventSize->SetBinError(eq+1,0.0000001);}
        if(Decoder->getAverageBusyTime(eq)  > 0.) {hBusyTime->SetBinContent(eq+1,Decoder->getAverageBusyTime(eq)*1000000); hBusyTime->SetBinError(eq+1,0.00000001);}
        
        Printf("eq = %i, size = %f, busy = %f",eq, Decoder->getAverageEventSize(eq), Decoder->getAverageBusyTime(eq));
        
        for(Int_t column = 0; column < 24; column++)
          for(Int_t dilogic = 0; dilogic<10; dilogic++)
            for(Int_t channel = 0; channel<48; channel++){
        
                Float_t mean = Decoder->getChannelSum(eq,column,dilogic,channel)/Decoder->getChannelSamples(eq,column,dilogic,channel);
                Float_t sigma = TMath::Sqrt(Decoder->getChannelSquare(eq,column,dilogic,channel)/Decoder->getChannelSamples(eq,column,dilogic,channel) - mean*mean);
                
               // if(mean>1) Printf("*******************************************   mean = %f, sigma = %f **************************************",mean,sigma);
                
                hPedestalMean->Fill(mean);
                hPedestalSigma->Fill(sigma);
               }
             }
        
      
      /* Access the pads
      
      uint16_t   Decoder->theEquipments[0..13]->padSamples[0..23][0..9][0..47]  Number of samples
      float      Decoder->theEquipments[0..13]->padSum[0..23][0..9][0..47]      Sum of the charge of all samples
      float      Decoder->theEquipments[0..13]->padSquares[0..23][0..9][0..47]  Sum of the charge squares of all samples

    Methods to access pads

  void ConvertCoords(int Mod, int Col, int Row, int *Equi, int *Colu, int *Dilo, int *Chan);

  uint16_t GetChannelSamples(int Equipment, int Column, int Dilogic, int Channel);
  float GetChannelSum(int Equipment, int Column, int Dilogic, int Channel);
  float GetChannelSquare(int Equipment, int Column, int Dilogic, int Channel);
  uint16_t GetPadSamples(int Module, int Column, int Row);
  float GetPadSum(int Module, int Column, int Row);
  float GetPadSquares(int Module, int Column, int Row);

      */
      
      
    }
  }
  
  if(nCycles > 50) {hPedestalMean->Reset(); hPedestalSigma->Reset(); nCycles = 0;}
 
  delete Decoder; 

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

void HmpidTask::endOfCycle()
{
  ILOG(Info) << "endOfCycle" << ENDM;
}

void HmpidTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "endOfActivity" << ENDM;
}

void HmpidTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info) << "Resetting the histogram" << ENDM;
  hPedestalMean->Reset();
  hPedestalSigma->Reset();
  hBusyTime->Reset();
  hEventSize->Reset();
}

} // namespace o2::quality_control_modules::hmpid
