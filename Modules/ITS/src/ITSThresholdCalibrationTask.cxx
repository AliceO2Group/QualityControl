// Copyright CERN and copyright holders of ALICE O2. This software is
// // distributed under the terms of the GNU General Public License v3 (GPL
// // Version 3), copied verbatim in the file "COPYING".
// //
// // See http://alice-o2.web.cern.ch/license for full licensing information.
// //
// // In applying this license CERN does not waive the privileges and immunities
// // granted to it by virtue of its status as an Intergovernmental Organization
// // or submit itself to any jurisdiction.
//

///
/// \file   ITSThresholdCalibrationTask.cxx
/// \author Artem Isakov
///

#include "QualityControl/QcInfoLogger.h"
#include "ITS/ITSThresholdCalibrationTask.h"
#include <DataFormatsITS/TrackITS.h>
#include <DataFormatsITSMFT/ROFRecord.h>
#include <Framework/InputRecord.h>
#include "ReconstructionDataFormats/Vertex.h"
#include "ReconstructionDataFormats/PrimaryVertex.h"
#include "TLine.h"
#include "TLatex.h"
#include <Framework/DataSpecUtils.h>

using namespace o2::itsmft;
using namespace o2::its;

namespace o2::quality_control_modules::its
{

ITSThresholdCalibrationTask::ITSThresholdCalibrationTask() : TaskInterface()
{
}

ITSThresholdCalibrationTask::~ITSThresholdCalibrationTask()
{
  delete hSuccessRate;

  for (int iScan = 0; iScan < 3; iScan++) {
    for (int iLayer = 0; iLayer < 7; iLayer++) {
      delete hCalibrationLayer[iLayer][iScan];
      delete hCalibrationRMSLayer[iLayer][iScan];
      if (iScan == 2) {
        delete hCalibrationThrNoiseLayer[iLayer];
        delete hCalibrationThrNoiseRMSLayer[iLayer];
      }
    }
    for (int iBarrel = 0; iBarrel < 3; iBarrel++) {
      delete hCalibrationChipSum[iScan][iBarrel];
      delete hCalibrationChipAverage[iScan][iBarrel];
      delete hCalibrationChipCounts[iScan][iBarrel];

      delete hCalibrationRMSChipSum[iScan][iBarrel];
      delete hCalibrationRMSChipAverage[iScan][iBarrel];
    }
  }

  for (int iBarrel = 0; iBarrel < 3; iBarrel++) {

    delete hCalibrationThrNoiseChipSum[iBarrel];
    delete hCalibrationThrNoiseChipAverage[iBarrel];

    delete hCalibrationThrNoiseRMSChipSum[iBarrel];
    delete hCalibrationThrNoiseRMSChipAverage[iBarrel];
  }
}

void ITSThresholdCalibrationTask::initialize(o2::framework::InitContext& /*ctx*/)
{

  ILOG(Info, Support) << "initialize ITSThresholdCalibrationTask" << ENDM;

  mRunNumberPath = mCustomParameters["runNumberPath"];

  createAllHistos();
  publishHistos();
}

void ITSThresholdCalibrationTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;
}

void ITSThresholdCalibrationTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void ITSThresholdCalibrationTask::monitorData(o2::framework::ProcessingContext& ctx)
{

  ILOG(Info, Support) << "START DOING QC General" << ENDM;
  const auto tunString = ctx.inputs().get<gsl::span<char>>("tunestring");
  const auto runType = ctx.inputs().get<short int>("runtype");
  const auto scanType = ctx.inputs().get<char>("scantype");

  string inString(tunString.begin(), tunString.end());

  Int_t iScan;
  Double_t CalibrationValue;
  if (scanType == 'V')
    iScan = 0;
  else if (scanType == 'I')
    iScan = 1;
  else if (scanType == 'T')
    iScan = 2;

  auto splitRes = split(inString, "Stave:");

  for (auto StaveStr : splitRes) {
    if (StaveStr.size() > 0) {
      CalibrationResStruct result = CalibrationParser(StaveStr);

      int currentStave = StaveBoundary[result.Layer] + result.Stave + 1;
      int currentChip = result.ChipID + 1;
      int iBarrel = getBarrel(result.Layer);

      if (scanType == 'V') {
        CalibrationValue = result.VCASN;
      } else if (scanType == 'I') {
        CalibrationValue = result.ITHR;
      }
      if (scanType == 'T') {
        CalibrationValue = result.THR;
        hCalibrationThrNoiseChipSum[iBarrel]->SetBinContent(currentChip, currentStave, result.Noise + hCalibrationThrNoiseChipSum[iBarrel]->GetBinContent(currentChip, currentStave));
        hCalibrationThrNoiseRMSChipSum[iBarrel]->SetBinContent(currentChip, currentStave, result.NoiseRMS + hCalibrationThrNoiseRMSChipSum[iBarrel]->GetBinContent(currentChip, currentStave));
      }

      hCalibrationChipSum[iScan][iBarrel]->SetBinContent(currentChip, currentStave, CalibrationValue + hCalibrationChipSum[iScan][iBarrel]->GetBinContent(currentChip, currentStave));
      hCalibrationRMSChipSum[iScan][iBarrel]->SetBinContent(currentChip, currentStave, result.RMS + hCalibrationRMSChipSum[iScan][iBarrel]->GetBinContent(currentChip, currentStave));

      hCalibrationChipCounts[iScan][iBarrel]->SetBinContent(currentChip, currentStave, hCalibrationChipCounts[iScan][iBarrel]->GetBinContent(currentChip, currentStave) + 1);

      if (result.status == 1)
        SuccessStatus[result.Layer]++;
      TotalStatus[result.Layer]++;
    }
  }

  calculateAverages(iScan);

  for (int iLayer; iLayer < 7; iLayer++) {
    if (TotalStatus[iLayer] > 0)
      hSuccessRate->SetBinContent(iLayer + 1, SuccessStatus[iLayer] / TotalStatus[iLayer]);
  }
}

void ITSThresholdCalibrationTask::calculateAverages(Int_t iScan)
{

  for (int iBarrel = 0; iBarrel < 3; iBarrel++) { // TH2 Plots per Chip
    hCalibrationChipAverage[iScan][iBarrel]->Divide(hCalibrationChipSum[iScan][iBarrel], hCalibrationChipCounts[iScan][iBarrel]);
    hCalibrationRMSChipAverage[iScan][iBarrel]->Divide(hCalibrationRMSChipSum[iScan][iBarrel], hCalibrationChipCounts[iScan][iBarrel]);

    if (iScan == 2) {
      hCalibrationThrNoiseChipAverage[iBarrel]->Divide(hCalibrationThrNoiseChipSum[iBarrel], hCalibrationChipCounts[iScan][iBarrel]);
      hCalibrationThrNoiseRMSChipAverage[iBarrel]->Divide(hCalibrationThrNoiseRMSChipSum[iBarrel], hCalibrationChipCounts[iScan][iBarrel]);
    }
  }

  for (int iLayer = 0; iLayer < 7; iLayer++) { // TH1 plots per layer

    Int_t iBarrel = getBarrel(iLayer);

    for (int iStave = 1; iStave <= NStaves[iLayer]; iStave++)
      for (int iChip = 1; iChip <= nChipsPerStave[iLayer]; iChip++) {
        hCalibrationLayer[iLayer][iScan]->Fill(hCalibrationChipAverage[iScan][iBarrel]->GetBinContent(iChip, StaveBoundary[iLayer] + iStave));
        hCalibrationRMSLayer[iLayer][iScan]->Fill(hCalibrationRMSChipAverage[iScan][iBarrel]->GetBinContent(iChip, StaveBoundary[iLayer] + iStave));

        if (iScan == 2) {
          hCalibrationThrNoiseLayer[iLayer]->Fill(hCalibrationThrNoiseChipAverage[iBarrel]->GetBinContent(iChip, StaveBoundary[iLayer] + iStave));
          hCalibrationThrNoiseRMSLayer[iLayer]->Fill(hCalibrationThrNoiseChipAverage[iBarrel]->GetBinContent(iChip, StaveBoundary[iLayer] + iStave));
        }
      }
  }
}

