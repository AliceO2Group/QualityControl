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
#include "Framework/InputRecordWalker.h"

using namespace o2::itsmft;
using namespace o2::its;

namespace o2::quality_control_modules::its
{

ITSThresholdCalibrationTask::ITSThresholdCalibrationTask() : TaskInterface()
{
}

ITSThresholdCalibrationTask::~ITSThresholdCalibrationTask()
{
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
      delete hCalibrationChipAverage[iScan][iBarrel];
      delete hCalibrationRMSChipAverage[iScan][iBarrel];
    }
  }

  for (int iBarrel = 0; iBarrel < 3; iBarrel++) {

    delete hCalibrationThrNoiseChipAverage[iBarrel];
    delete hCalibrationThrNoiseRMSChipAverage[iBarrel];
    delete hCalibrationChipDone[iBarrel];
    delete hUnsuccess[iBarrel];

    delete hCalibrationDColChipAverage[iBarrel];
    for (int iPixelScanType = 0; iPixelScanType < 3; iPixelScanType++)
      delete hCalibrationPixelpAverage[iPixelScanType][iBarrel];
  }
}

void ITSThresholdCalibrationTask::initialize(o2::framework::InitContext& /*ctx*/)
{

  ILOG(Info, Support) << "initialize ITSThresholdCalibrationTask" << ENDM;

  createAllHistos();
  publishHistos();
}

void ITSThresholdCalibrationTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;
}

void ITSThresholdCalibrationTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void ITSThresholdCalibrationTask::monitorData(o2::framework::ProcessingContext& ctx)
{

  string inStringChipDone, inString, inPixel;
  char scanType;
  for (auto&& input : o2::framework::InputRecordWalker(ctx.inputs())) {
    if (input.header != nullptr && input.payload != nullptr) {
      const auto* header = o2::framework::DataRefUtils::getHeader<header::DataHeader*>(input);

      if ((strcmp(header->dataOrigin.str, "ITS") == 0) && (strcmp(header->dataDescription.str, "TSTR") == 0)) {
        const auto tmpstring = ctx.inputs().get<gsl::span<char>>(input);
        std::copy(tmpstring.begin(), tmpstring.end(), std::back_inserter(inString));
      }
      if ((strcmp(header->dataOrigin.str, "ITS") == 0) && (strcmp(header->dataDescription.str, "QCSTR") == 0)) {
        const auto tmpstring = ctx.inputs().get<gsl::span<char>>(input);
        std::copy(tmpstring.begin(), tmpstring.end(), std::back_inserter(inStringChipDone));
      }
      if ((strcmp(header->dataOrigin.str, "ITS") == 0) && (strcmp(header->dataDescription.str, "PIXTYP") == 0)) {
        const auto tmpstring = ctx.inputs().get<gsl::span<char>>(input);
        std::copy(tmpstring.begin(), tmpstring.end(), std::back_inserter(inPixel));
      }

      if ((strcmp(header->dataOrigin.str, "ITS") == 0) && (strcmp(header->dataDescription.str, "SCANT") == 0)) {
        scanType = ctx.inputs().get<char>(input);
      }
    }
  }

  Int_t iScan;
  if (scanType == 'V')
    iScan = 0;
  else if (scanType == 'I')
    iScan = 1;
  else if (scanType == 'T')
    iScan = 2;
  else if (scanType == 'A' || scanType == 'D')
    iScan = 3;
  auto splitRes = splitString(inString, "O2");

  if (scanType == 'A' || scanType == 'D')
    doAnalysisPixel(inPixel);
  else
    doAnalysisTHR(inString, iScan);

  //--------------- Fill chips for which scan is completed
  auto splitResChipDone = splitString(inStringChipDone, "O2");
  for (auto StaveStr : splitResChipDone) {
    if (StaveStr.size() > 0) {
      CalibrationResStructTHR result = CalibrationParserTHR(StaveStr);

      int currentStave = StaveBoundary[result.Layer] + result.Stave + 1;
      int iBarrel = getBarrel(result.Layer);
      int currentChip = getCurrentChip(iBarrel, result.ChipID, result.HIC, result.Hs);
      if (hCalibrationChipDone[iBarrel]->GetBinContent(currentChip, currentStave) > 0) {
        continue; // chip may appear >twice here
      }
      hCalibrationChipDone[iBarrel]->Fill(currentChip - 1, currentStave - 1);
    }
  }
}

