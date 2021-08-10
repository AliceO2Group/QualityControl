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
/// \file   PedestalTask.cxx
/// \author Sergey Evdokimov
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TF1.h>
#include <TSpectrum.h>

#include "QualityControl/QcInfoLogger.h"
#include "CPV/PedestalTask.h"
#include <Framework/InputRecord.h>
#include "DataFormatsCPV/TriggerRecord.h"

namespace o2::quality_control_modules::cpv
{

PedestalTask::~PedestalTask()
{
  for (int i = 0; i < kNHist1D; i++) {
    if (mHist1D[i]) {
      mHist1D[i]->Delete();
      mHist1D[i] = nullptr;
    }
  }
  for (int i = 0; i < kNHist2D; i++) {
    if (mHist2D[i]) {
      mHist2D[i]->Delete();
      mHist2D[i] = nullptr;
    }
  }
  for (int i = 0; i < kNChannels; i++) {
    if (mHistAmplitudes[i]) {
      mHistAmplitudes[i]->Delete();
      mHistAmplitudes[i] = nullptr;
    }
  }
}

void PedestalTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize PedestalTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("minNEventsToUpdatePedestals"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter : minNEventsToUpdatePedestals " << param->second << ENDM;
    mMinNEventsToUpdatePedestals = stoi(param->second);
    ILOG(Info, Devel) << "I set minNEventsToUpdatePedestals = " << mMinNEventsToUpdatePedestals << ENDM;
  }
  initHistograms();
  mNEventsTotal = 0;
  mNEventsFromLastFillHistogramsCall = 0;
}

void PedestalTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity() : resetting everything" << activity.mId << ENDM;
  resetHistograms();
  mNEventsTotal = 0;
  mNEventsFromLastFillHistogramsCall = 0;
}

void PedestalTask::startOfCycle()
{
  ILOG(Info, Devel) << "startOfCycle" << ENDM;
  //at at the startOfCycle all HistAmplitudes are not updated by definition
  for (int i = 0; i < kNChannels; i++)
    mIsUpdatedAmplitude[i] = false;
}

void PedestalTask::monitorData(o2::framework::ProcessingContext& ctx)
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
  int nInputs = ctx.inputs().size();
  mHist1D[H1DNInputs]->Fill(nInputs);

  int nValidInputs = ctx.inputs().countValidInputs();
  mHist1D[H1DNValidInputs]->Fill(nValidInputs);

  for (auto&& input : ctx.inputs()) {
    // get message header
    if (input.header != nullptr && input.payload != nullptr) {
      const auto* header = header::get<header::DataHeader*>(input.header);
      // get payload of a specific input, which is a char array.
      // const char* payload = input.payload;

      // for the sake of an example, let's fill the histogram with payload sizes
      mHist1D[H1DInputPayloadSize]->Fill(header->payloadSize);
    }
  }

  // 2. Using get("<binding>")
  auto digits = ctx.inputs().get<gsl::span<o2::cpv::Digit>>("digits");
  mHist1D[H1DNDigitsPerInput]->Fill(digits.size());

  auto digitsTR = ctx.inputs().get<gsl::span<o2::cpv::TriggerRecord>>("dtrigrec");
  //mNEventsTotal += digitsTR.size();//number of events in the current input
  for (const auto& trigRecord : digitsTR) {
    LOG(DEBUG) << " monitorData() : trigger record #" << mNEventsTotal
               << " contains " << trigRecord.getNumberOfObjects() << " objects." << ENDM;
    if (trigRecord.getNumberOfObjects() > 0) { //at least 1 digit -> pedestal event
      mNEventsTotal++;
      mNEventsFromLastFillHistogramsCall++;
      for (int iDig = trigRecord.getFirstEntry(); iDig < trigRecord.getFirstEntry() + trigRecord.getNumberOfObjects(); iDig++) {
        mHist1D[H1DDigitIds]->Fill(digits[iDig].getAbsId());
        short relId[3];
        if (mCPVGeometry.absToRelNumbering(digits[iDig].getAbsId(), relId)) {
          //reminder: relId[3]={Module, phi col, z row} where Module=2..4, phi col=0..127, z row=0..59
          mHist2D[H2DDigitMapM2 + relId[0] - 2]->Fill(relId[1], relId[2]);
          mHistAmplitudes[digits[iDig].getAbsId()]->Fill(digits[iDig].getAmplitude());
          mIsUpdatedAmplitude[digits[iDig].getAbsId()] = true;
        }
      }
    }
  }
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