ITSThresholdCalibrationTask::CalibrationResStruct ITSThresholdCalibrationTask::CalibrationParser(string input)
{
  CalibrationResStruct result;
  auto StaveINFO = split(input, ",");

  for (string info : StaveINFO) {
    if (info.size() == 0)
      continue;

    if (info.substr(0, 1) == "L") {
      result.Layer = std::stod(info.substr(1, 1));
      result.Stave = std::stod(info.substr(3, 2));
    } else {

      std::string name = split(info, ":")[0];
      string data = split(info, ":")[1];
      if (name == "Hs_pos") {
        result.Hs = std::stod(data);
      } else if (name == "Hic_Pos") {
        result.HIC = std::stod(data);
      } else if (name == "ChipID") {
        result.ChipID = std::stod(data);
      } else if (name == "VCASN") {
        result.VCASN = std::stof(data);
      } else if (name == "Rms") {
        result.RMS = std::stof(data);
      } else if (name == "Status") {
        result.status = std::stod(data);
      } else if (name == "Noise") {
        result.Noise = std::stof(data);
      } else if (name == "NoiseRMS") {
        result.NoiseRMS = std::stof(data);
      } else if (name == "ITHR") {
        result.ITHR = std::stof(data);
      } else if (name == "THR") {
        result.THR = std::stof(data);
      }
    }
  }
  return result;
}

void ITSThresholdCalibrationTask::endOfCycle()
{

  std::ifstream runNumberFile(mRunNumberPath.c_str());
  if (runNumberFile) {

    std::string runNumber;
    runNumberFile >> runNumber;
    if (runNumber != mRunNumber) {
      for (unsigned int iObj = 0; iObj < mPublishedObjects.size(); iObj++)
        getObjectsManager()->addMetadata(mPublishedObjects.at(iObj)->GetName(), "Run", runNumber);
      mRunNumber = runNumber;
    }
    ILOG(Info, Support) << "endOfCycle" << ENDM;
  }
}

void ITSThresholdCalibrationTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void ITSThresholdCalibrationTask::reset()
{
  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  hSuccessRate->Reset();

  for (int iScan = 0; iScan < 3; iScan++) {
    for (int iLayer = 0; iLayer < 7; iLayer++) {
      hCalibrationLayer[iLayer][iScan]->Reset();
      hCalibrationRMSLayer[iLayer][iScan]->Reset();

      if (iScan == 2) {
        hCalibrationThrNoiseLayer[iLayer]->Reset();
        hCalibrationThrNoiseRMSLayer[iLayer]->Reset();
      }
    }
    for (int iBarrel = 0; iBarrel < 3; iBarrel++) {
      hCalibrationChipSum[iScan][iBarrel]->Reset();
      hCalibrationChipAverage[iScan][iBarrel]->Reset();
      hCalibrationChipCounts[iScan][iBarrel]->Reset();

      hCalibrationRMSChipSum[iScan][iBarrel]->Reset();
      hCalibrationRMSChipAverage[iScan][iBarrel]->Reset();
    }
  }

  for (int iBarrel = 0; iBarrel < 3; iBarrel++) {

    hCalibrationThrNoiseChipSum[iBarrel]->Reset();
    hCalibrationThrNoiseChipAverage[iBarrel]->Reset();

    hCalibrationThrNoiseRMSChipSum[iBarrel]->Reset();
    hCalibrationThrNoiseRMSChipAverage[iBarrel]->Reset();
  }
}

