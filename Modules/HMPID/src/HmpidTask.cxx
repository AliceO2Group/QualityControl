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
/// \author Antonio Franco, Giacomo Volpe, Antonio Paz
/// \brief      Class for quality control of HMPID detectors
/// \version    0.2.4
/// \date       19/05/2022
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TProfile.h>
#include <TProfile2D.h>
#include <TMath.h>
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/DataRefUtils.h>

#include "QualityControl/QcInfoLogger.h"
#include "HMPID/HmpidTask.h"
#include "HMPIDReconstruction/HmpidEquipment.h"
#include "HMPIDReconstruction/HmpidDecoder2.h"
#include "DataFormatsHMP/Digit.h"

namespace o2::quality_control_modules::hmpid
{

HmpidTask::~HmpidTask()
{
  delete hPedestalMean;
  delete hPedestalSigma;
  delete hBusyTime;
  delete hEventSize;
  delete hEventNumber;
  for (Int_t i = 0; i < numCham; ++i) {
    delete hModuleMap[i];
  }
  delete hHmpBigMap_profile;
  delete hHmpHvSectorQ;
  delete hHmpPadOccPrf;
  delete CheckerMessages;
  delete hCheckHV;
}

Int_t NumCycles = 0;

void HmpidTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize HmpidTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - myOwnKey: " << param->second << ENDM;
  }

  hPedestalMean = new TH1F("hPedestalMean", "Pedestal Mean", 2000, 0, 2000);
  hPedestalMean->SetXTitle("Pedestal mean (ADC channel)");
  hPedestalMean->SetYTitle("Entries/1 ADC");

  hPedestalSigma = new TH1F("hPedestalSigma", "Pedestal Sigma", 100, 0, 10);
  hPedestalSigma->SetXTitle("Pedestal sigma (ADC channel)");
  hPedestalSigma->SetYTitle("Entries/0.1 ADC");

  // TProfiles
  hBusyTime = new TProfile("hBusyTime", "HMP Busy Time per DDL;DDL;Busy Time (#mus)", 14, 0.5, 14.5);
  hBusyTime->Sumw2();
  hBusyTime->SetOption("histE");
  hBusyTime->SetMinimum(0);
  hBusyTime->SetMarkerStyle(20);
  hBusyTime->SetMarkerColor(kBlack);
  hBusyTime->SetLineColor(kBlack);
  hBusyTime->SetFillStyle(3004);
  for (Int_t iddl = 0; iddl < 14; iddl++)
    hBusyTime->GetXaxis()->SetBinLabel(iddl + 1, Form("%d", iddl + 1));
  hBusyTime->SetStats(0);
  hBusyTime->GetXaxis()->SetLabelSize(0.025);
  hBusyTime->GetYaxis()->SetLabelSize(0.025);

  hEventSize = new TProfile("hEventSize", "HMP Event Size per DDL;DDL;Event Size (kB)", 14, 0.5, 14.5);
  hEventSize->Sumw2();
  hEventSize->SetOption("E"); // hist
  hEventSize->SetMinimum(0);
  hEventSize->SetMarkerStyle(20);
  hEventSize->SetMarkerColor(kBlack);
  hEventSize->SetLineColor(kBlack);
  hEventSize->SetFillStyle(3004);
  for (Int_t iddl = 0; iddl < 14; iddl++)
    hEventSize->GetXaxis()->SetBinLabel(iddl + 1, Form("%d", iddl + 1));
  hEventSize->SetStats(0);
  hEventSize->GetXaxis()->SetLabelSize(0.025);
  hEventSize->GetYaxis()->SetLabelSize(0.025);

  hEventNumber = new TProfile("hEventNumber", "HMP Event Number per DDL;DDL;Event Number", 14, 0.5, 14.5);
  hEventNumber->Sumw2();
  hEventNumber->SetOption("E"); // hist
  hEventNumber->SetMinimum(0);
  hEventNumber->SetMarkerStyle(20);
  hEventNumber->SetMarkerColor(kBlack);
  hEventNumber->SetLineColor(kBlack);
  for (Int_t iddl = 0; iddl < 14; iddl++)
    hEventNumber->GetXaxis()->SetBinLabel(iddl + 1, Form("%d", iddl + 1));
  hEventNumber->SetStats(0);
  hEventNumber->GetXaxis()->SetLabelSize(0.025);
  hEventNumber->GetYaxis()->SetLabelSize(0.025);

  for (Int_t i = 0; i < numCham; ++i) {
    hModuleMap[i] = new TH2F(Form("hModuleMap%i", i), Form("Coordinates of hits in chamber %i", i), 160, 0, 160, 144, 0, 144);
    hModuleMap[i]->SetXTitle("X coordinate");
    hModuleMap[i]->SetYTitle("Y coordinate");
    hModuleMap[i]->SetMarkerStyle(20);

    getObjectsManager()->startPublishing(hModuleMap[i]);
    getObjectsManager()->setDefaultDrawOptions(hModuleMap[i], "colz");
    getObjectsManager()->setDisplayHint(hModuleMap[i], "colz");
  }

  hHmpBigMap_profile = new TProfile2D("hHmpBigMap_profile", "HMP Sum Q Maps Ch: 0-6", 160, 0, 160, 1008, 0, 1008);
  hHmpBigMap_profile->SetXTitle("Ch 0-6: pad X");
  hHmpBigMap_profile->SetYTitle("Ch0, Ch1, Ch2, Ch3, Ch4, Ch5, Ch6 pad Y");
  hHmpBigMap_profile->SetZTitle("Sum Q / Nevt");
  hHmpBigMap_profile->SetMarkerStyle(20);
  hHmpBigMap_profile->SetStats(0);
  hHmpBigMap_profile->GetXaxis()->SetLabelSize(0.025);
  hHmpBigMap_profile->GetYaxis()->SetLabelSize(0.025);
  hHmpBigMap_profile->GetZaxis()->SetLabelSize(0.015);
  hHmpBigMap_profile->GetXaxis()->SetTitleOffset(1.);
  hHmpBigMap_profile->GetYaxis()->SetTitleOffset(1.4);

  hHmpHvSectorQ = new TH2F("hHmpHvSectorQ", "HMP HV Sector vs Q", 410, 1, 4101, 42, 0, 42);
  hHmpHvSectorQ->SetXTitle("Q (ADC)");
  hHmpHvSectorQ->SetYTitle("HV Sector (Ch0-Sc0,Ch0-Sc1,...)");
  hHmpHvSectorQ->SetZTitle("Entries*Q/Nevt");
  hHmpHvSectorQ->SetMarkerStyle(20);
  hHmpHvSectorQ->SetStats(0);
  hHmpHvSectorQ->GetXaxis()->SetLabelSize(0.025);
  hHmpHvSectorQ->GetYaxis()->SetLabelSize(0.025);
  hHmpHvSectorQ->GetZaxis()->SetLabelSize(0.015);
  hHmpHvSectorQ->GetXaxis()->SetTitleOffset(1.);
  hHmpHvSectorQ->GetYaxis()->SetTitleOffset(1.4);

  hHmpPadOccPrf = new TProfile("hHmpPadOccPrf", "HMP Average pad occupancy per DDL;DDL;Pad occupancy (%)", 14, 0.5, 14.5);
  hHmpPadOccPrf->Sumw2();
  hHmpPadOccPrf->SetOption("hist"); // E
  hHmpPadOccPrf->SetMinimum(0);
  hHmpPadOccPrf->SetMarkerStyle(20);
  hHmpPadOccPrf->SetMarkerColor(kBlack);
  hHmpPadOccPrf->SetLineColor(kBlack);
  hHmpPadOccPrf->SetFillStyle(3004);
  for (Int_t iddl = 0; iddl < 14; iddl++)
    hHmpPadOccPrf->GetXaxis()->SetBinLabel(iddl + 1, Form("%d", iddl + 1));
  // hHmpPadOccPrf->GetXaxis()->SetLabelSize(0.02);
  hHmpPadOccPrf->SetStats(0);
  hHmpPadOccPrf->GetYaxis()->SetDecimals(1);
  hHmpPadOccPrf->GetXaxis()->SetLabelSize(0.025);
  hHmpPadOccPrf->GetYaxis()->SetLabelSize(0.025);

  getObjectsManager()->startPublishing(hPedestalMean);

  getObjectsManager()->startPublishing(hPedestalSigma);

  getObjectsManager()->startPublishing(hBusyTime);
  getObjectsManager()->setDefaultDrawOptions(hBusyTime, "E"); // hist E
  getObjectsManager()->setDisplayHint(hBusyTime, "E");        // hist E

  getObjectsManager()->startPublishing(hEventSize);
  getObjectsManager()->setDefaultDrawOptions(hEventSize, "E"); // hist E
  getObjectsManager()->setDisplayHint(hEventSize, "E");        // hist E

  getObjectsManager()->startPublishing(hEventNumber);

  getObjectsManager()->startPublishing(hHmpBigMap_profile);
  getObjectsManager()->setDefaultDrawOptions(hHmpBigMap_profile, "colz");
  getObjectsManager()->setDisplayHint(hHmpBigMap_profile, "colz");

  getObjectsManager()->startPublishing(hHmpHvSectorQ);
  getObjectsManager()->setDefaultDrawOptions(hHmpHvSectorQ, "colz");
  getObjectsManager()->setDisplayHint(hHmpHvSectorQ, "colz");

  getObjectsManager()->startPublishing(hHmpPadOccPrf);
  getObjectsManager()->setDefaultDrawOptions(hHmpPadOccPrf, "E"); // hist E
  getObjectsManager()->setDisplayHint(hHmpPadOccPrf, "E");        // hist E

  // Error messages
  CheckerMessages = new TCanvas("CheckerMessages");
  getObjectsManager()->startPublishing(CheckerMessages);

  // TH2 to check HV
  hCheckHV = new TH2F("hCheckHV", "hCheckHV", 42, -0.5, 41.5, 4, 0, 4);
  hCheckHV->SetXTitle("HV Sector (Ch0-Sc0,Ch0-Sc1,...)");
  hCheckHV->SetYTitle("Quality");
  hCheckHV->SetMarkerStyle(20);
  hCheckHV->SetStats(0);
  hCheckHV->GetXaxis()->SetLabelSize(0.025);
  hCheckHV->GetYaxis()->SetLabelSize(0.025);
  hCheckHV->GetXaxis()->SetTitleOffset(1.);
  hCheckHV->GetYaxis()->SetTitleOffset(1.4);
  hCheckHV->SetMinimum(0);
  hCheckHV->SetMaximum(2);
  getObjectsManager()->startPublishing(hCheckHV);
  getObjectsManager()->setDefaultDrawOptions(hCheckHV, "col");
  getObjectsManager()->setDisplayHint(hCheckHV, "col");
}

