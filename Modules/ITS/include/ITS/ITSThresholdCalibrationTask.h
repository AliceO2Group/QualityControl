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
/// \file   ITSThreshodCalibrationTask.h
/// \author Artem Isakov
///

#ifndef QC_MODULE_ITS_ITSTHRESHOLDCALIBRATIONTASK_H
#define QC_MODULE_ITS_ITSTHRESHOLDCALIBRATIONTASK_H

#include "QualityControl/TaskInterface.h"
#include <TH1D.h>
#include <TH2D.h>
#include <ITSBase/GeometryTGeo.h>
#include <TTree.h>
#include "ITSMFTReconstruction/ChipMappingITS.h"

class TH1D;
class TH2D;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::its
{

class ITSThresholdCalibrationTask : public TaskInterface
{

 public:
  ITSThresholdCalibrationTask();
  ~ITSThresholdCalibrationTask() override;

  struct CalibrationResStructTHR {
    int Layer;
    int Stave;
    int Hs;
    int HIC;
    int ChipID;
    float MainVal; // can be THR, ITHR, VCASN
    float RMS;
    float Noise;
    float NoiseRMS;
    float status;
    float Tot;    // time over threshold
    float TotRms; // time over threshold rms
    float Rt;     // rise time
    float RtRms;  // rise time rms
  };
  struct CalibrationResStructPixel {
    int Layer;
    int Stave;
    int Hs;
    int HIC;
    int ChipID;
    int Type;
    int counts;
    int Dcols;
  };

  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;
  std::vector<std::string> splitString(std::string, std::string);

 private:
  void publishHistos();
  void formatAxes(TH1* h, const char* xTitle, const char* yTitle, float xOffset = 1., float yOffset = 1.);
  void formatLayers(TH2* h, Int_t iBarrel);
  void addObject(TObject* aObject);
  void createAllHistos();
  Int_t getBarrel(Int_t iLayer);
  int getCurrentChip(int barrel, int chipid, int hic, int hs);

  void doAnalysisTHR(string inString, int iScan);
  void doAnalysisPixel(string inString);

  CalibrationResStructTHR CalibrationParserTHR(string input);
  CalibrationResStructPixel CalibrationParserPixel(string input);

  std::vector<TObject*> mPublishedObjects;

  static constexpr int NLayer = 7; // layer number in ITS detector
  static constexpr int NLayerIB = 3;

  static constexpr int NSubStave2[NLayer] = { 1, 1, 1, 2, 2, 2, 2 };
  const int NSubStave[NLayer] = { 1, 1, 1, 2, 2, 2, 2 };
  const int NStaves[NLayer] = { 12, 16, 20, 24, 30, 42, 48 };
  const int nHicPerStave[NLayer] = { 1, 1, 1, 8, 8, 14, 14 };
  const int nChipsPerStave[NLayer] = { 9, 9, 9, 112, 112, 196, 196 };
  const int ChipBoundary[NLayer + 1] = { 0, 108, 252, 432, 3120, 6480, 14712, 24120 };
  const int StaveBoundary[NLayer] = { 0, 12, 28, 0, 24, 0, 42 };

  TString sScanTypes[3] = { "VCASN", "ITHR", "THR" };
  TString sCalibrationType[3] = { "Noisy", "Dead", "Ineff" };
  TString sBarrelType[3] = { "IB", "ML", "OL" };
  Int_t nChips[3] = { 9, 112, 196 };
  Int_t nStaves[3] = { 48, 54, 90 };
  Int_t nXmax[3] = { 130, 100, 450 };
  Int_t nZmax[3] = { 130, 110, 300 };
  Int_t nZmin[3] = { 20, 20, 30 };

  TString sXtitles[3] = { "DAC", "DAC", "e" };

  TH2F *hCalibrationChipDone[3], *hCalibrationChipAverage[3][3], *hCalibrationRMSChipAverage[3][3];
  TH2F *hCalibrationThrNoiseRMSChipAverage[3], *hCalibrationThrNoiseChipAverage[3];
  // TH2F *hCalibrationDeadColumns[3], *hCalibrationDeadPixels[3];
  TH2D* hCalibrationDColChipAverage[3];
  TH2D* hCalibrationPixelpAverage[3][3];

  TH2F* hUnsuccess[3];
  TH1F *hCalibrationLayer[NLayer][3], *hCalibrationRMSLayer[NLayer][3];
  TH1F *hCalibrationThrNoiseLayer[NLayer], *hCalibrationThrNoiseRMSLayer[NLayer];

  // Histograms for pulse-length scan
  TH2F *hTimeOverThreshold[3], *hTimeOverThresholdRms[3], *hRiseTime[3], *hRiseTimeRms[3];
  TH1F *hTimeOverThresholdLayer[NLayer], *hTimeOverThresholdRmsLayer[NLayer], *hRiseTimeLayer[NLayer], *hRiseTimeRmsLayer[NLayer];

  o2::itsmft::ChipMappingITS mp;
};
} // namespace o2::quality_control_modules::its

#endif // QC_MODULE_ITS_ITSTHRESHOLDCALIBRATIONTASK_H
