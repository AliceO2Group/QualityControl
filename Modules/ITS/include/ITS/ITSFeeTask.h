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
/// \file   ITSFeeTask.h
/// \author Jian Liu
/// \author Liang Zhang
///

#ifndef QC_MODULE_ITS_ITSFEETASK_H
#define QC_MODULE_ITS_ITSFEETASK_H

#include "QualityControl/TaskInterface.h"

#include <TH1.h>
#include <TH2.h>

class TH2I;
class TH1I;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::its
{

/// \brief ITS FEE task aiming at 100% online data integrity checking
class ITSFeeTask final : public TaskInterface
{
  struct GBTDiagnosticWord { //GBT diagnostic word
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
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  void setAxisTitle(TH1* object, const char* xTitle, const char* yTitle);
  void createFeePlots();
  void setPlotsFormat();
  void getRunNumber(); //for ITS commissioning only
  void resetGeneralPlots();
  static constexpr int NLayer = 7;
  static constexpr int NLayerIB = 3;
  static constexpr int NLanes = 28;
  static constexpr int NFees = 48 * 3 + 144 * 2;
  static constexpr int NFlags = 4;
  const int StaveBoundary[NLayer + 1] = { 0, 12, 28, 48, 72, 102, 144, 192 };
  int mTimeFrameId = 0;
  static constexpr int mNTrigger = 13;
  TString mTriggerType[mNTrigger] = { "ORBIT", "HB", "HBr", "HC", "PHYSICS", "PP", "CAL", "SOT", "EOT", "SOC", "EOC", "TF", "INT" };
  std::string mLaneStatusFlag[NFlags] = { "OK", "WARNING", "ERROR", "FAULT" }; //b00 OK, b01 WARNING, b10 ERROR, b11 FAULT

  TH1I* mTFInfo; //count vs TF ID
  TH2I* mTriggerVsFeeId;
  TH1I* mTrigger;
  TH2I* mLaneInfo;
  TH2I* mFlag1Check;         //include transmission_timeout, packet_overflow, lane_starts_violation
  TH2I* mIndexCheck;         //should be zero
  TH2I* mIdCheck;            //should be 0x : e4
  TH2I* mLaneStatus[NFlags]; //4 flags for each lane. 3/8/14 lane for each link. 3/2/2 link for each RU. TODO: remove the OK flag in these 4 flag plots, OK flag plot just used to debug.
  TH1I* mProcessingTime;
  //TH1D* mInfoCanvas;//TODO: default, not implemented yet
  std::string mRunNumberPath;
  std::string mRunNumber = "000000";
};

} // namespace o2::quality_control_modules::its

#endif // QC_MODULE_ITS_ITSFEETASK_H
