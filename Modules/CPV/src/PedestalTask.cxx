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
  for (int i = kNHist1D; i--;) {
    if (mHist1D[i]) {
      mHist1D[i]->Delete();
      mHist1D[i] = nullptr;
    }
  }
  for (int i = kNHist2D; i--;) {
    if (mHist2D[i]) {
      mHist2D[i]->Delete();
      mHist2D[i] = nullptr;
    }
  }
  for (int i = 0; i<kNChannels; i++){
    if(mHistAmplitudes[i]){
      mHistAmplitudes[i]->Delete();
      mHistAmplitudes[i] = nullptr;
    }
  }
}

void PedestalTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize PedestalTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - myOwnKey: " << param->second << ENDM;
  }
  initHistograms();
  mNEvents = 0;
}

void PedestalTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity" << activity.mId << ENDM;
  resetHistograms();  
}

void PedestalTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
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
  for (const auto& digit : digits){
    mHist1D[H1DDigitIds]->Fill(digit.getAbsId());
    short relId[3];
    mCPVGeometry.absToRelNumbering(digit.getAbsId(),relId);
    //reminder: relId[3]={Module, phi col, z row} where Module=2..4, phi col=0..127, z row=0..59
    mHist2D[H2DDigitMapM2 + relId[0] - 2]->Fill(relId[1], relId[2]);
    mHistAmplitudes[digit.getAbsId()]->Fill(digit.getAmplitude());
  } 

  auto digitsTR = ctx.inputs().get<gsl::span<o2::cpv::TriggerRecord>>("dtrigrec");
  //mNEvents += digitsTR.size();//number of events in the current input
  for (const auto& trigRecord : digitsTR) {
    ILOG(Info, Devel) << " monitorData() : trigger record #" << mNEvents
		      << " contains "<<trigRecord.getNumberOfObjects()<<" objects."<< ENDM;
    if(trigRecord.getNumberOfObjects() > 0) mNEvents++;//at least 1 digit in this trigger record
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
  ILOG(Info, Support) << "endOfCycle. I call fillHistograms()" << ENDM;
  fillHistograms();
}

void PedestalTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void PedestalTask::reset()
{
  // clean all the monitor objects here
  ILOG(Info, Support) << "Resetting PedestalTask" << ENDM;
  resetHistograms();
  mNEvents = 0;
}