void PedestalTask::endOfCycle()
{
  ILOG(Info, Devel) << "endOfCycle. I call fillHistograms()" << ENDM;
  //fit histograms if have sufficient increment of event number
  if (mNEventsFromLastFillHistogramsCall >= mMinNEventsToUpdatePedestals) {
    fillHistograms();
    mNEventsFromLastFillHistogramsCall = 0;
  }
}

void PedestalTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
  //do a final fill of histograms (if needed)
  if (mNEventsFromLastFillHistogramsCall) {
    ILOG(Info, Devel) << "Final call of fillHistograms() " << ENDM;
    fillHistograms();
  }
}

void PedestalTask::reset()
{
  // clean all the monitor objects here
  ILOG(Info, Support) << "Resetting PedestalTask" << ENDM;
  resetHistograms();
  mNEventsTotal = 0;
  mNEventsFromLastFillHistogramsCall = 0;
}

void PedestalTask::initHistograms()
{
  ILOG(Info, Devel) << "initing histograms" << ENDM;

  //create monitoring histograms (or reset, if they already exist)
  for (int i = 0; i < kNChannels; i++) {
    if (!mHistAmplitudes[i]) {
      mHistAmplitudes[i] =
        new TH1F(Form("HistAmplitude%d", i), Form("HistAmplitude%d", i), 4096, 0., 4096.);
      //publish some of them
      if (i % 1000 == 0) {
        getObjectsManager()->startPublishing(mHistAmplitudes[i]);
      }
    } else {
      mHistAmplitudes[i]->Reset();
    }
    mIsUpdatedAmplitude[i] = false;
  }

  //1D Histos
  if (!mHist1D[H1DInputPayloadSize]) {
    mHist1D[H1DInputPayloadSize] =
      new TH1F("InputPayloadSize", "Input Payload Size", 30000, 0, 30000000);
    getObjectsManager()->startPublishing(mHist1D[H1DInputPayloadSize]);
  } else {
    mHist1D[H1DInputPayloadSize]->Reset();
  }

  if (!mHist1D[H1DNInputs]) {
    mHist1D[H1DNInputs] = new TH1F("NInputs", "Number of inputs", 10, -0.5, 9.5);
    getObjectsManager()->startPublishing(mHist1D[H1DNInputs]);
  } else {
    mHist1D[H1DNInputs]->Reset();
  }

  if (!mHist1D[H1DNValidInputs]) {
    mHist1D[H1DNValidInputs] =
      new TH1F("NValidInputs", "Number of valid inputs", 10, -0.5, 9.5);
    getObjectsManager()->startPublishing(mHist1D[H1DNValidInputs]);
  } else {
    mHist1D[H1DNValidInputs]->Reset();
  }

  if (!mHist1D[H1DNDigitsPerInput]) {
    mHist1D[H1DNDigitsPerInput] =
      new TH1F("NDigitsPerInput", "Number of digits per input", 30000, 0, 300000);
    getObjectsManager()->startPublishing(mHist1D[H1DNDigitsPerInput]);
  } else {
    mHist1D[H1DNDigitsPerInput]->Reset();
  }

  if (!mHist1D[H1DDigitIds]) {
    mHist1D[H1DDigitIds] = new TH1F("DigitIds", "Digit Ids", 30000, -0.5, 29999.5);
    getObjectsManager()->startPublishing(mHist1D[H1DDigitIds]);
  } else {
    mHist1D[H1DDigitIds]->Reset();
  }

  for (int mod = 0; mod < kNModules; mod++) {
    if (!mHist1D[H1DPedestalValueM2 + mod]) {
      mHist1D[H1DPedestalValueM2 + mod] =
        new TH1F(
          Form("PedestalValueM%d", mod + 2),
          Form("Pedestal value distribution M%d", mod + 2),
          512, 0, 512);
      mHist1D[H1DPedestalValueM2 + mod]->GetXaxis()->SetTitle("Pedestal value");
      getObjectsManager()->startPublishing(mHist1D[H1DPedestalValueM2 + mod]);
    } else {
      mHist1D[H1DPedestalValueM2 + mod]->Reset();
    }

    if (!mHist1D[H1DPedestalSigmaM2 + mod]) {
      mHist1D[H1DPedestalSigmaM2 + mod] =
        new TH1F(
          Form("PedestalSigmaM%d", mod + 2),
          Form("Pedestal sigma distribution M%d", mod + 2),
          200, 0, 20);
      mHist1D[H1DPedestalSigmaM2 + mod]->GetXaxis()->SetTitle("Pedestal sigma");
      getObjectsManager()->startPublishing(mHist1D[H1DPedestalSigmaM2 + mod]);
    } else {
      mHist1D[H1DPedestalSigmaM2 + mod]->Reset();
    }

    if (!mHist1D[H1DPedestalEfficiencyM2 + mod]) {
      mHist1D[H1DPedestalEfficiencyM2 + mod] =
        new TH1F(
          Form("PedestalEfficiencyM%d", mod + 2),
          Form("Pedestal efficiency distribution M%d", mod + 2),
          500, 0., 5.);
      mHist1D[H1DPedestalEfficiencyM2 + mod]->GetXaxis()->SetTitle("Pedestal efficiency");
      getObjectsManager()->startPublishing(mHist1D[H1DPedestalEfficiencyM2 + mod]);
    } else {
      mHist1D[H1DPedestalEfficiencyM2 + mod]->Reset();
    }
  }

  //2D Histos
  if (!mHist2D[H2DErrorType]) {
    mHist2D[H2DErrorType] = new TH2F("ErrorType", "ErrorType", 50, 0, 50, 5, 0, 5);
    mHist2D[H2DErrorType]->SetStats(0);
    getObjectsManager()->startPublishing(mHist2D[H2DErrorType]);
  } else {
    mHist2D[H2DErrorType]->Reset();
  }

  //per-module histos
  int nPadsX = mCPVGeometry.kNumberOfCPVPadsPhi;
  int nPadsZ = mCPVGeometry.kNumberOfCPVPadsZ;

  for (int mod = 0; mod < kNModules; mod++) {
    if (!mHist2D[H2DDigitMapM2 + mod]) {
      mHist2D[H2DDigitMapM2 + mod] =
        new TH2F(
          Form("DigitMapM%d", 2 + mod),
          Form("Digit Map in M%d", mod + 2),
          nPadsX, -0.5, nPadsX - 0.5,
          nPadsZ, -0.5, nPadsZ - 0.5);
      mHist2D[H2DDigitMapM2 + mod]->GetXaxis()->SetTitle("x, pad");
      mHist2D[H2DDigitMapM2 + mod]->GetYaxis()->SetTitle("z, pad");
      mHist2D[H2DDigitMapM2 + mod]->SetStats(0);
      getObjectsManager()->startPublishing(mHist2D[H2DDigitMapM2 + mod]);
    } else {
      mHist2D[H2DDigitMapM2 + mod]->Reset();
    }

    if (!mHist2D[H2DPedestalValueMapM2 + mod]) {
      mHist2D[H2DPedestalValueMapM2 + mod] =
        new TH2F(
          Form("PedestalValueMapM%d", 2 + mod),
          Form("PedestalValue Map in M%d", mod + 2),
          nPadsX, -0.5, nPadsX - 0.5,
          nPadsZ, -0.5, nPadsZ - 0.5);
      mHist2D[H2DPedestalValueMapM2 + mod]->GetXaxis()->SetTitle("x, pad");
      mHist2D[H2DPedestalValueMapM2 + mod]->GetYaxis()->SetTitle("z, pad");
      mHist2D[H2DPedestalValueMapM2 + mod]->SetStats(0);
      getObjectsManager()->startPublishing(mHist2D[H2DPedestalValueMapM2 + mod]);
    } else {
      mHist2D[H2DPedestalValueMapM2 + mod]->Reset();
    }

    if (!mHist2D[H2DPedestalSigmaMapM2 + mod]) {
      mHist2D[H2DPedestalSigmaMapM2 + mod] =
        new TH2F(
          Form("PedestalSigmaMapM%d", 2 + mod),
          Form("PedestalSigma Map in M%d", mod + 2),
          nPadsX, -0.5, nPadsX - 0.5,
          nPadsZ, -0.5, nPadsZ - 0.5);
      mHist2D[H2DPedestalSigmaMapM2 + mod]->GetXaxis()->SetTitle("x, pad");
      mHist2D[H2DPedestalSigmaMapM2 + mod]->GetYaxis()->SetTitle("z, pad");
      mHist2D[H2DPedestalSigmaMapM2 + mod]->SetStats(0);
      getObjectsManager()->startPublishing(mHist2D[H2DPedestalSigmaMapM2 + mod]);
    } else {
      mHist2D[H2DPedestalSigmaMapM2 + mod]->Reset();
    }

    if (!mHist2D[H2DPedestalEfficiencyMapM2 + mod]) {
      mHist2D[H2DPedestalEfficiencyMapM2 + mod] =
        new TH2F(
          Form("PedestalEfficiencyMapM%d", 2 + mod),
          Form("Pedestal Efficiency Map in M%d", mod + 2),
          nPadsX, -0.5, nPadsX - 0.5,
          nPadsZ, -0.5, nPadsZ - 0.5);
      mHist2D[H2DPedestalEfficiencyMapM2 + mod]->GetXaxis()->SetTitle("x, pad");
      mHist2D[H2DPedestalEfficiencyMapM2 + mod]->GetYaxis()->SetTitle("z, pad");
      mHist2D[H2DPedestalEfficiencyMapM2 + mod]->SetStats(0);
      getObjectsManager()->startPublishing(mHist2D[H2DPedestalEfficiencyMapM2 + mod]);
    } else {
      mHist2D[H2DPedestalEfficiencyMapM2 + mod]->Reset();
    }
    if (!mHist2D[H2DPedestalNPeaksMapM2 + mod]) {
      mHist2D[H2DPedestalNPeaksMapM2 + mod] =
        new TH2F(
          Form("PedestalNPeaksMapM%d", 2 + mod),
          Form("Number of pedestal peaks Map in M%d", mod + 2),
          nPadsX, -0.5, nPadsX - 0.5, nPadsZ, -0.5, nPadsZ - 0.5);
      mHist2D[H2DPedestalNPeaksMapM2 + mod]->GetXaxis()->SetTitle("x, pad");
      mHist2D[H2DPedestalNPeaksMapM2 + mod]->GetYaxis()->SetTitle("z, pad");
      mHist2D[H2DPedestalNPeaksMapM2 + mod]->SetStats(0);
      getObjectsManager()->startPublishing(mHist2D[H2DPedestalNPeaksMapM2 + mod]);
    } else {
      mHist2D[H2DPedestalNPeaksMapM2 + mod]->Reset();
    }
  }
}