void HmpidTask::startOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;

  HmpidTask::reset();

  mDecoder = new o2::hmpid::HmpidDecoder2(14);
  mDecoder->init();
  mDecoder->setVerbosity(2); // this is for Debug
}

void HmpidTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void HmpidTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  mDecoder->init();
  mDecoder->setVerbosity(2); // this is for Debug
                             //  static const Int_t numCham = 7;
  static AliceO2::InfoLogger::InfoLogger::AutoMuteToken msgLimit(LogErrorDevel, 1, 600); // send it only every 10 minutes

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
        ILOG_INST.log(msgLimit, "Error decoding the Superpage !");
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
        if (mDecoder->mTheEquipments[eq]->mNumberOfEvents > 0) {
          hHmpPadOccPrf->Fill(eqId + 1, (100. * mDecoder->mTheEquipments[eq]->mTotalPads) / (11520. * mDecoder->mTheEquipments[eq]->mNumberOfEvents));
        }

        hEventNumber->Fill(eqId + 1, mDecoder->mTheEquipments[eq]->mEventNumber);

        int module, x, y;

        for (Int_t column = 0; column < 24; column++) {
          for (Int_t dilogic = 0; dilogic < 10; dilogic++) {
            for (Int_t channel = 0; channel < 48; channel++) {
              Int_t n_samp = mDecoder->mTheEquipments[eq]->mPadSamples[column][dilogic][channel];
              if (n_samp > 0) {
                Float_t mean = mDecoder->getChannelSum(eqId, column, dilogic, channel) / n_samp;
                Float_t sigma = TMath::Sqrt(mDecoder->getChannelSquare(eqId, column, dilogic, channel) / n_samp - mean * mean);
                hPedestalMean->Fill(mean);
                hPedestalSigma->Fill(sigma);
                o2::hmpid::Digit::equipment2Absolute(eqId, column, dilogic, channel, &module, &x, &y);
                hModuleMap[module]->Fill(x, y, mean);
                hHmpBigMap_profile->Fill(x, module * 144 + y, mean);
                if (mDecoder->mTheEquipments[eq]->mNumberOfEvents > 0) {
                  hHmpHvSectorQ->Fill(mean, module * 6 + y / 24, mean / Float_t(mDecoder->mTheEquipments[eq]->mNumberOfEvents));
                }
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
}

void HmpidTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
  NumCycles++;
  if (NumCycles > 15) {
    HmpidTask::reset();
    NumCycles = 0;
  }
}

void HmpidTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void HmpidTask::reset()
{
  // clean all the monitor objects here
  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  hPedestalMean->Reset();
  hPedestalSigma->Reset();
  hBusyTime->Reset();
  hEventSize->Reset();
  hEventNumber->Reset();
  for (Int_t i = 0; i < numCham; ++i) {
    hModuleMap[i]->Reset();
  }
  hHmpBigMap_profile->Reset();
  hHmpHvSectorQ->Reset();
  hHmpPadOccPrf->Reset();
  hCheckHV->Reset();
  for (int i = 1; i <= hCheckHV->GetNbinsX(); i++) {
    hCheckHV->SetBinContent(i, hCheckHV->GetYaxis()->FindBin("Null"), 0.001);
    hCheckHV->SetBinContent(i, hCheckHV->GetYaxis()->FindBin("Good"), -0.001);
    hCheckHV->SetBinContent(i, hCheckHV->GetYaxis()->FindBin("Bad"), -0.001);
    hCheckHV->SetBinContent(i, hCheckHV->GetYaxis()->FindBin(""), -0.001);
  }
}

} // namespace o2::quality_control_modules::hmpid
