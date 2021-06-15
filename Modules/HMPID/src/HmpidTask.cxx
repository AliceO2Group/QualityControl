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
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>

#include "QualityControl/QcInfoLogger.h"
//#include "HMPID/HmpidDecodeRawMem.h"
#include "HMPID/HmpidTask.h"
#include "HMPIDReconstruction/HmpidEquipment.h"
#include "HMPIDReconstruction/HmpidDecoder2.h"

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

Int_t NumCycles = 0;

void HmpidTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info) << "initialize HmpidTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
    ILOG(Info) << "Custom parameter - myOwnKey: " << param->second << ENDM;
  }

  hPedestalMean = new TH1F("hPedestalMean", "Pedestal Mean", 2000, 0, 2000);
  hPedestalMean->SetXTitle("Pedestal mean (ADC channel)");
  hPedestalMean->SetYTitle("Entries/1 ADC");

  hPedestalSigma = new TH1F("hPedestalSigma", "Pedestal Sigma", 100, 0, 10);
  hPedestalSigma->SetXTitle("Pedestal sigma (ADC channel)");
  hPedestalMean->SetYTitle("Entries/0.1 ADC");

  hBusyTime = new TH1F("hBusyTime", "Average Busy Time", 14, 0, 14);
  hBusyTime->SetXTitle("Equipment");
  hBusyTime->SetYTitle("Busy time (#mus)");
  hBusyTime->SetMarkerStyle(20);

  hEventSize = new TH1F("hEventSize", "Average Event Size", 14, 0, 14);
  hEventSize->SetXTitle("Equipment");
  hEventSize->SetYTitle("Event size (kB)");
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

  mDecoder = new o2::hmpid::HmpidDecoder2(14);
  mDecoder->init();
  mDecoder->setVerbosity(2); // this is for Debug
}

void HmpidTask::startOfCycle()
{
  ILOG(Info) << "startOfCycle" << ENDM;
}

void HmpidTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  NumCycles++;
  mDecoder->init();
  mDecoder->setVerbosity(2); // this is for Debug

  //for (auto&& input : ctx.inputs()) {
  for (auto&& input : o2::framework::InputRecordWalker(ctx.inputs())) {
    // get message header
    if (input.header != nullptr && input.payload != nullptr) {
      const auto* header = header::get<header::DataHeader*>(input.header);
      int32_t* ptrToPayload = (int32_t*)(input.payload);

      if (header->payloadSize < 80) {
        continue;
      }
      mDecoder->setUpStream(ptrToPayload, (long int)header->payloadSize);
      if (!mDecoder->decodeBufferFast()) {
        ILOG(Error) << "Error decoding the Superpage !" << ENDM;
      }
      for (Int_t eq = 0; eq < 14; eq++) {
        if (mDecoder->getAverageEventSize(eq) > 0.) {
          hEventSize->SetBinContent(eq + 1, mDecoder->getAverageEventSize(eq) / 1000.);
          hEventSize->SetBinError(eq + 1, 0.0000001);
        }
        if (mDecoder->getAverageBusyTime(eq) > 0.) {
          hBusyTime->SetBinContent(eq + 1, mDecoder->getAverageBusyTime(eq) * 1000000);
          hBusyTime->SetBinError(eq + 1, 0.00000001);
        }
        for (Int_t column = 0; column < 24; column++) {
          for (Int_t dilogic = 0; dilogic < 10; dilogic++) {
            for (Int_t channel = 0; channel < 48; channel++) {
              Float_t mean = mDecoder->getChannelSum(eq, column, dilogic, channel) / mDecoder->getChannelSamples(eq, column, dilogic, channel);
              Float_t sigma = TMath::Sqrt(mDecoder->getChannelSquare(eq, column, dilogic, channel) / mDecoder->getChannelSamples(eq, column, dilogic, channel) - mean * mean);
              hPedestalMean->Fill(mean);
              hPedestalSigma->Fill(sigma);
            }
          }
        }
      }

      /* Access the pads
      uint16_t   decoder.theEquipments[0..13]->padSamples[0..23][0..9][0..47]  Number of samples
      float      decoder.theEquipments[0..13]->padSum[0..23][0..9][0..47]      Sum of the charge of all samples
      float      decoder.theEquipments[0..13]->padSquares[0..23][0..9][0..47]  Sum of the charge squares of all samples
'     uint16_t GetChannelSamples(int Equipment, int Column, int Dilogic, int Channel);
      float GetChannelSum(int Equipment, int Column, int Dilogic, int Channel);
      float GetChannelSquare(int Equipment, int Column, int Dilogic, int Channel);
      uint16_t GetPadSamples(int Module, int Column, int Row);
      float GetPadSum(int Module, int Column, int Row);
      float GetPadSquares(int Module, int Column, int Row);
      */
    }
  }

  if (NumCycles > 50) {
    hPedestalMean->Reset();
    hPedestalSigma->Reset();
    NumCycles = 0;
  }
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