void ITSThresholdCalibrationTask::createAllHistos()
{
  /*
    TString sScanTypes[3]= {"VCASN","ITHR","THR"};
    TString sBarrelType[3]= {"IB","MB","OB"};
    Int_t nChips[3]= {9,112, 196};
    Int_t nStaves[3]={48,54,90};
    Int_t nXmax[3]= {80,100,400};
    TString sXtitles[3]={"DAC","DAC","e"};
  */
  for (int iScan = 0; iScan < 3; iScan++) {

    for (int iLayer = 0; iLayer < 7; iLayer++) {
      hCalibrationLayer[iLayer][iScan] = new TH1F(Form("%sLayer%d", sScanTypes[iScan].Data(), iLayer), Form("%sLayer%d", sScanTypes[iScan].Data(), iLayer), nXmax[iScan], -0.5, nXmax[iScan] - 0.5);
      hCalibrationLayer[iLayer][iScan]->SetStats(0);
      formatAxes(hCalibrationLayer[iLayer][iScan], Form("%s (%s) ", sScanTypes[iScan].Data(), sXtitles[iScan].Data()), "Chip counts", 1, 1.10);
      addObject(hCalibrationLayer[iLayer][iScan]);

      hCalibrationRMSLayer[iLayer][iScan] = new TH1F(Form("%sRMSLayer%d", sScanTypes[iScan].Data(), iLayer), Form("%sRMSLayer%d", sScanTypes[iScan].Data(), iLayer), 50, -0.5, 9.5);
      hCalibrationRMSLayer[iLayer][iScan]->SetStats(0);
      formatAxes(hCalibrationRMSLayer[iLayer][iScan], Form("RMS (%s)", sXtitles[iScan].Data()), "Chip counts", 1, 1.10);
      addObject(hCalibrationRMSLayer[iLayer][iScan]);
    }

    for (int iBarrel = 0; iBarrel < 3; iBarrel++) {

      hCalibrationChipSum[iScan][iBarrel] = new TH2F(Form("%sChipSum%s", sScanTypes[iScan].Data(), sBarrelType[iBarrel].Data()), Form("%sChipSum%s", sScanTypes[iScan].Data(), sBarrelType[iBarrel].Data()), nChips[iBarrel], -0.5, nChips[iBarrel] - 0.5, nStaves[iBarrel], -0.5, nStaves[iBarrel] - 0.5);
      hCalibrationChipCounts[iScan][iBarrel] = new TH2F(Form("%sChipCounts%s", sScanTypes[iScan].Data(), sBarrelType[iBarrel].Data()), Form("%sChipCounts%s", sScanTypes[iScan].Data(), sBarrelType[iBarrel].Data()), nChips[iBarrel], -0.5, nChips[iBarrel] - 0.5, nStaves[iBarrel], -0.5, nStaves[iBarrel] - 0.5);
      hCalibrationChipAverage[iScan][iBarrel] = new TH2F(Form("%sChipAverage%s", sScanTypes[iScan].Data(), sBarrelType[iBarrel].Data()), Form("Average %s per Chip in %s", sScanTypes[iScan].Data(), sBarrelType[iBarrel].Data()), nChips[iBarrel], -0.5, nChips[iBarrel] - 0.5, nStaves[iBarrel], -0.5, nStaves[iBarrel] - 0.5);
      hCalibrationChipAverage[iScan][iBarrel]->SetStats(0);
      formatAxes(hCalibrationChipAverage[iScan][iBarrel], "Chip", "Stave", 1, 1.10);

      formatLayers(hCalibrationChipAverage[iScan][iBarrel], iBarrel);

      addObject(hCalibrationChipAverage[iScan][iBarrel]);

      hCalibrationRMSChipSum[iScan][iBarrel] = new TH2F(Form("%sRMSChipSum%s", sScanTypes[iScan].Data(), sBarrelType[iBarrel].Data()), Form("%sRMSChipSum%s", sScanTypes[iScan].Data(), sBarrelType[iBarrel].Data()), nChips[iBarrel], -0.5, nChips[iBarrel] - 0.5, nStaves[iBarrel], -0.5, nStaves[iBarrel] - 0.5);
      hCalibrationRMSChipAverage[iScan][iBarrel] = new TH2F(Form("%sRMSChipAverage%s", sScanTypes[iScan].Data(), sBarrelType[iBarrel].Data()), Form("Average %s RMS per Chip in %s", sScanTypes[iScan].Data(), sBarrelType[iBarrel].Data()), nChips[iBarrel], -0.5, nChips[iBarrel] - 0.5, nStaves[iBarrel], -0.5, nStaves[iBarrel] - 0.5);
      hCalibrationRMSChipAverage[iScan][iBarrel]->SetStats(0);
      formatAxes(hCalibrationRMSChipAverage[iScan][iBarrel], "Chip", "Stave", 1, 1.10);
      formatLayers(hCalibrationRMSChipAverage[iScan][iBarrel], iBarrel);
      addObject(hCalibrationRMSChipAverage[iScan][iBarrel]);
    }
  }

  //------------------Noise histograms for THR scan

  for (int iBarrel = 0; iBarrel < 3; iBarrel++) { // TH2 for THR noise plots

    hCalibrationThrNoiseChipSum[iBarrel] = new TH2F(Form("ThrNoiseChipSum%s", sBarrelType[iBarrel].Data()), Form("ThrNoiseChipSum%s", sBarrelType[iBarrel].Data()), nChips[iBarrel], -0.5, nChips[iBarrel] - 0.5, nStaves[iBarrel], -0.5, nStaves[iBarrel] - 0.5);
    hCalibrationThrNoiseChipAverage[iBarrel] = new TH2F(Form("ThrNoiseChipAverage%s", sBarrelType[iBarrel].Data()), Form("Average THR Noise per Chip in %s", sBarrelType[iBarrel].Data()), nChips[iBarrel], -0.5, nChips[iBarrel] - 0.5, nStaves[iBarrel], -0.5, nStaves[iBarrel] - 0.5);
    hCalibrationThrNoiseChipAverage[iBarrel]->SetStats(0);
    formatAxes(hCalibrationThrNoiseChipAverage[iBarrel], "Chip", "Stave", 1, 1.10);
    formatLayers(hCalibrationThrNoiseChipAverage[iBarrel], iBarrel);
    addObject(hCalibrationThrNoiseChipAverage[iBarrel]);

    hCalibrationThrNoiseRMSChipSum[iBarrel] = new TH2F(Form("ThrNoiseRMSChipSum%s", sBarrelType[iBarrel].Data()), Form("ThrNoiseRMSChipSum%s", sBarrelType[iBarrel].Data()), nChips[iBarrel], -0.5, nChips[iBarrel] - 0.5, nStaves[iBarrel], -0.5, nStaves[iBarrel] - 0.5);
    hCalibrationThrNoiseRMSChipAverage[iBarrel] = new TH2F(Form("ThrNoiseRMSChipAverage%s", sBarrelType[iBarrel].Data()), Form("Average THR NoiseRMS per Chip in %s", sBarrelType[iBarrel].Data()), nChips[iBarrel], -0.5, nChips[iBarrel] - 0.5, nStaves[iBarrel], -0.5, nStaves[iBarrel] - 0.5);
    hCalibrationThrNoiseRMSChipAverage[iBarrel]->SetStats(0);
    formatAxes(hCalibrationThrNoiseRMSChipAverage[iBarrel], "Chip", "Stave", 1, 1.10);
    formatLayers(hCalibrationThrNoiseRMSChipAverage[iBarrel], iBarrel);
    addObject(hCalibrationThrNoiseRMSChipAverage[iBarrel]);
  }
  for (int iLayer = 0; iLayer < 7; iLayer++) {
    hCalibrationThrNoiseLayer[iLayer] = new TH1F(Form("ThrNoiseLayer%d", iLayer), Form("ThrNoiseLayer%d", iLayer), 10, -0.5, 9.5);
    hCalibrationThrNoiseLayer[iLayer]->SetStats(0);
    formatAxes(hCalibrationThrNoiseLayer[iLayer], "THR noise (e) ", "Chip counts", 1, 1.10);
    addObject(hCalibrationThrNoiseLayer[iLayer]);

    hCalibrationThrNoiseRMSLayer[iLayer] = new TH1F(Form("ThrNoiseRMSLayer%d", iLayer), Form("ThrNoiseRMSLayer%d", iLayer), 50, -0.5, 9.5);
    hCalibrationThrNoiseRMSLayer[iLayer]->SetStats(0);
    formatAxes(hCalibrationThrNoiseRMSLayer[iLayer], "THR noise RMS (e)", "Chip counts", 1, 1.10);
    addObject(hCalibrationThrNoiseRMSLayer[iLayer]);
  }

  hSuccessRate = new TH1F("SuccessRate", "Success Rate", 7, -0.5, 6.5);
  hSuccessRate->SetStats(0);
  formatAxes(hSuccessRate, "Layer", "Rate", 1, 1.10);
  addObject(hSuccessRate);

  for (int iLayer; iLayer < 7; iLayer++) {
    SuccessStatus[iLayer] = 0;
    TotalStatus[iLayer] = 0;
  }
}
void ITSThresholdCalibrationTask::addObject(TObject* aObject)
{
  if (!aObject) {
    ILOG(Info, Support) << " ERROR: trying to add non-existent object " << ENDM;
    return;
  } else {
    mPublishedObjects.push_back(aObject);
  }
}