void ITSThresholdCalibrationTask::doAnalysisTHR(string inString, int iScan)
{

  auto splitRes = splitString(inString, "O2");

  //-------------- General TH2 plots
  for (auto StaveStr : splitRes) {
    if (StaveStr.size() > 0) {
      CalibrationResStructTHR result = CalibrationParserTHR(StaveStr);

      int currentStave = StaveBoundary[result.Layer] + result.Stave + 1;
      int iBarrel = getBarrel(result.Layer);
      int currentChip = getCurrentChip(iBarrel, result.ChipID, result.HIC, result.Hs);

      Double_t calibrationValue;
      if (iScan == 0) {
        calibrationValue = result.VCASN;
      } else if (iScan == 1) {
        calibrationValue = result.ITHR;
      } else if (iScan == 2) {
        calibrationValue = result.THR;
        hCalibrationThrNoiseChipAverage[iBarrel]->SetBinContent(currentChip, currentStave, result.Noise);
        hCalibrationThrNoiseRMSChipAverage[iBarrel]->Fill(currentChip, currentStave, result.NoiseRMS);
      }

      hCalibrationChipAverage[iScan][iBarrel]->SetBinContent(currentChip, currentStave, calibrationValue);
      hCalibrationRMSChipAverage[iScan][iBarrel]->SetBinContent(currentChip, currentStave, result.RMS);

      // -------------- Fill percentage of unsuccess
      hUnsuccess[iBarrel]->SetBinContent(currentChip, currentStave, result.status);
    }
  }

  //------------------ 1D summary plots per layer
  for (int iLayer = 0; iLayer < 7; iLayer++) { // TH1 plots per layer

    Int_t iBarrel = getBarrel(iLayer);
    hCalibrationLayer[iLayer][iScan]->Reset();
    hCalibrationRMSLayer[iLayer][iScan]->Reset();
    if (iScan == 2) {
      hCalibrationThrNoiseLayer[iLayer]->Reset();
      hCalibrationThrNoiseRMSLayer[iLayer]->Reset();
    }

    for (int iStave = 1; iStave <= NStaves[iLayer]; iStave++) {
      for (int iChip = 1; iChip <= nChipsPerStave[iLayer]; iChip++) {

        if (hCalibrationChipAverage[iScan][iBarrel]->GetBinContent(iChip, StaveBoundary[iLayer] + iStave) == 0)
          continue; // to avoid 0-entries

        hCalibrationLayer[iLayer][iScan]->Fill(hCalibrationChipAverage[iScan][iBarrel]->GetBinContent(iChip, StaveBoundary[iLayer] + iStave));
        hCalibrationRMSLayer[iLayer][iScan]->Fill(hCalibrationRMSChipAverage[iScan][iBarrel]->GetBinContent(iChip, StaveBoundary[iLayer] + iStave));

        if (iScan == 2) {
          hCalibrationThrNoiseLayer[iLayer]->Fill(hCalibrationThrNoiseChipAverage[iBarrel]->GetBinContent(iChip, StaveBoundary[iLayer] + iStave));
          hCalibrationThrNoiseRMSLayer[iLayer]->Fill(hCalibrationThrNoiseChipAverage[iBarrel]->GetBinContent(iChip, StaveBoundary[iLayer] + iStave));
        }
      }
    }
  }
}

