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
  int mTFCount = 0;
  void setAxisTitle(TH1* object, const char* xTitle, const char* yTitle);
  void createGeneralPlots(); // create General Plots depend mLayer which define by json file
  void createErrorTriggerPlots();
  void createOccupancyPlots();
  void setPlotsFormat();
  void getParameters(); // get Task parameters from json file
  void resetGeneralPlots();
  void resetOccupancyPlots();
  void resetObject(TH1* obj);
  void getStavePoint(int layer, int stave, double* px, double* py); // prepare for fill TH2Poly, get all point for add TH2Poly bin
  // detector information
  static constexpr int NCols = 1024; // column number in Alpide chip
  static constexpr int NRows = 512;  // row number in Alpide chip
  static constexpr int NLayer = 7;   // layer number in ITS detector
  static constexpr int NLayerIB = 3;

  static constexpr int NSubStave2[NLayer] = { 1, 1, 1, 2, 2, 2, 2 };
  const int NSubStave[NLayer] = { 1, 1, 1, 2, 2, 2, 2 };
  const int NStaves[NLayer] = { 12, 16, 20, 24, 30, 42, 48 };
  const int nHicPerStave[NLayer] = { 1, 1, 1, 8, 8, 14, 14 };
  const int nLanePerHic[NLayer] = { 3, 3, 3, 2, 2, 2, 2 };
  const int nChipsPerHic[NLayer] = { 9, 9, 9, 14, 14, 14, 14 };
  const int ChipBoundary[NLayer + 1] = { 0, 108, 252, 432, 3120, 6480, 14712, 24120 };
  const int StaveBoundary[NLayer + 1] = { 0, 12, 28, 48, 72, 102, 144, 192 };
  const char* OBLabel34[16] = { "HIC1L_B0_ln7", "HIC1L_A8_ln6", "HIC2L_B0_ln8", "HIC2L_A8_ln5", "HIC3L_B0_ln9", "HIC3L_A8_ln4", "HIC4L_B0_ln10", "HIC4L_A8_ln3", "HIC1U_B0_ln21", "HIC1U_A8_ln20", "HIC2U_B0_ln22", "HIC2U_A8_ln19", "HIC3U_B0_ln23", "HIC3U_A8_ln18", "HIC4U_B0_ln24", "HIC4U_A8_ln17" };
  const char* OBLabel56[28] = { "HIC1L_B0_ln7", "HIC1L_A8_ln6", "HIC2L_B0_ln8", "HIC2L_A8_ln5", "HIC3L_B0_ln9", "HIC3L_A8_ln4", "HIC4L_B0_ln10", "HIC4L_A8_ln3", "HIC5L_B0_ln11", "HIC5L_A8_ln2", "HIC6L_B0_ln12", "HIC6L_A8_ln1", "HIC7L_B0_ln13", "HIC7L_A8_ln0", "HIC1U_B0_ln21", "HIC1U_A8_ln20", "HIC2U_B0_ln22", "HIC2U_A8_ln19", "HIC3U_B0_ln23", "HIC3U_A8_ln18", "HIC4U_B0_ln24", "HIC4U_A8_ln17", "HIC5U_B0_ln25", "HIC5U_A8_ln16", "HIC6U_B0_ln26", "HIC6U_A8_ln15", "HIC7U_B0_ln27", "HIC7U_A8_ln14" };
  const int ReduceFraction = 1;
  const float StartAngle[7] = { 16.997 / 360 * (TMath::Pi() * 2.), 17.504 / 360 * (TMath::Pi() * 2.), 17.337 / 360 * (TMath::Pi() * 2.), 8.75 / 360 * (TMath::Pi() * 2.), 7 / 360 * (TMath::Pi() * 2.), 5.27 / 360 * (TMath::Pi() * 2.), 4.61 / 360 * (TMath::Pi() * 2.) }; // start angle of first stave in each layer
  const float MidPointRad[7] = { 23.49, 31.586, 39.341, 197.598, 246.944, 345.348, 394.883 };                                                                                                                                                                               // mid point radius

  int mNThreads = 0;
  std::unordered_map<unsigned int, int> mHitPixelID_Hash[7][48][2][14][14]; // layer, stave, substave, hic, chip

  o2::itsmft::RawPixelDecoder<o2::itsmft::ChipMappingITS>* mDecoder;
  ChipPixelData* mChipDataBuffer = nullptr;
  std::vector<ChipPixelData> mChipsBuffer;
  int mHitNumberOfChip[7][48][2][14][14] = { { { { { 0 } } } } }; // layer, stave, substave, hic, chip
  unsigned int mTimeFrameId = 0;
  uint32_t mTriggerTypeCount[13] = { 0 };

  int mNError = 19;
  int mNTrigger = 13;
  unsigned int mErrors[19] = { 0 };
  static constexpr int NTrigger = 13;
  int16_t partID = 0;
  int mLayer;
  int mHitCutForCheck = 100; // Hit number cut for fired pixel check in a trigger
  int mGetTFFromBinding = 0;
  int mHitCutForNoisyPixel = 1024;        // Hit number cut for noisy pixel, this number should be define according how many TF will be accumulated before reset(one can reference the cycle time)
  float mOccupancyCutForNoisyPixel = 0.1; // Occupancy cut for noisy pixel. check if the hit/event value over this cut. similar with mHitCutForNoisyPixel

  std::unordered_map<unsigned int, int>*** mHitPixelID_InStave /* = new std::unordered_map<unsigned int, int>**[NStaves[lay]]*/;
  int** mHitnumberLane /* = new int*[NStaves[lay]]*/;       // IB : hitnumber[stave][chip]; OB : hitnumber[stave][lane]
  double** mOccupancyLane /* = new double*[NStaves[lay]]*/; // IB : occupancy[stave][chip]; OB : occupancy[stave][Lane]
  int*** mErrorCount /* = new int**[NStaves[lay]]*/;        // IB : errorcount[stave][FEE][errorid]
  double** mChipPhi /* = new double*[NStaves[lay]]*/;       // IB/OB : mChipPhi[Stave][chip]
  double** mChipEta /* = new double*[NStaves[lay]]*/;       // IB/OB : mChipEta[Stave][chip]
  int** mChipStat /* = new double*[NStaves[lay]]*/;         // IB/OB : mChipStat[Stave][chip]
  int mNoisyPixelNumber[7][48] = { { 0 } };

  int mMaxGeneralAxisRange = -3;  // the range of TH2Poly plots z axis range, pow(10, mMinGeneralAxisRange) ~ pow(10, mMaxGeneralAxisRange)
  int mMinGeneralAxisRange = -12; //
  int mMaxGeneralNoisyAxisRange = 4000;
  int mMinGeneralNoisyAxisRange = 0;
  static constexpr int nbinsetaIB = 9;
  static constexpr int nbinsphiIB = 16;
  int mEtabins = 130;
  int mPhibins = 240;
  float etabinsIB[3][nbinsetaIB + 1] = { { -2.34442, -2.06375, -1.67756, -1.0732, 0., 1.0732, 1.67756, 2.06375, 2.34442, 2.6 }, { -2.05399, -1.77866, -1.40621, -0.851972, 0., 0.851972, 1.40621, 1.77866, 2.05399, 2.4 }, { -1.8412, -1.57209, -1.21477, -0.707629, 0., 0.707629, 1.21477, 1.57209, 1.8412, 2.1 } };
  float phibinsIB[3][nbinsphiIB + 1] = { { -2.86662, -2.34302, -1.81942, -1.29582, -0.772222, -0.248623, 0.203977, 0.274976, 0.596676, 0.798574, 0.989375, 1.32217, 1.38207, 1.84577, 2.36937, 2.89297, 3.2 }, { -2.93762, -2.54492, -2.15222, -1.75952, -1.36682, -0.974121, -0.581422, -0.188722, 0.159761, 0.47392, 0.78808, 1.10224, 1.77477, 2.16747, 2.56017, 2.95287, 3.2 }, { -2.98183, -2.66767, -2.35351, -2.03935, -1.72519, -1.41104, -1.09688, -0.782717, -0.468558, -0.154398, 1.4164, 1.73056, 2.04472, 2.35888, 2.67304, 2.98719, 3.2 } };

  float phibinsOB3[96 + 1] = { -3.114800, -3.037860, -2.983770, -2.907500, -2.853010, -2.776060, -2.721970, -2.645700, -2.591210, -2.514260, -2.460170, -2.383900, -2.329410, -2.252460, -2.198370, -2.122100, -2.067610, -1.990660, -1.936570, -1.860300, -1.805810, -1.728860, -1.674770, -1.598500, -1.544010, -1.467060, -1.412970, -1.336700, -1.282210, -1.205270, -1.151170, -1.074910, -1.020410, -0.943466, -0.889371, -0.813106, -0.758610, -0.681667, -0.627572, -0.551307, -0.496811, -0.419867, -0.365773, -0.289507, -0.235011, -0.158068, -0.103973, -0.027708, 0.026788, 0.103731, 0.157826, 0.234091, 0.288587, 0.365531, 0.419626, 0.495891, 0.550387, 0.627330, 0.681425, 0.757690, 0.812186, 0.889130, 0.943224, 1.019490, 1.073990, 1.150930, 1.205020, 1.281290, 1.335780, 1.412730, 1.466820, 1.543090, 1.597580, 1.674530, 1.728620, 1.804890, 1.859380, 1.936330, 1.990420, 2.066690, 2.121180, 2.198130, 2.252220, 2.328490, 2.382980, 2.459930, 2.514020, 2.590290, 2.644780, 2.721730, 2.775820, 2.852090, 2.906580, 2.983520, 3.037620, 3.113880, 3.18 };
  float phibinsOB4[120 + 1] = { -3.120100, -3.058560, -3.015340, -2.954240, -2.910660, -2.849120, -2.805900, -2.744800, -2.701220, -2.639690, -2.596460, -2.535360, -2.491780, -2.430250, -2.387020, -2.325920, -2.282340, -2.220810, -2.177580, -2.116480, -2.072900, -2.011370, -1.968140, -1.907040, -1.863460, -1.801930, -1.758700, -1.697600, -1.654020, -1.592490, -1.549260, -1.488160, -1.444580, -1.383050, -1.339820, -1.278720, -1.235140, -1.173610, -1.130380, -1.069280, -1.025700, -0.964169, -0.920941, -0.859843, -0.816263, -0.754730, -0.711501, -0.650403, -0.606823, -0.545290, -0.502062, -0.440964, -0.397384, -0.335850, -0.292622, -0.231524, -0.187944, -0.126411, -0.083183, -0.022085, 0.021495, 0.083029, 0.126257, 0.187355, 0.230935, 0.292468, 0.335696, 0.396794, 0.440374, 0.501908, 0.545136, 0.606234, 0.649814, 0.711347, 0.754575, 0.815673, 0.859253, 0.920787, 0.964015, 1.025110, 1.068690, 1.130230, 1.173450, 1.234550, 1.278130, 1.339670, 1.382890, 1.443990, 1.487570, 1.549110, 1.592330, 1.653430, 1.697010, 1.758540, 1.801770, 1.862870, 1.906450, 1.967980, 2.011210, 2.072310, 2.115890, 2.177420, 2.220650, 2.281750, 2.325330, 2.386860, 2.430090, 2.491190, 2.534770, 2.596300, 2.639530, 2.700630, 2.744210, 2.805740, 2.848970, 2.910070, 2.953650, 3.015180, 3.058410, 3.119510, 3.17 };
  float phibinsOB5[168 + 1] = { -3.126190, -3.082200, -3.051310, -3.007540, -2.976590, -2.932600, -2.901710, -2.857940, -2.826990, -2.783000, -2.752110, -2.708350, -2.677390, -2.633400, -2.602510, -2.558750, -2.527800, -2.483800, -2.452910, -2.409150, -2.378200, -2.334200, -2.303310, -2.259550, -2.228600, -2.184610, -2.153710, -2.109950, -2.079000, -2.035010, -2.004110, -1.960350, -1.929400, -1.885410, -1.854510, -1.810750, -1.779800, -1.735810, -1.704910, -1.661150, -1.630200, -1.586210, -1.555320, -1.511550, -1.480600, -1.436610, -1.405720, -1.361950, -1.331000, -1.287010, -1.256120, -1.212350, -1.181400, -1.137410, -1.106520, -1.062750, -1.031800, -0.987808, -0.956917, -0.913149, -0.882199, -0.838208, -0.807317, -0.763550, -0.732599, -0.688609, -0.657717, -0.613950, -0.583000, -0.539009, -0.508118, -0.464350, -0.433400, -0.389409, -0.358518, -0.314751, -0.283800, -0.239810, -0.208918, -0.165151, -0.134201, -0.090210, -0.059319, -0.015551, 0.015399, 0.059390, 0.090281, 0.134048, 0.164999, 0.208989, 0.239880, 0.283648, 0.314598, 0.358589, 0.389480, 0.433247, 0.464198, 0.508188, 0.539080, 0.582847, 0.613798, 0.657788, 0.688679, 0.732447, 0.763397, 0.807388, 0.838279, 0.882046, 0.912997, 0.956988, 0.987879, 1.031650, 1.062600, 1.106590, 1.137480, 1.181250, 1.212200, 1.256190, 1.287080, 1.330850, 1.361800, 1.405790, 1.436680, 1.480440, 1.511400, 1.555390, 1.586280, 1.630040, 1.661000, 1.704990, 1.735880, 1.779640, 1.810590, 1.854590, 1.885480, 1.929240, 1.960190, 2.004180, 2.035080, 2.078840, 2.109790, 2.153780, 2.184680, 2.228440, 2.259390, 2.303380, 2.334280, 2.378040, 2.408990, 2.452980, 2.483880, 2.527640, 2.558590, 2.602580, 2.633470, 2.677240, 2.708190, 2.752180, 2.783070, 2.826840, 2.857790, 2.901780, 2.932670, 2.976440, 3.007390, 3.051380, 3.082270, 3.126040, 3.16 };
  float phibinsOB6[192 + 1] = { -3.128150, -3.089680, -3.062670, -3.024370, -2.997250, -2.958780, -2.931770, -2.893470, -2.866350, -2.827880, -2.800870, -2.762570, -2.735450, -2.696980, -2.669970, -2.631670, -2.604550, -2.566080, -2.539070, -2.500770, -2.473650, -2.435180, -2.408170, -2.369870, -2.342750, -2.304280, -2.277270, -2.238970, -2.211850, -2.173380, -2.146370, -2.108070, -2.080950, -2.042480, -2.015470, -1.977170, -1.950050, -1.911580, -1.884570, -1.846270, -1.819150, -1.780680, -1.753670, -1.715370, -1.688250, -1.649780, -1.622770, -1.584470, -1.557350, -1.518880, -1.491870, -1.453570, -1.426450, -1.387980, -1.360970, -1.322670, -1.295550, -1.257080, -1.230070, -1.191770, -1.164650, -1.126180, -1.099170, -1.060870, -1.033750, -0.995284, -0.968271, -0.929973, -0.902854, -0.864385, -0.837372, -0.799074, -0.771954, -0.733485, -0.706472, -0.668174, -0.641054, -0.602585, -0.575572, -0.537274, -0.510155, -0.471686, -0.444673, -0.406375, -0.379255, -0.340786, -0.313773, -0.275475, -0.248355, -0.209886, -0.182873, -0.144575, -0.117455, -0.078987, -0.051973, -0.013676, 0.013444, 0.051913, 0.078926, 0.117224, 0.144344, 0.182813, 0.209826, 0.248124, 0.275244, 0.313713, 0.340726, 0.379024, 0.406143, 0.444612, 0.471625, 0.509923, 0.537043, 0.575512, 0.602525, 0.640823, 0.667943, 0.706412, 0.733425, 0.771723, 0.798842, 0.837311, 0.864324, 0.902622, 0.929742, 0.968211, 0.995224, 1.033520, 1.060640, 1.099110, 1.126120, 1.164420, 1.191540, 1.230010, 1.257020, 1.295320, 1.322440, 1.360910, 1.387920, 1.426220, 1.453340, 1.491810, 1.518820, 1.557120, 1.584240, 1.622710, 1.649720, 1.688020, 1.715140, 1.753610, 1.780620, 1.818920, 1.846040, 1.884510, 1.911520, 1.949820, 1.976940, 2.015410, 2.042420, 2.080720, 2.107840, 2.146310, 2.173320, 2.211620, 2.238740, 2.277210, 2.304220, 2.342520, 2.369640, 2.408110, 2.435120, 2.473420, 2.500540, 2.539010, 2.566020, 2.604320, 2.631440, 2.669910, 2.696920, 2.735220, 2.762340, 2.800810, 2.827820, 2.866120, 2.893240, 2.931710, 2.958720, 2.997020, 3.024140, 3.062610, 3.089620, 3.127920, 3.16 };
  float etabinsOB3[28 + 1] = { -1.477450, -1.408650, -1.335370, -1.257090, -1.173240, -1.083160, -0.986146, -0.881284, -0.768237, -0.646314, -0.515338, -0.375741, -0.228836, -0.076955, 0.076955, 0.228836, 0.375741, 0.515338, 0.646314, 0.768237, 0.881284, 0.986146, 1.083160, 1.173240, 1.257090, 1.335370, 1.408650, 1.477450, 1.54 };
  float etabinsOB4[28 + 1] = { -1.279420, -1.214230, -1.145270, -1.072190, -0.994647, -0.912252, -0.824650, -0.731369, -0.632506, -0.527868, -0.417671, -0.302482, -0.183282, -0.061466, 0.061466, 0.183282, 0.302482, 0.417671, 0.527868, 0.632506, 0.731369, 0.824650, 0.912252, 0.994647, 1.072190, 1.145270, 1.214230, 1.279420, 1.34 };
  float etabinsOB5[49 + 1] = { -1.483370, -1.445140, -1.405550, -1.364530, -1.321980, -1.277790, -1.231870, -1.184020, -1.134270, -1.082440, -1.028370, -0.971952, -0.913041, -0.851513, -0.787142, -0.720045, -0.650055, -0.577151, -0.501376, -0.422849, -0.341786, -0.258367, -0.173299, -0.086974, 0.000000, 0.086974, 0.173299, 0.258367, 0.341786, 0.422849, 0.501376, 0.577151, 0.650055, 0.720045, 0.787142, 0.851513, 0.913041, 0.971952, 1.028370, 1.082440, 1.134270, 1.184020, 1.231870, 1.277790, 1.321980, 1.364530, 1.405550, 1.445140, 1.483370, 1.53 };
  float etabinsOB6[49 + 1] = { -1.369600, -1.332400, -1.293960, -1.254200, -1.213050, -1.170430, -1.126260, -1.080380, -1.032840, -0.983488, -0.932225, -0.878966, -0.823626, -0.766135, -0.706331, -0.644375, -0.580163, -0.513723, -0.445132, -0.374519, -0.302079, -0.227946, -0.152690, -0.076567, 0.000000, 0.076567, 0.152690, 0.227946, 0.302079, 0.374519, 0.445132, 0.513723, 0.580163, 0.644375, 0.706331, 0.766135, 0.823626, 0.878966, 0.932225, 0.983488, 1.032840, 1.080380, 1.126260, 1.170430, 1.213050, 1.254200, 1.293960, 1.332400, 1.369600, 1.4 };

  TString mTriggerType[NTrigger] = { "ORBIT", "HB", "HBr", "HC", "PHYSICS", "PP", "CAL", "SOT", "EOT", "SOC", "EOC", "TF", "INT" };

  // General plots
  TH1F* mTFInfo; // count vs TF ID
  TH1D* mErrorPlots;
  TH2I* mErrorVsFeeid;
  TH2I* mTriggerVsFeeid;
  TH1D* mTriggerPlots;
  // TH1D* mInfoCanvas;//TODO: default, not implemented yet
  TH2I* mInfoCanvasComm;       // tmp object decidated to ITS commissioning
  TH2I* mInfoCanvasOBComm;     // tmp object decidated to ITS Outer Barral commissioning
  TH2Poly* mGeneralOccupancy;  // Max Occuapncy(chip/hic) in one stave
  TH2Poly* mGeneralNoisyPixel; // Noisy pixel number in one stave

  TText* mTextForShifter;
  TText* mTextForShifterOB;
  TText* mTextForShifter2;
  TText* mTextForShifterOB2;

  // Occupancy and hit-map
  THnSparseI* mStaveHitmap[7][48];
  TH2D* mDeadChipPos[7];
  TH2D* mAliveChipPos[7];
  TH2D* mTotalDeadChipPos;
  TH2D* mTotalAliveChipPos;
  TH2D* mChipStaveOccupancy[7];
  TH2I* mChipStaveEventHitCheck[7];
  TH1D* mOccupancyPlot[7];

  // Geometry decoder
  o2::its::GeometryTGeo* mGeom;
  std::string mGeomPath;
};
} // namespace o2::quality_control_modules::its

#endif // QC_MODULE_ITS_ITSFHRTASK_H