void PedestalTask::initHistograms()
{
  //create monitoring histograms (or reset, if they already exists)
  for (int i = 0; i < kNChannels; i++){
    if (!mHistAmplitudes[i]) {
      mHistAmplitudes[i] = new TH1F(Form("HistAmplitude%d", i), Form("HistAmplitude%d", i), 4096, 0., 4096.);
      if(i % 1000 == 0) getObjectsManager()->startPublishing(mHistAmplitudes[i]);
      
    } else {
      mHistAmplitudes[i]->Reset();
    }  
  }
  
  
  //1D Histos
  if (!mHist1D[H1DInputPayloadSize]){
    mHist1D[H1DInputPayloadSize] = new TH1F("InputPayloadSize", "Input Payload Size", 30000, 0, 30000000);
    getObjectsManager()->startPublishing(mHist1D[H1DInputPayloadSize]);
  } else {
    mHist1D[H1DInputPayloadSize]->Reset();
  }
  
  if (!mHist1D[H1DNInputs]){
    mHist1D[H1DNInputs] = new TH1F("NInputs", "Number of inputs", 10, -0.5, 9.5);
    getObjectsManager()->startPublishing(mHist1D[H1DNInputs]);
  } else {
    mHist1D[H1DNInputs]->Reset();
  }
  
  if (!mHist1D[H1DNValidInputs]){
    mHist1D[H1DNValidInputs] = new TH1F("NValidInputs", "Number of valid inputs", 10, -0.5, 9.5);
    getObjectsManager()->startPublishing(mHist1D[H1DNValidInputs]);
  } else {
    mHist1D[H1DNValidInputs]->Reset();
  }
  
  if (!mHist1D[H1DNDigitsPerInput]){
    mHist1D[H1DNDigitsPerInput] = new TH1F("NDigitsPerInput", "Number of digits per input", 30000, 0, 300000);
    getObjectsManager()->startPublishing(mHist1D[H1DNDigitsPerInput]);
  } else {
    mHist1D[H1DNDigitsPerInput]->Reset();
  }
  
  if (!mHist1D[H1DDigitIds]){
    mHist1D[H1DDigitIds] = new TH1F("DigitIds", "Digit Ids", 30000, -0.5, 29999.5);
    getObjectsManager()->startPublishing(mHist1D[H1DDigitIds]);
  } else {
    mHist1D[H1DDigitIds]->Reset();
  }

  for (int mod = 0; mod < kNModules; mod++){
    if (!mHist1D[H1DPedestalValueM2 + mod]){
      mHist1D[H1DPedestalValueM2 + mod] = new TH1F(Form("PedestalValueM%d", mod + 2), Form("Pedestal value distribution M%d", mod + 2), 512, 0, 512);
      getObjectsManager()->startPublishing(mHist1D[H1DPedestalValueM2 + mod]);
    } else {
      mHist1D[H1DPedestalValueM2 + mod]->Reset();
    }
	
    if (!mHist1D[H1DPedestalSigmaM2 + mod]){
      mHist1D[H1DPedestalSigmaM2 + mod] = new TH1F(Form("PedestalSigmaM%d", mod + 2), Form("Pedestal sigma distribution M%d", mod + 2), 512, 0, 512);
      getObjectsManager()->startPublishing(mHist1D[H1DPedestalSigmaM2 + mod]);
    } else {
      mHist1D[H1DPedestalSigmaM2 + mod]->Reset();
    }
	
    if (!mHist1D[H1DPedestalEfficiencyM2 + mod]){
      mHist1D[H1DPedestalEfficiencyM2 + mod] = new TH1F(Form("PedestalEfficiencyM%d", mod + 2), Form("Pedestal efficiency distribution M%d", mod + 2), 1000, 0., 10.);
      getObjectsManager()->startPublishing(mHist1D[H1DPedestalEfficiencyM2 + mod]);
    } else {
      mHist1D[H1DPedestalEfficiencyM2 + mod]->Reset();
    }
  }


  
  //2D Histos
  if (!mHist2D[H2DErrorType]){
    mHist2D[H2DErrorType] = new TH2F("ErrorType", "ErrorType", 50, 0, 50, 5, 0, 5);
    getObjectsManager()->startPublishing(mHist2D[H2DErrorType]);
  } else {
    mHist2D[H2DErrorType]->Reset();
  }

  //per-module histos
  int nPadsX = mCPVGeometry.kNumberOfCPVPadsPhi;
  int nPadsZ = mCPVGeometry.kNumberOfCPVPadsZ;
  
  for (int mod = 0; mod < kNModules; mod++){
    if (!mHist2D[H2DDigitMapM2 + mod]){
      mHist2D[H2DDigitMapM2 + mod] = new TH2F(Form("DigitMapM%d", 2 + mod), Form("Digit Map in M%d", mod + 2), nPadsX, -0.5, nPadsX - 0.5, nPadsZ, -0.5, nPadsZ - 0.5);
      getObjectsManager()->startPublishing(mHist2D[H2DDigitMapM2 + mod]);
    } else {
      mHist2D[H2DDigitMapM2 + mod]->Reset();
    }
    
    if (!mHist2D[H2DPedestalValueMapM2 + mod]){
      mHist2D[H2DPedestalValueMapM2 + mod] = new TH2F(Form("PedestalValueMapM%d", 2 + mod), Form("PedestalValue Map in M%d", mod + 2), nPadsX, -0.5, nPadsX - 0.5, nPadsZ, -0.5, nPadsZ - 0.5);
      getObjectsManager()->startPublishing(mHist2D[H2DPedestalValueMapM2 + mod]);
    } else {
      mHist2D[H2DPedestalValueMapM2 + mod]->Reset();
    }

    if (!mHist2D[H2DPedestalSigmaMapM2 + mod]){
      mHist2D[H2DPedestalSigmaMapM2 + mod] = new TH2F(Form("PedestalSigmaMapM%d", 2 + mod), Form("PedestalSigma Map in M%d", mod + 2), nPadsX, -0.5, nPadsX - 0.5, nPadsZ, -0.5, nPadsZ - 0.5);
      getObjectsManager()->startPublishing(mHist2D[H2DPedestalSigmaMapM2 + mod]);
    } else {
      mHist2D[H2DPedestalSigmaMapM2 + mod]->Reset();
    }

    if (!mHist2D[H2DPedestalEfficiencyMapM2 + mod]){
      mHist2D[H2DPedestalEfficiencyMapM2 + mod] = new TH2F(Form("PedestalEfficiencyMapM%d", 2 + mod), Form("Pedestal Efficiency Map in M%d", mod + 2), nPadsX, -0.5, nPadsX - 0.5, nPadsZ, -0.5, nPadsZ - 0.5);
      getObjectsManager()->startPublishing(mHist2D[H2DPedestalEfficiencyMapM2 + mod]);
    } else {
      mHist2D[H2DPedestalEfficiencyMapM2 + mod]->Reset();
    }
    if (!mHist2D[H2DPedestalNPeaksMapM2 + mod]){
      mHist2D[H2DPedestalNPeaksMapM2 + mod] = new TH2F(Form("PedestalNPeaksMapM%d", 2 + mod), Form("Number of pedestal peaks Map in M%d", mod + 2), nPadsX, -0.5, nPadsX - 0.5, nPadsZ, -0.5, nPadsZ - 0.5);
      getObjectsManager()->startPublishing(mHist2D[H2DPedestalNPeaksMapM2 + mod]);
    } else {
      mHist2D[H2DPedestalNPeaksMapM2 + mod]->Reset();
    }

    
  }


  
}