void ITSThresholdCalibrationTask::doAnalysisPixel(string inString)
{

  auto splitRes = splitString(inString, "O2");

  //-------------- General TH2 plots
  for (auto StaveStr : splitRes) {
    if (StaveStr.size() > 0) {
      CalibrationResStructPixel result = CalibrationParserPixel(StaveStr);

      int currentStave = StaveBoundary[result.Layer] + result.Stave + 1;
      int iBarrel = getBarrel(result.Layer);
      int currentChip = getCurrentChip(iBarrel, result.ChipID, result.HIC, result.Hs);

      hCalibrationPixelpAverage[result.Type][iBarrel]->SetBinContent(currentChip, currentStave, result.counts);
      if (result.Type == 0)
        hCalibrationDColChipAverage[iBarrel]->SetBinContent(currentChip, currentStave, result.Dcols);
    }
  }
}

int ITSThresholdCalibrationTask::getCurrentChip(int barrel, int chipid, int hic, int hs)
{
  int currentChip;
  switch (barrel) {
    case 0: {
      currentChip = chipid + 1;
      break;
    }
    case 1: {
      currentChip = chipid < 7 ? (chipid + 1) + 14 * (hic - 1) + hs * 56 : chipid + 14 * (hic - 1) + hs * 56; // this is already from 1 to 112
      break;
    }
    case 2: {
      currentChip = chipid < 7 ? (chipid + 1) + 14 * (hic - 1) + hs * 98 : chipid + 14 * (hic - 1) + hs * 98; // this is already from 1 to 196
      break;
    }
  }
  return currentChip;
}

ITSThresholdCalibrationTask::CalibrationResStructPixel ITSThresholdCalibrationTask::CalibrationParserPixel(string input)
{
  CalibrationResStructPixel result;
  auto StaveINFO = splitString(input, ",");

  for (string info : StaveINFO) {
    if (info.size() == 0)
      continue;

    std::cout << "We have string: " << info << std::endl;
    std::string name = splitString(info, ":")[0];
    string data = splitString(info, ":")[1];

    if (name == "ChipID") {
      int o2chipid = std::stod(data);
      int Hs, HIC, ChipID, Layer, Stave;
      mp.expandChipInfoHW(o2chipid, Layer, Stave, Hs, HIC, ChipID);
      result.Hs = Hs;
      result.HIC = HIC;
      result.Layer = Layer;
      result.Stave = Stave;
      result.ChipID = ChipID;
    } else if (name == "PixelType") {
      if (data == "Noisy")
        result.Type = 0;
      else if (data == "Dead")
        result.Type = 1;
      else if (data == "Ineff")
        result.Type = 2;
    } else if (name == "PixelNos") {
      result.counts = std::stoi(data);
    } else if (name == "DcolNos") {
      result.Dcols = std::stoi(data);
    }
  }
  return result;
}

