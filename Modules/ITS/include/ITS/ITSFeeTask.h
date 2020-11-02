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

  struct GBTDdw { //GBT diagnostic word

    union GBTBits {
      struct payloadBits {
        uint64_t flags2 : 36; /// 0:35  not defined yet
        uint64_t flags1 : 36; /// 36:71  not defined yet
        uint64_t id : 8;      /// 72:79  0xe0; Header Status Word (HSW) identifier
      } payload;

      uint8_t data8[16]; // 80 bits GBT word +  padding to 128 bits
    } ddwBits;
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
  void createErrorTFPlots(int barrel);
  void setPlotsFormat();
  void getEnableLayers();
  void getRunNumber(); //for ITS commissioning only
  void resetGeneralPlots();
  static constexpr int NLayer = 7;
  static constexpr int NLayerIB = 3;
  const int StaveBoundary[NLayer + 1] = { 0, 12, 28, 48, 72, 102, 144, 192 };
  std::array<bool, NLayer> mEnableLayers = { false };
  int mTimeFrameId = 0;
  int mNTrigger = 13;
  static constexpr int NError = 13;
  TString mErrorType[NError] = { "ORBIT", "HB", "HBr", "HC", "PHYSICS", "PP", "CAL", "SOT", "EOT", "SOC", "EOC", "TF", "INT" }; //TODO: replace by defined error flags

  TH1I* mTFInfo; //count vs TF ID
  TH2I* mErrorFlagVsFeeId;
  TH1I* mErrorFlag;
  //TH1D* mInfoCanvas;//TODO: default, not implemented yet
  std::string mRunNumberPath;
  std::string mRunNumber = "000000";
};

} // namespace o2::quality_control_modules::its

#endif // QC_MODULE_ITS_ITSFEETASK_H