void ITSThresholdCalibrationTask::formatLayers(TH2* h, Int_t iBarrel)
{
  Int_t iLayerBegin, iLayerEnd;

  switch (iBarrel) {
    case 0: {
      iLayerBegin = 1;
      iLayerEnd = 3;
      break;
    }
    case 1: {
      iLayerBegin = 4;
      iLayerEnd = 5;
      break;
    }
    case 2: {
      iLayerBegin = 6;
      iLayerEnd = 7;
      break;
    }
  };

  for (Int_t iLayer = iLayerBegin; iLayer <= iLayerEnd; iLayer++) {
    TLatex* msg = new TLatex(0.5 + iBarrel * 4, StaveBoundary[iLayer - 1] + 2 + iBarrel * 2, Form("L%d", iLayer - 1));
    msg->SetTextSize(18);
    msg->SetTextFont(43);
    h->GetListOfFunctions()->Add(msg);

    if (iLayer < iLayerEnd) {
      auto l = new TLine(-0.5, StaveBoundary[iLayer] - 0.5, nChipsPerStave[iLayer] - 0.5, StaveBoundary[iLayer] - 0.5);
      h->GetListOfFunctions()->Add(l);
    }
  }
}

Int_t ITSThresholdCalibrationTask::getBarrel(Int_t iLayer)
{

  Int_t iBarrel;
  if (iLayer < 3)
    iBarrel = 0;
  else if (iLayer < 5)
    iBarrel = 1;
  else
    iBarrel = 2;

  return iBarrel;
}