ITSThresholdCalibrationTask::CalibrationResStructTHR ITSThresholdCalibrationTask::CalibrationParserTHR(string input)
{
  CalibrationResStructTHR result;
  auto StaveINFO = splitString(input, ",");
  for (string info : StaveINFO) {
    if (info.size() == 0)
      continue;

    if (info.substr(0, 1) == "L") {
      result.Layer = std::stod(info.substr(1, 1));
      result.Stave = std::stod(info.substr(3, 2));
    } else {

      std::string name = splitString(info, ":")[0];
      string data = splitString(info, ":")[1];
      if (name == "ChipID") {
        int o2chipid = std::stod(data);
        int Hs, HIC, ChipID, Layer, Stave;
        mp.expandChipInfoHW(o2chipid, Layer, Stave, Hs, HIC, ChipID);
        result.Hs = Hs;
        result.HIC = HIC;
        result.Layer = Layer;
        result.Stave = Stave;
        result.ChipID = ChipID;
      } else if (name == "VCASN") {
        result.VCASN = std::stof(data);
      } else if (name == "Rms") {
        result.RMS = std::stof(data);
      } else if (name == "Status") {
        result.status = std::stof(data);
      } else if (name == "Noise") {
        result.Noise = std::stof(data);
      } else if (name == "NoiseRms") {
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
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void ITSThresholdCalibrationTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void ITSThresholdCalibrationTask::reset()
{
  ILOG(Info, Support) << "Resetting the histogram" << ENDM;

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
      hCalibrationChipAverage[iScan][iBarrel]->Reset();
      hCalibrationRMSChipAverage[iScan][iBarrel]->Reset();
    }
  }

  for (int iBarrel = 0; iBarrel < 3; iBarrel++) {

    hUnsuccess[iBarrel]->Reset();
    hCalibrationThrNoiseChipAverage[iBarrel]->Reset();
    hCalibrationThrNoiseRMSChipAverage[iBarrel]->Reset();
    hCalibrationChipDone[iBarrel]->Reset();

    hCalibrationDColChipAverage[iBarrel]->Reset();
    for (int iPixelScanType = 0; iPixelScanType < 3; iPixelScanType++)
      hCalibrationPixelpAverage[iPixelScanType][iBarrel]->Reset();
  }
}

void ITSThresholdCalibrationTask::createAllHistos()
{
  for (int iScan = 0; iScan < 3; iScan++) {

    for (int iLayer = 0; iLayer < 7; iLayer++) {
      hCalibrationLayer[iLayer][iScan] = new TH1F(Form("%sLayer%d", sScanTypes[iScan].Data(), iLayer), Form("%s for Layer%d", sScanTypes[iScan].Data(), iLayer), nXmax[iScan], -0.5, nXmax[iScan] - 0.5);
      hCalibrationLayer[iLayer][iScan]->SetStats(0);
      formatAxes(hCalibrationLayer[iLayer][iScan], Form("%s (%s) ", sScanTypes[iScan].Data(), sXtitles[iScan].Data()), "Chip counts", 1, 1.10);
      addObject(hCalibrationLayer[iLayer][iScan]);

      hCalibrationRMSLayer[iLayer][iScan] = new TH1F(Form("%sRMSLayer%d", sScanTypes[iScan].Data(), iLayer), Form("%s RMS for Layer%d", sScanTypes[iScan].Data(), iLayer), 50, -0.5, 9.5);
      hCalibrationRMSLayer[iLayer][iScan]->SetStats(0);
      formatAxes(hCalibrationRMSLayer[iLayer][iScan], Form("RMS (%s)", sXtitles[iScan].Data()), "Chip counts", 1, 1.10);
      addObject(hCalibrationRMSLayer[iLayer][iScan]);
    }

    for (int iBarrel = 0; iBarrel < 3; iBarrel++) {

      hCalibrationChipAverage[iScan][iBarrel] = new TH2F(Form("%sChipAverage%s", sScanTypes[iScan].Data(), sBarrelType[iBarrel].Data()), Form("Average chip %s for %s", sScanTypes[iScan].Data(), sBarrelType[iBarrel].Data()), nChips[iBarrel], -0.5, nChips[iBarrel] - 0.5, nStaves[iBarrel], -0.5, nStaves[iBarrel] - 0.5);
      hCalibrationChipAverage[iScan][iBarrel]->SetMinimum(nZmin[iScan]);
      hCalibrationChipAverage[iScan][iBarrel]->SetMaximum(nZmax[iScan]);
      hCalibrationChipAverage[iScan][iBarrel]->SetStats(0);
      if (iBarrel != 0)
        formatAxes(hCalibrationChipAverage[iScan][iBarrel], "Chip", "", 1, 1.10);
      formatLayers(hCalibrationChipAverage[iScan][iBarrel], iBarrel);

      addObject(hCalibrationChipAverage[iScan][iBarrel]);

      hCalibrationRMSChipAverage[iScan][iBarrel] = new TH2F(Form("%sRMSChipAverage%s", sScanTypes[iScan].Data(), sBarrelType[iBarrel].Data()), Form("Average chip %s RMS for %s", sScanTypes[iScan].Data(), sBarrelType[iBarrel].Data()), nChips[iBarrel], -0.5, nChips[iBarrel] - 0.5, nStaves[iBarrel], -0.5, nStaves[iBarrel] - 0.5);
      hCalibrationRMSChipAverage[iScan][iBarrel]->SetStats(0);
      if (iBarrel != 0)
        formatAxes(hCalibrationRMSChipAverage[iScan][iBarrel], "Chip", "", 1, 1.10);
      formatLayers(hCalibrationRMSChipAverage[iScan][iBarrel], iBarrel);

      addObject(hCalibrationRMSChipAverage[iScan][iBarrel]);
    }
  }
  //------------------Noise histograms for THR scan, success rate, and chip completed

  for (int iBarrel = 0; iBarrel < 3; iBarrel++) { // TH2 for THR noise plots

    hCalibrationThrNoiseChipAverage[iBarrel] = new TH2F(Form("ThrNoiseChipAverage%s", sBarrelType[iBarrel].Data()), Form("Average chip threshold noise for %s", sBarrelType[iBarrel].Data()), nChips[iBarrel], -0.5, nChips[iBarrel] - 0.5, nStaves[iBarrel], -0.5, nStaves[iBarrel] - 0.5);
    hCalibrationThrNoiseChipAverage[iBarrel]->SetStats(0);
    if (iBarrel != 0)
      formatAxes(hCalibrationThrNoiseChipAverage[iBarrel], "Chip", "", 1, 1.10);
    formatLayers(hCalibrationThrNoiseChipAverage[iBarrel], iBarrel);
    addObject(hCalibrationThrNoiseChipAverage[iBarrel]);

    hCalibrationThrNoiseRMSChipAverage[iBarrel] = new TH2F(Form("ThrNoiseRMSChipAverage%s", sBarrelType[iBarrel].Data()), Form("Average chip threshold NoiseRMS for %s", sBarrelType[iBarrel].Data()), nChips[iBarrel], -0.5, nChips[iBarrel] - 0.5, nStaves[iBarrel], -0.5, nStaves[iBarrel] - 0.5);
    hCalibrationThrNoiseRMSChipAverage[iBarrel]->SetStats(0);
    if (iBarrel != 0)
      formatAxes(hCalibrationThrNoiseRMSChipAverage[iBarrel], "Chip", "", 1, 1.10);
    formatLayers(hCalibrationThrNoiseRMSChipAverage[iBarrel], iBarrel);
    addObject(hCalibrationThrNoiseRMSChipAverage[iBarrel]);

    hCalibrationChipDone[iBarrel] = new TH2F(Form("ChipDone%s", sBarrelType[iBarrel].Data()), Form("Chips Done %s", sBarrelType[iBarrel].Data()), nChips[iBarrel], -0.5, nChips[iBarrel] - 0.5, nStaves[iBarrel], -0.5, nStaves[iBarrel] - 0.5);
    hCalibrationChipDone[iBarrel]->SetStats(0);
    if (iBarrel != 0)
      formatAxes(hCalibrationChipDone[iBarrel], "Chip", "", 1, 1.10);

    formatLayers(hCalibrationChipDone[iBarrel], iBarrel);
    addObject(hCalibrationChipDone[iBarrel]);

    hUnsuccess[iBarrel] = new TH2F(Form("ChipUnsuccess%s", sBarrelType[iBarrel].Data()), Form("Percentage of unsuccess %s", sBarrelType[iBarrel].Data()), nChips[iBarrel], -0.5, nChips[iBarrel] - 0.5, nStaves[iBarrel], -0.5, nStaves[iBarrel] - 0.5);
    hUnsuccess[iBarrel]->SetStats(0);
    hUnsuccess[iBarrel]->SetMinimum(0);
    hUnsuccess[iBarrel]->SetMaximum(100);
    if (iBarrel != 0)
      formatAxes(hUnsuccess[iBarrel], "Chip", "", 1, 1.10);
    formatLayers(hUnsuccess[iBarrel], iBarrel);
    addObject(hUnsuccess[iBarrel]);
  }
  for (int iLayer = 0; iLayer < 7; iLayer++) {
    hCalibrationThrNoiseLayer[iLayer] = new TH1F(Form("ThrNoiseLayer%d", iLayer), Form("Threshold Noise for Layer%d", iLayer), 10, -0.5, 9.5);
    hCalibrationThrNoiseLayer[iLayer]->SetStats(0);
    formatAxes(hCalibrationThrNoiseLayer[iLayer], "THR noise (e) ", "Chip counts", 1, 1.10);
    addObject(hCalibrationThrNoiseLayer[iLayer]);

    hCalibrationThrNoiseRMSLayer[iLayer] = new TH1F(Form("ThrNoiseRMSLayer%d", iLayer), Form("Threshold Noise RMS for Layer%d", iLayer), 50, -0.5, 9.5);
    hCalibrationThrNoiseRMSLayer[iLayer]->SetStats(0);
    formatAxes(hCalibrationThrNoiseRMSLayer[iLayer], "THR noise RMS (e)", "Chip counts", 1, 1.10);
    addObject(hCalibrationThrNoiseRMSLayer[iLayer]);
  }

  for (int iBarrel = 0; iBarrel < 3; iBarrel++) {

    for (int iType = 0; iType < 3; iType++) {
      hCalibrationPixelpAverage[iType][iBarrel] = new TH2D(Form("%sPixels%s", sCalibrationType[iType].Data(), sBarrelType[iBarrel].Data()), Form("Number of %s pixels per chip for %s", sCalibrationType[iType].Data(), sBarrelType[iBarrel].Data()), nChips[iBarrel], -0.5, nChips[iBarrel] - 0.5, nStaves[iBarrel], -0.5, nStaves[iBarrel] - 0.5);
      hCalibrationPixelpAverage[iType][iBarrel]->SetStats(0);
      if (iBarrel != 0)
        formatAxes(hCalibrationPixelpAverage[iType][iBarrel], "Chip", "", 1, 1.10);
      formatLayers(hCalibrationPixelpAverage[iType][iBarrel], iBarrel);
      addObject(hCalibrationPixelpAverage[iType][iBarrel]);
    }
    hCalibrationDColChipAverage[iBarrel] = new TH2D(Form("DCols%s", sBarrelType[iBarrel].Data()), Form("Number of DColls per chip for %s", sBarrelType[iBarrel].Data()), nChips[iBarrel], -0.5, nChips[iBarrel] - 0.5, nStaves[iBarrel], -0.5, nStaves[iBarrel] - 0.5);
    hCalibrationDColChipAverage[iBarrel]->SetStats(0);
    if (iBarrel != 0)
      formatAxes(hCalibrationDColChipAverage[iBarrel], "Chip", "", 1, 1.10);
    formatLayers(hCalibrationDColChipAverage[iBarrel], iBarrel);
    addObject(hCalibrationDColChipAverage[iBarrel]);
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

  //-----------Setting Labels for axis
  if (iBarrel == 0) {
    for (int i = 1; i <= h->GetNbinsX(); i++) {
      h->GetXaxis()->SetBinLabel(i, Form("Chip %d ", i - 1));
    }
  }

  int iLayer = iLayerBegin - 1;
  int iStave = 0;
  for (int i = 1; i <= h->GetNbinsY(); i++) {
    h->GetYaxis()->SetBinLabel(i, Form("L%d_%02d", iLayer, iStave));
    iStave++;
    if (iStave >= NStaves[iLayer]) {
      iStave = 0;
      iLayer++;
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

std::vector<std::string> ITSThresholdCalibrationTask::splitString(std::string s, std::string delimiter)
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
