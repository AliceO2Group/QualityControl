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
/// \file   ITSFeeTask.h
/// \author Jian Liu
/// \author Liang Zhang
/// \author Pietro Fecchio
/// \author Antonio Palasciano
///

#ifndef QC_MODULE_ITS_ITSFEETASK_H
#define QC_MODULE_ITS_ITSFEETASK_H

#include "QualityControl/TaskInterface.h"
#include "Headers/RAWDataHeader.h"
#include "Headers/RDHAny.h"
#include "DetectorsRaw/RDHUtils.h"

#include <TH1.h>
#include <TH2.h>
#include <TH2Poly.h>
#include "TMath.h"
#include <TLine.h>
#include <TText.h>
#include <TLatex.h>
#include <TLine.h>
#include <TLegend.h>

class TH2I;
class TH1I;
class TH2F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::its
{

/// \brief ITS FEE task aiming at 100% online data integrity checking
class ITSFeeTask final : public TaskInterface
{
  struct GBTDiagnosticWord { // GBT diagnostic word
    union {
      uint64_t word0 = 0x0;
      struct {
        uint64_t laneStatus : 56;
        uint16_t zero0 : 8;
      } laneBits;
    } laneWord;
    union {
      uint64_t word1 = 0x0;
      struct {
        uint8_t flag1 : 4;
        uint8_t index : 4;
        uint8_t id : 8;
        uint64_t padding : 48;
      } indexBits;
    } indexWord;
  };

 public:
  /// \brief Constructor
  ITSFeeTask();
  /// Destructor
  ~ITSFeeTask() override;

  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  void getParameters(); // get Task parameters from json file
  void setAxisTitle(TH1* object, const char* xTitle, const char* yTitle);
  void createFeePlots();
  void getStavePoint(int layer, int stave, double* px, double* py); // prepare for fill TH2Poly, get all point for add TH2Poly bin
  void setPlotsFormat();
  void drawLayerName(TH2* histo2D);
  void resetGeneralPlots();
  void resetLanePlotsAndCounters();
  static constexpr int NLayer = 7;
  const int NStaves[NLayer] = { 12, 16, 20, 24, 30, 42, 48 };
  static constexpr int NLayerIB = 3;
  static constexpr int NLanesMax = 28;
  static constexpr int NFees = 48 * 3 + 144 * 2;
  static constexpr int NFlags = 3;
  static constexpr int NTrigger = 13;
  const int StaveBoundary[NLayer + 1] = { 0, 12, 28, 48, 72, 102, 144, 192 };
  const int NLanePerStaveLayer[NLayer] = { 9, 9, 9, 16, 16, 28, 28 };
  const int LayerBoundaryFEE[NLayer - 1] = { 35, 83, 143, 191, 251, 335 };
  const float StartAngle[7] = { 16.997 / 360 * (TMath::Pi() * 2.), 17.504 / 360 * (TMath::Pi() * 2.), 17.337 / 360 * (TMath::Pi() * 2.), 8.75 / 360 * (TMath::Pi() * 2.), 7 / 360 * (TMath::Pi() * 2.), 5.27 / 360 * (TMath::Pi() * 2.), 4.61 / 360 * (TMath::Pi() * 2.) }; // start angle of first stave in each layer
  const float MidPointRad[7] = { 23.49, 31.586, 39.341, 197.598, 246.944, 345.348, 394.883 };
  const int laneMax[NLayer] = { 108, 144, 180, 384, 480, 1176, 1344 };
  const int NLanesIB = 432, NLanesML = 864, NLanesOL = 2520, NLanesTotal = 3816;
  const int lanesPerFeeId[NLayer] = { 3, 3, 3, 8, 8, 14, 14 };
  const int feePerLayer[NLayer] = { 36, 48, 60, 48, 60, 84, 96 };
  const int StavePerLayer[NLayer] = { 12, 16, 20, 24, 30, 42, 48 };
  const int feePerStave[NLayer] = { 3, 3, 3, 2, 2, 2, 2 };
  const int feeBoundary[NLayer] = { 0, 35, 83, 143, 191, 251, 335 };
  const int indexFeeLow[NLayer] = { 0, 3, 6, 3, 17, 0, 14 };
  const int indexFeeUp[NLayer] = { 3, 6, 9, 11, 25, 14, 28 };
  int mTimeFrameId = 0;
  TString mTriggerType[NTrigger] = { "ORBIT", "HB", "HBr", "HC", "PHYSICS", "PP", "CAL", "SOT", "EOT", "SOC", "EOC", "TF", "INT" };
  std::string mLaneStatusFlag[NFlags] = { "WARNING", "ERROR", "FAULT" }; // b00 OK, b01 WARNING, b10 ERROR, b11 FAULT

  int mStatusFlagNumber[7][48][28][3] = { { { 0 } } }; //[iLayer][iStave][iLane][iLaneStatusFlag]
  int mStatusSummaryLayerNumber[7][3] = { { 0 } };     //[iLayer][iflag]
  int mStatusSummaryNumber[4][3] = { { 0 } };          //[summary][iflag] ---> Global, IB, ML, OL

  // parameters taken from the .json
  int mNPayloadSizeBins = 4096;
  bool mResetLaneStatus = false;
  bool mResetPayload = false;

  TH1I* mTFInfo; // count vs TF ID
  TH2I* mTriggerVsFeeId;
  TH1I* mTrigger;
  TH2I* mLaneInfo;
  TH2I* mFlag1Check; // include transmission_timeout, packet_overflow, lane_starts_violation
  TH2I* mIndexCheck; // should be zero
  TH2I* mIdCheck;    // should be 0x : e4
  TH2I* mRDHSummary;
  TH2I* mLaneStatus[NFlags]; // 4 flags for each lane. 3/8/14 lane for each link. 3/2/2 link for each RU. TODO: remove the OK flag in these 4 flag plots, OK flag plot just used to debug.
  TH2I* mLaneStatusCumulative[NFlags];
  TH2Poly* mLaneStatusOverview[NFlags] = { 0x0 };
  TH1I* mLaneStatusSummary[NLayer];
  TH1D* mLaneStatusSummaryIB;
  TH1D* mLaneStatusSummaryML;
  TH1D* mLaneStatusSummaryOL;
  TH1D* mLaneStatusSummaryGlobal;
  TH1I* mProcessingTime;
  TH2F* mPayloadSize; // average payload size vs linkID
  // TH1D* mInfoCanvas;//TODO: default, not implemented yet
  std::string mRunNumberPath;
  std::string mRunNumber = "000000";
};

} // namespace o2::quality_control_modules::its

#endif // QC_MODULE_ITS_ITSFEETASK_H