void PedestalTask::fillHistograms()
{
  // count pedestals and update MOs
  float pedestalValue, pedestalSigma, pedestalEfficiency;
  short relId[3];
  TF1* functionGaus = new TF1("functionGaus", "gaus", 0., 4095.);
  TSpectrum* peakSearcher = new TSpectrum(5); //find up to 5 pedestal peaks
  int numberOfPeaks;                          //number of pedestal peaks in channel. Normaly it's 1, otherwise channel is bad
  double *xPeaks, yPeaks[5];                  // arrays of x-position of the peaks and their heights

  //first, reset pedestal histograms
  for (int mod = 0; mod < 3; mod++) {
    mHist2D[H2DPedestalNPeaksMapM2 + mod]->Reset();
    mHist2D[H2DPedestalValueMapM2 + mod]->Reset();
    mHist2D[H2DPedestalSigmaMapM2 + mod]->Reset();
    mHist2D[H2DPedestalEfficiencyMapM2 + mod]->Reset();
    mHist1D[H1DPedestalValueM2 + mod]->Reset();
    mHist1D[H1DPedestalSigmaM2 + mod]->Reset();
    mHist1D[H1DPedestalEfficiencyM2 + mod]->Reset();
  }

  //then fill them with actual values
  for (int channel = 0; channel < kNChannels; channel++) {
    if (!mHistAmplitudes[channel]) {
      ILOG(Error, Devel) << "fillHistograms() : histo mHistAmplitudes[" << channel
                         << "] does not exist! Something is going wrong." << ENDM;
      continue;
    }
    if (!mIsUpdatedAmplitude[channel])
      continue; //no data in channel, skipping it

    if (channel % 1000 == 0) {
      ILOG(Info, Devel) << "fillHistograms(): Start to search peaks in channel " << channel << ENDM;
      LOG(INFO) << "fillHistograms(): Start to search peaks in channel " << channel;
    }

    numberOfPeaks = peakSearcher->Search(mHistAmplitudes[channel], 10., "nobackground", 0.2);
    xPeaks = peakSearcher->GetPositionX();

    if (numberOfPeaks == 1) { // only 1 peak, fit spectrum with gaus
      yPeaks[0] = mHistAmplitudes[channel]
                    ->GetBinContent(mHistAmplitudes[channel]->GetXaxis()->FindBin(xPeaks[0]));
      functionGaus->SetParameters(yPeaks[0], xPeaks[0], 2.);
      mHistAmplitudes[channel]->Fit(functionGaus, "WWQ", "", xPeaks[0] - 20., xPeaks[0] + 20.);
      pedestalValue = functionGaus->GetParameter(1);
      pedestalSigma = functionGaus->GetParameter(2);
      //stop publish this amplitude as it's OK (that fails due to some unclear reason)
      //if (getObjectsManager()->isBeingPublished(mHistAmplitudes[channel]->GetName())){
      //getObjectsManager()->stopPublishing(mHistAmplitudes[channel]->GetName());
      //}
    } else {
      if (numberOfPeaks > 1) { // >1 peaks, no fit. Just use mean and stddev as ped value & sigma
        pedestalValue = mHistAmplitudes[channel]->GetMean();
        if (pedestalValue > 0)
          pedestalValue = -pedestalValue; //let it be negative so we can know it's bad later
        pedestalSigma = mHistAmplitudes[channel]->GetStdDev();
        //let's publish this bad amplitude
        if (!getObjectsManager()->isBeingPublished(mHistAmplitudes[channel]->GetName())) {
          getObjectsManager()->startPublishing(mHistAmplitudes[channel]);
        }
      } else { //numberOfPeaks < 1 - what is it?
        //no peaks found((( OK let's show the spectrum to the world...
        if (!getObjectsManager()->isBeingPublished(mHistAmplitudes[channel]->GetName())) {
          getObjectsManager()->startPublishing(mHistAmplitudes[channel]);
        }
        continue;
      }
    }

    pedestalEfficiency = mHistAmplitudes[channel]->GetEntries() / mNEventsTotal;
    mCPVGeometry.absToRelNumbering(channel, relId);
    mHist2D[H2DPedestalValueMapM2 + relId[0] - 2]
      ->SetBinContent(relId[1] + 1, relId[2] + 1, pedestalValue);
    mHist2D[H2DPedestalSigmaMapM2 + relId[0] - 2]
      ->SetBinContent(relId[1] + 1, relId[2] + 1, pedestalSigma);
    mHist2D[H2DPedestalEfficiencyMapM2 + relId[0] - 2]
      ->SetBinContent(relId[1] + 1, relId[2] + 1, pedestalEfficiency);
    mHist2D[H2DPedestalNPeaksMapM2 + relId[0] - 2]
      ->SetBinContent(relId[1] + 1, relId[2] + 1, numberOfPeaks);

    mHist1D[H1DPedestalValueM2 + relId[0] - 2]->Fill(pedestalValue);
    mHist1D[H1DPedestalSigmaM2 + relId[0] - 2]->Fill(pedestalSigma);
    mHist1D[H1DPedestalEfficiencyM2 + relId[0] - 2]->Fill(pedestalEfficiency);
  }

  //show some info to developer
  ILOG(Info, Devel) << "fillHistograms() : at this time, N events = " << mNEventsTotal << ENDM;
  LOG(INFO) << "fillPedestals() : I finished filling of histograms";
}

void PedestalTask::resetHistograms()
{
  // clean all histograms
  ILOG(Info, Support) << "Resetting amplitude histograms" << ENDM;
  for (int i = 0; i < kNChannels; i++) {
    mHistAmplitudes[i]->Reset();
    mIsUpdatedAmplitude[i] = false;
  }

  ILOG(Info, Support) << "Resetting the 1D Histograms" << ENDM;
  for (int itHist1D = H1DInputPayloadSize; itHist1D < kNHist1D; itHist1D++) {
    mHist1D[itHist1D]->Reset();
  }

  ILOG(Info, Support) << "Resetting the 2D Histograms" << ENDM;
  for (int itHist2D = H2DErrorType; itHist2D < kNHist2D; itHist2D++) {
    mHist2D[itHist2D]->Reset();
  }
}

} // namespace o2::quality_control_modules::cpv
