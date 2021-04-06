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
/// \file   BasicReadoutHeaderQcTask.h
/// \author Tomas Herman
/// \author Guillermo Contreras
///

#ifndef QC_MODULE_MFT_MFTBasicReadoutHeaderQcTask_H
#define QC_MODULE_MFT_MFTBasicReadoutHeaderQcTask_H

// Quality Control
#include "QualityControl/TaskInterface.h"

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::mft
{

/// \brief MFT Basic Readout Header QC task
///
/// \author Tomas Herman
/// \author Guillermo Contreras
class BasicReadoutHeaderQcTask /*final*/ : public TaskInterface // todo add back the "final" when doxygen is fixed
{
  // addapted from ITSFeeTask
  struct MFTDDW { //GBT diagnostic word
    union {
      uint64_t word0 = 0x0;
      struct {
        uint64_t laneStatus : 50;
        uint16_t someInfo : 14;
      } laneBits;
    } laneWord;
    union {
      uint64_t word1 = 0x0;
      struct {
        uint16_t someInfo2 : 8;
        uint16_t id : 8;
        uint64_t padding : 48;
      } indexBits;
    } indexWord;
  };

 public:
  /// \brief Constructor
  BasicReadoutHeaderQcTask() = default;
  /// Destructor
  ~BasicReadoutHeaderQcTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  const int knRU = 80;      // number of RU
  const int kmaxRUid = 104; // max number to identify a RU
  int RUidMap[104];         // id start from zero
  std::unique_ptr<TH1F> mMFT_SummaryLaneStatus_H = nullptr;
  std::vector<std::unique_ptr<TH2F>> mMFT_IndividualLaneStatus_vH;

  // maps RUid into an idx for histograms
  void generateRUidMap();
  // unpacks RU ID into geometry information needed to name histograms
  void unpackRUid(int RUid, int& zone, int& plane, int& disc, int& half);
};

} // namespace o2::quality_control_modules::mft

#endif // QC_MODULE_MFT_MFTBasicReadoutHeaderQcTask_H