void PedestalTask::fillHistograms()
{
  // count pedestals and update MOs
  static float pedestalValue, pedestalSigma, pedestalEfficiency;
  static short relId[3];
  TF1* functionGaus = new TF1("functionGaus", "gaus", 0., 4095.);
  TSpectrum *peakSearcher = new TSpectrum(5);//find up to 5 pedestal peaks
  int numberOfPeaks; //number of pedestal peaks in channel. Normaly it's 1, otherwise channel is bad
  double *xPeaks, yPeaks[5]; // arrays of x-position of the peaks and their heights
  
  for (int channel = 0; channel < kNChannels; channel++){
    if (mHistAmplitudes[channel]->GetEntries() < 1) continue; //no data in channel, skipping it
    //first, make just estimation of pedestal value and sigma in whole histo range
    //pedestalValue = mHistAmplitudes[channel]->GetMean();
    //pedestalSigma = mHistAmplitudes[channel]->GetStdDev();
    //then calc pedestal value and sigma in a zone of interest
    //(i.e. exclude events which are far from pedestal value)
    //mHistAmplitudes[channel]->GetXaxis()->SetRangeUser(pedestalValue - 5. * pedestalSigma,
    //						       pedestalValue + 5. * pedestalSigma);
    //pedestalValue = mHistAmplitudes[channel]->GetMean();
    //pedestalSigma = mHistAmplitudes[channel]->GetStdDev();
    //mHistAmplitudes[channel]->Fit(functionGaus, "WWQ", "", pedestalValue - 5. * pedestalSigma, 
    //				                    pedestalValue + 5. * pedestalSigma);
    //if(functionGaus->GetParameter(1) > 0){//in some cases it's not because of several peaks in the spectrum
    //  pedestalValue = functionGaus->GetParameter(1);
    //  pedestalSigma = functionGaus->GetParameter(2);
    //}

    ILOG(Info, Support) << "fillHistograms(): Start to search peaks in channel " << channel << ENDM;
    
    numberOfPeaks = peakSearcher->Search(mHistAmplitudes[channel], 2, "nobackground", 0.05);
    xPeaks = peakSearcher->GetPositionX();
    
    if(numberOfPeaks == 1) {// only 1 peak, fit spectrum with gaus
      yPeaks[0] = mHistAmplitudes[channel]->GetBinContent(mHistAmplitudes[channel]->GetXaxis()->FindBin(xPeaks[0]));
      functionGaus->SetParameters(yPeaks[0], xPeaks[0], 2.);
      mHistAmplitudes[channel]->Fit(functionGaus);
      pedestalValue = functionGaus->GetParameter(1);
      pedestalSigma = functionGaus->GetParameter(2);
    } else
      if (numberOfPeaks > 1){// >1 peaks, no fit. Just use mean and stddev as ped value & sigma
	pedestalValue = mHistAmplitudes[channel]->GetMean();
	pedestalSigma = mHistAmplitudes[channel]->GetStdDev();
	//let's publish this bad amplitude
	getObjectsManager()->startPublishing(mHistAmplitudes[channel]);
    }
    
    pedestalEfficiency = mHistAmplitudes[channel]->GetEntries()/mNEvents;
    mCPVGeometry.absToRelNumbering(channel,relId);
    mHist2D[H2DPedestalValueMapM2 + relId[0] - 2]->SetBinContent(relId[1] + 1, relId[2] + 1, pedestalValue);
    mHist2D[H2DPedestalSigmaMapM2 + relId[0] - 2]->SetBinContent(relId[1] + 1, relId[2] + 1, pedestalSigma);
    mHist2D[H2DPedestalEfficiencyMapM2 + relId[0] - 2]->SetBinContent(relId[1] + 1, relId[2] + 1, pedestalEfficiency);
    mHist2D[H2DPedestalNPeaksMapM2 + relId[0] - 2]->SetBinContent(relId[1] + 1, relId[2] + 1, numberOfPeaks);

    mHist1D[H1DPedestalValueM2 + relId[0] - 2]->Fill(pedestalValue);
    mHist1D[H1DPedestalSigmaM2 + relId[0] - 2]->Fill(pedestalSigma);
    mHist1D[H1DPedestalEfficiencyM2 + relId[0] - 2]->Fill(pedestalEfficiency);

  }

  //show some info to developer
  ILOG(Info, Devel) << "fillHistograms() : N events = " << mNEvents << ENDM;
}
  
void PedestalTask::resetHistograms()
{
  // clean all histograms
  ILOG(Info, Support) << "Resetting amplitude histograms" << ENDM;
  for (int i = 0; i < kNChannels; i++){
    mHistAmplitudes[i]->Reset();
  }

  ILOG(Info, Support) << "Resetting the 1D Histograms" << ENDM;
  for (int itHist1D = H1DInputPayloadSize; itHist1D < kNHist1D; itHist1D++){
    mHist1D[itHist1D]->Reset();
  }
  
  ILOG(Info, Support) << "Resetting the 2D Histograms" << ENDM;
  for (int itHist2D = H2DErrorType; itHist2D < kNHist2D; itHist2D++){
    mHist2D[itHist2D]->Reset();
  }
}
  
} // namespace o2::quality_control_modules::cpv