void ITSThresholdCalibrationTask::formatAxes(TH1* h, const char* xTitle, const char* yTitle, float xOffset, float yOffset)
{
  h->GetXaxis()->SetTitle(xTitle);
  h->GetYaxis()->SetTitle(yTitle);
  h->GetXaxis()->SetTitleOffset(xOffset);
  h->GetYaxis()->SetTitleOffset(yOffset);
}

void ITSThresholdCalibrationTask::publishHistos()
{
  for (unsigned int iObj = 0; iObj < mPublishedObjects.size(); iObj++) {
    getObjectsManager()->startPublishing(mPublishedObjects.at(iObj));
    ILOG(Info, Support) << " Object will be published: " << mPublishedObjects.at(iObj)->GetName() << ENDM;
  }
}

std::vector<std::string> ITSThresholdCalibrationTask::split(std::string s, std::string delimiter)
{
  size_t pos_start = 0, pos_end, delim_len = delimiter.length();
  std::string token;
  std::vector<std::string> res;

  while ((pos_end = s.find(delimiter, pos_start)) != string::npos) {
    token = s.substr(pos_start, pos_end - pos_start);
    pos_start = pos_end + delim_len;
    res.push_back(token);
  }

  res.push_back(s.substr(pos_start));
  return res;
}

} // namespace o2::quality_control_modules::its
