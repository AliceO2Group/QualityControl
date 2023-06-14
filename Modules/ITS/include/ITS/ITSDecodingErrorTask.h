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
/// \file   ITSDecodingErrorTask.h
/// \author Zhen Zhang
///

#ifndef QC_MODULE_ITS_ITSDECODINGERRORTASK_H
#define QC_MODULE_ITS_ITSDECODINGERRORTASK_H

#include "QualityControl/TaskInterface.h"

#include <TH1.h>
#include <TH2.h>

class TH2D;
class TH1D;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::its
{

/// \brief ITS FEE task aiming at 100% online data integrity checking
class ITSDecodingErrorTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  ITSDecodingErrorTask();
  /// Destructor
  ~ITSDecodingErrorTask() override;

  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  int mTFCount = 0;
  void getParameters(); // get Task parameters from json file
  void setAxisTitle(TH1* object, const char* xTitle, const char* yTitle);
  void createDecodingPlots();
  void getStavePoint(int layer, int stave, double* px, double* py); // prepare for fill TH2Poly, get all point for add TH2Poly bin
  void setPlotsFormat();
  void resetGeneralPlots();
  static constexpr int NLayer = 7;
  static constexpr int NLayerIB = 3;
  const int StaveBoundary[NLayer + 1] = { 0, 12, 28, 48, 72, 102, 144, 192 };
  const int ChipBoundary[NLayer + 1] = { 0, 108, 252, 432, 3120, 6480, 14712, 24120 };
  static constexpr int NFees = 48 * 3 + 144 * 2;

  TH1D* mChipErrorPlots;
  TH1D* mLinkErrorPlots;
  TH2D* mChipErrorVsChipid[7]; // chip ErrorVsChipid
  TH2D* mLinkErrorVsFeeid;     // link ErrorVsFeeid
  TH2D* mChipErrorVsFeeid;     // chip ErrorVsFeeid
};

} // namespace o2::quality_control_modules::its

#endif // QC_MODULE_ITS_ITSDECODINGERRORTASK_H
