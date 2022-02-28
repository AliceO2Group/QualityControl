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

  struct CalibrationResStruct {
    int Layer;
    int Stave;
    int Hs;
    int HIC;
    int ChipID;
    float VCASN;
    float RMS;
    float ITHR;
    float THR;
    float Noise;
    float NoiseRMS;
    int status;
  };

  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;
  std::vector<std::string> split(std::string, std::string);

 private:
  void publishHistos();
  void formatAxes(TH1* h, const char* xTitle, const char* yTitle, float xOffset = 1., float yOffset = 1.);
  void formatLayers(TH2* h, Int_t iBarrel);
  void addObject(TObject* aObject);
  void createAllHistos();
  void calculateAverages(Int_t Scan);
  Int_t getBarrel(Int_t iLayer);

  CalibrationResStruct CalibrationParser(string input);

  std::vector<TObject*> mPublishedObjects;

  std::string mRunNumber;
  std::string mRunNumberPath;

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
  TString sBarrelType[3] = { "IB", "MB", "OB" };
  Int_t nChips[3] = { 9, 112, 196 };
  Int_t nStaves[3] = { 48, 54, 90 };
  Int_t nXmax[3] = { 80, 100, 400 };
  TString sXtitles[3] = { "DAC", "DAC", "e" };

  TH2F *hThrAverageChipIB, *hIthrAverageChipIB, *hVcasnAverageChipIB;
  TH2F *hThrSumChipIB, *hIthrSumChipIB, *hVcasnSumChipIB;
  TH2F *hThrCountsChipIB, *hIthrCountsChipIB, *hVcasnCountsChipIB;

  TH2F *hThrRMSAverageChipIB, *hIthrRMSAverageChipIB, *hVcasnRMSAverageChipIB;
  TH2F *hThrRMSSumChipIB, *hIthrRMSSumChipIB, *hVcasnRMSSumChipIB;
  TH2F *hThrRMSCountsChipIB, *hIthrRMSCountsChipIB, *hVcasnRMSCountsChipIB;

  TH2F *hCalibrationChipSum[3][3], *hCalibrationChipCounts[3][3], *hCalibrationChipAverage[3][3];
  TH2F *hCalibrationRMSChipSum[3][3], *hCalibrationRMSChipAverage[3][3];
  TH2F *hCalibrationThrNoiseRMSChipAverage[3], *hCalibrationThrNoiseRMSChipSum[3];
  TH2F *hCalibrationThrNoiseChipAverage[3], *hCalibrationThrNoiseChipSum[3];

  TH1F* hSuccessRate;
  TH1F *hCalibrationLayer[7][3], *hCalibrationRMSLayer[7][3];
  TH1F *hCalibrationThrNoiseLayer[7], *hCalibrationThrNoiseRMSLayer[7];

  Int_t SuccessStatus[7], TotalStatus[7];
};
} // namespace o2::quality_control_modules::its

#endif // QC_MODULE_ITS_ITSTHRESHOLDCALIBRATIONTASK_H
