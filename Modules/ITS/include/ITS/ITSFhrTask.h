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
/// \file   ITSFhrTask.h
/// \author Liang Zhang
/// \author Jian Liu
///

#ifndef QC_MODULE_ITS_ITSFHRTASK_H
#define QC_MODULE_ITS_ITSFHRTASK_H

#include "QualityControl/TaskInterface.h"
#include <ITSMFTReconstruction/ChipMappingITS.h>
#include <ITSMFTReconstruction/PixelData.h>
#include <ITSBase/GeometryTGeo.h>
#include <ITSMFTReconstruction/RawPixelDecoder.h>

#include <TText.h>
#include <TH1.h>
#include <TH2.h>
#include <THnSparse.h>
#include <TH2Poly.h>

class TH1F;
class TH2;

using namespace o2::quality_control::core;
using PixelReader = o2::itsmft::PixelReader;

namespace o2::quality_control_modules::its
{

/// \brief ITS Fake-hit rate real-time data processing task
/// Working with the chain of "Detector -> RU -> CRU -> Readout -> STFB -> o2-dpl-raw-proxy ->  QC"
class ITSFhrTask final : public TaskInterface
{
  using ChipPixelData = o2::itsmft::ChipPixelData;

 public:
  /// \brief Constructor
  ITSFhrTask();
  /// Destructor
  ~ITSFhrTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  int mAverageProcessTime = 0;
  void setAxisTitle(TH1* object, const char* xTitle, const char* yTitle);
  void createGeneralPlots(); //create General Plots depend mLayer which define by json file
  void createErrorTriggerPlots();
  void createOccupancyPlots();
  void setPlotsFormat();
  void getParameters(); //get Task parameters from json file
  void resetGeneralPlots();
  void resetOccupancyPlots();
  void resetObject(TH1* obj);
  void getStavePoint(int layer, int stave, double* px, double* py); //prepare for fill TH2Poly, get all point for add TH2Poly bin
  //detector information
  static constexpr int NCols = 1024; //column number in Alpide chip
  static constexpr int NRows = 512;  //row number in Alpide chip
  static constexpr int NLayer = 7;   //layer number in ITS detector
  static constexpr int NLayerIB = 3;

  static constexpr int NSubStave2[NLayer] = { 1, 1, 1, 2, 2, 2, 2 };
  const int NSubStave[NLayer] = { 1, 1, 1, 2, 2, 2, 2 };
  const int NStaves[NLayer] = { 12, 16, 20, 24, 30, 42, 48 };
  const int nHicPerStave[NLayer] = { 1, 1, 1, 8, 8, 14, 14 };
  const int nChipsPerHic[NLayer] = { 9, 9, 9, 14, 14, 14, 14 };
  const int ChipBoundary[NLayer + 1] = { 0, 108, 252, 432, 3120, 6480, 14712, 24120 };
  const int StaveBoundary[NLayer + 1] = { 0, 12, 28, 48, 72, 102, 144, 192 };
  const int ReduceFraction = 1;
  const float StartAngle[7] = { 16.997 / 360 * (TMath::Pi() * 2.), 17.504 / 360 * (TMath::Pi() * 2.), 17.337 / 360 * (TMath::Pi() * 2.), 8.75 / 360 * (TMath::Pi() * 2.), 7 / 360 * (TMath::Pi() * 2.), 5.27 / 360 * (TMath::Pi() * 2.), 4.61 / 360 * (TMath::Pi() * 2.) }; //start angle of first stave in each layer
  const float MidPointRad[7] = { 23.49, 31.586, 39.341, 197.598, 246.944, 345.348, 394.883 };                                                                                                                                                                               //mid point radius

  int mNThreads = 0;
  std::unordered_map<unsigned int, int> mHitPixelID_Hash[7][48][2][14][14]; //layer, stave, substave, hic, chip

  o2::itsmft::RawPixelDecoder<o2::itsmft::ChipMappingITS>* mDecoder;
  ChipPixelData* mChipDataBuffer = nullptr;
  std::vector<ChipPixelData> mChipsBuffer;
  int mHitNumberOfChip[7][48][2][14][14] = { { { { { 0 } } } } }; //layer, stave, substave, hic, chip
  int mTimeFrameId = 0;
  uint32_t mTriggerTypeCount[13] = { 0 };

  int mNError = 19;
  int mNTrigger = 13;
  unsigned int mErrors[19] = { 0 };
  static constexpr int NTrigger = 13;
  int16_t partID = 0;
  int mLayer;
  int mHitCutForCheck = 100; //Hit number cut for fired pixel check in a trigger
  int mGetTFFromBinding = 0;
  int mHitCutForNoisyPixel = 1024;        //Hit number cut for noisy pixel, this number should be define according how many TF will be accumulated before reset(one can reference the cycle time)
  float mOccupancyCutForNoisyPixel = 0.1; //Occupancy cut for noisy pixel. check if the hit/event value over this cut. similar with mHitCutForNoisyPixel

  std::unordered_map<unsigned int, int>*** mHitPixelID_InStave /* = new std::unordered_map<unsigned int, int>**[NStaves[lay]]*/;
  int** mHitnumber /* = new int*[NStaves[lay]]*/;       //IB : hitnumber[stave][chip]; OB : hitnumber[stave][hic]
  double** mOccupancy /* = new double*[NStaves[lay]]*/; //IB : occupancy[stave][chip]; OB : occupancy[stave][hic]
  int*** mErrorCount /* = new int**[NStaves[lay]]*/;    //IB : errorcount[stave][FEE][errorid]
  int mNoisyPixelNumber[7][48] = { { 0 } };

  TString mTriggerType[NTrigger] = { "ORBIT", "HB", "HBr", "HC", "PHYSICS", "PP", "CAL", "SOT", "EOT", "SOC", "EOC", "TF", "INT" };

  //General plots
  TH1F* mTFInfo; //count vs TF ID
  TH1D* mErrorPlots;
  TH2I* mErrorVsFeeid;
  TH2I* mTriggerVsFeeid;
  TH1D* mTriggerPlots;
  //TH1D* mInfoCanvas;//TODO: default, not implemented yet
  TH2I* mInfoCanvasComm;       //tmp object decidated to ITS commissioning
  TH2I* mInfoCanvasOBComm;     //tmp object decidated to ITS Outer Barral commissioning
  TH2Poly* mGeneralOccupancy;  //Max Occuapncy(chip/hic) in one stave
  TH2Poly* mGeneralNoisyPixel; //Noisy pixel number in one stave

  TText* mTextForShifter;
  TText* mTextForShifterOB;
  TText* mTextForShifter2;
  TText* mTextForShifterOB2;

  //Occupancy and hit-map
  THnSparseI* mStaveHitmap[7][48];
  TH2D* mChipStaveOccupancy[7];
  TH2I* mChipStaveEventHitCheck[7];
  TH1D* mOccupancyPlot[7];

  std::string mRunNumberPath;
  std::string mRunNumber = "000000";

  //Geometry decoder
  o2::its::GeometryTGeo* mGeom;
  std::string mGeomPath;
};
} // namespace o2::quality_control_modules::its

#endif // QC_MODULE_ITS_ITSFHRTASK_H
