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
///

#ifndef QC_MODULE_ITS_ITSFEETASK_H
#define QC_MODULE_ITS_ITSFEETASK_H

#include "QualityControl/TaskInterface.h"

#include <TText.h>
#include <TH1.h>
#include <TH2.h>
#include <TPaveText.h>

class TH2I;
class TH1F;

using namespace o2::quality_control::core;
using namespace o2::framework;
using namespace o2::header;

namespace o2::quality_control_modules::its
{

/// \brief ITS FEE task aiming at 100% online data integrity checking
class ITSFeeTask final : public TaskInterface
{

  struct GBTDdw { //GBT diagnostic word

    union GBTBits {
      struct payloadBits {
        uint64_t flags2 : 36; /// 0:35  not defined yet
        uint64_t flags1 : 36; /// 36:71  not defined yet
        uint64_t id : 8;      /// 72:79  0xe0; Header Status Word (HSW) identifier
      } payload;

      uint8_t data8[16]; // 80 bits GBT word + optional padding to 128 bits
    } ddwBits;
  };

 public:
  /// \brief Constructor
  ITSFeeTask();
  /// Destructor
  ~ITSFeeTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  void setAxisTitle(TH1* object, const char* xTitle, const char* yTitle);
  void createGeneralPlots(int barrel); //create General PLots for ITS barrels
  void createErrorTriggerPlots();
  void setPlotsFormat();
  void getEnableLayers();
  void resetGeneralPlots();
  static constexpr int NLayer = 7;
  static constexpr int NLayerIB = 3;
  const int StaveBoundary[NLayer + 1] = { 0, 12, 28, 48, 72, 102, 144, 192 };
  std::array<bool, NLayer> mEnableLayers = { false };
  int mTimeFrameId = 0;
  uint32_t mTriggerTypeCount[13] = { 0 };
  int mNTrigger = 13;
  static constexpr int NTrigger = 13;
  TString mTriggerType[NTrigger] = { "ORBIT", "HB", "HBr", "HC", "PHYSICS", "PP", "CAL", "SOT", "EOT", "SOC", "EOC", "TF", "INT" };
  TH1F* mTFInfo; //count vs TF ID
  TH2I* mTriggerVsFeeid;
  TH1D* mTriggerPlots;
  //TH1D* mInfoCanvas;//TODO: default, not implemented yet
  std::string mRunNumberPath;
  std::string mRunNumber = "000000";
};

} // namespace o2::quality_control_modules::its

#endif // QC_MODULE_ITS_ITSFEETASK_H
