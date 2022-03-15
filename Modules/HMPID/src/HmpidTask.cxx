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
/// \file   HmpidTask.cxx
/// \author Antonio Franco, Giacomo Volpe
///

#include <TCanvas.h>
#include <TH1.h>
#include <TProfile.h>
#include <TMath.h>
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/DataRefUtils.h>

#include "QualityControl/QcInfoLogger.h"
#include "HMPID/HmpidTask.h"
#include "HMPIDReconstruction/HmpidEquipment.h"
#include "HMPIDReconstruction/HmpidDecoder2.h"

namespace o2::quality_control_modules::hmpid
{

HmpidTask::~HmpidTask()
{
  delete hPedestalMean;
  delete hPedestalSigma;
  delete hBusyTime;
  delete hEventSize;
  delete hEventNumber;
}

Int_t NumCycles = 0;

void HmpidTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize HmpidTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - myOwnKey: " << param->second << ENDM;
  }

  hPedestalMean = new TH1F("hPedestalMean", "Pedestal Mean", 2000, 0, 2000);
  hPedestalMean->SetXTitle("Pedestal mean (ADC channel)");
  hPedestalMean->SetYTitle("Entries/1 ADC");

  hPedestalSigma = new TH1F("hPedestalSigma", "Pedestal Sigma", 100, 0, 10);
  hPedestalSigma->SetXTitle("Pedestal sigma (ADC channel)");
  hPedestalMean->SetYTitle("Entries/0.1 ADC");

  // TProfiles
  hBusyTime = new TProfile("hBusyTime", "HMP Busy Time per DDL;;Busy Time (#mus)", 14, 0.5, 14.5);
  hBusyTime->Sumw2();
  hBusyTime->SetOption("P");
  hBusyTime->SetMinimum(0);
  hBusyTime->SetMarkerStyle(20);
  hBusyTime->SetMarkerColor(kBlack);
  hBusyTime->SetLineColor(kBlack);
  for (Int_t iddl = 0; iddl < 14; iddl++)
    hBusyTime->GetXaxis()->SetBinLabel(iddl + 1, Form("%d", iddl + 1));
  hBusyTime->GetXaxis()->SetLabelSize(0.02);
  hBusyTime->SetStats(0);

  hEventSize = new TProfile("hEventSize", "HMP Event Size per DDL;;Event Size (kB)", 14, 0.5, 14.5);
  hEventSize->Sumw2();
  hEventSize->SetOption("P");
  hEventSize->SetMinimum(0);
  hEventSize->SetMarkerStyle(20);
  hEventSize->SetMarkerColor(kBlack);
  hEventSize->SetLineColor(kBlack);
  for (Int_t iddl = 0; iddl < 14; iddl++)
    hEventSize->GetXaxis()->SetBinLabel(iddl + 1, Form("%d", iddl + 1));
  hEventSize->GetXaxis()->SetLabelSize(0.02);
  hEventSize->SetStats(0);

  hEventNumber = new TProfile("hEventNumber", "HMP Event Number per DDL;;Event Number", 14, 0.5, 14.5);
  hEventNumber->Sumw2();
  hEventNumber->SetOption("P");
  hEventNumber->SetMinimum(0);
  hEventNumber->SetMarkerStyle(20);
  hEventNumber->SetMarkerColor(kBlack);
  hEventNumber->SetLineColor(kBlack);
  for (Int_t iddl = 0; iddl < 14; iddl++)
    hEventNumber->GetXaxis()->SetBinLabel(iddl + 1, Form("%d", iddl + 1));
  hEventNumber->GetXaxis()->SetLabelSize(0.02);
  hEventNumber->SetStats(0);

  getObjectsManager()->startPublishing(hPedestalMean);

  getObjectsManager()->startPublishing(hPedestalSigma);

  getObjectsManager()->startPublishing(hBusyTime);

  getObjectsManager()->startPublishing(hEventSize);

  getObjectsManager()->startPublishing(hEventNumber);
}

void HmpidTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;
  hPedestalMean->Reset();
  hPedestalSigma->Reset();

  mDecoder = new o2::hmpid::HmpidDecoder2(14);
  mDecoder->init();
  mDecoder->setVerbosity(2); // this is for Debug
}

void HmpidTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void HmpidTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  NumCycles++;
  mDecoder->init();
  mDecoder->setVerbosity(2); // this is for Debug
  // for (auto&& input : ctx.inputs()) {
  for (auto&& input : o2::framework::InputRecordWalker(ctx.inputs())) {
    // get message header
    if (input.header != nullptr && input.payload != nullptr) {
      auto payloadSize = o2::framework::DataRefUtils::getPayloadSize(input);
      int32_t* ptrToPayload = (int32_t*)(input.payload);
      if (payloadSize < 80) {
        continue;
      }
      mDecoder->setUpStream(ptrToPayload, (long int)payloadSize);
      if (!mDecoder->decodeBufferFast()) {
        ILOG(Error, Devel) << "Error decoding the Superpage !" << ENDM;
        break;
      }
      for (Int_t eq = 0; eq < 14; eq++) {
        int eqId = mDecoder->mTheEquipments[eq]->getEquipmentId();
        if (mDecoder->getAverageEventSize(eqId) > 0.) {
          hEventSize->Fill(eqId + 1, mDecoder->getAverageEventSize(eqId) / 1000.);
        }
        if (mDecoder->getAverageBusyTime(eqId) > 0.) {
          hBusyTime->Fill(eqId + 1, mDecoder->getAverageBusyTime(eqId) * 1000000);
        }

        hEventNumber->Fill(eqId + 1, mDecoder->mTheEquipments[eq]->mEventNumber);

        for (Int_t column = 0; column < 24; column++) {
          for (Int_t dilogic = 0; dilogic < 10; dilogic++) {
            for (Int_t channel = 0; channel < 48; channel++) {
              Int_t n_samp = mDecoder->mTheEquipments[eq]->mPadSamples[column][dilogic][channel];
              if (n_samp > 0) {
                Float_t mean = mDecoder->getChannelSum(eqId, column, dilogic, channel) / n_samp;
                Float_t sigma = TMath::Sqrt(mDecoder->getChannelSquare(eqId, column, dilogic, channel) / n_samp - mean * mean);
                hPedestalMean->Fill(mean);
                hPedestalSigma->Fill(sigma);
              }
            }
          }
        }
      }

      /* Access the pads
      uint16_t   decoder.theEquipments[0..13]->padSamples[0..23][0..9][0..47]  Number of samples
      float      decoder.theEquipments[0..13]->padSum[0..23][0..9][0..47]      Sum of the charge of all samples
      float      decoder.theEquipments[0..13]->padSquares[0..23][0..9][0..47]  Sum of the charge squares of all samples
      uint16_t GetChannelSamples(int Equipment, int Column, int Dilogic, int Channel);
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
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void HmpidTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void HmpidTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  hPedestalMean->Reset();
  hPedestalSigma->Reset();
  hBusyTime->Reset();
  hEventSize->Reset();
  hEventNumber->Reset();
}

} // namespace o2::quality_control_modules::hmpid
