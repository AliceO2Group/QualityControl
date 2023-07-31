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
/// \file   QcMFTDigitCheck.h
/// \author Tomas Herman
/// \author Guillermo Contreras
/// \author Katarina Krizkova Gajdosova
/// \author Diana Maria Krupova

#ifndef QC_MFT_DIGIT_CHECK_H
#define QC_MFT_DIGIT_CHECK_H

// Quality Control
#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::mft
{

/// \brief  MFT Digit Check
///
class QcMFTDigitCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  QcMFTDigitCheck() = default;
  /// Destructor
  ~QcMFTDigitCheck() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

 private:
  int mZoneThresholdMedium;
  int mZoneThresholdBad;

  // masked chips part
  bool mFirstCall;
  std::vector<int> mMaskedChips;
  std::vector<string> mChipMapName;

  void readMaskedChips(std::shared_ptr<MonitorObject> mo);
  void createMaskedChipsNames();

  // to form the name of the masked chips histograms
  int mHalf[936] = { 0 };
  int mDisk[936] = { 0 };
  int mFace[936] = { 0 };
  int mZone[936] = { 0 };
  int mSensor[936] = { 0 };
  int mTransID[936] = { 0 };
  int mLayer[936] = { 0 };
  int mLadder[936] = { 0 };
  float mX[936] = { 0 };
  float mY[936] = { 0 };
  void getChipMapData();

  ClassDefOverride(QcMFTDigitCheck, 2);
};

} // namespace o2::quality_control_modules::mft

#endif // QC_MFT_DIGIT_CHECK_H
