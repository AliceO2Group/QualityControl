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
/// \file   CheckHitMap.h
/// \author Nicolò Jacazio nicolo.jacazio@cern.ch
/// \brief  Checker for the hit map hit obtained with the TaskDigits
///

#ifndef QC_MODULE_TOF_CHECKHITMAP_H
#define QC_MODULE_TOF_CHECKHITMAP_H

#include "QualityControl/CheckInterface.h"
#include "Base/MessagePad.h"
#include "TPad.h"

namespace o2::quality_control_modules::tof
{

class CheckHitMap : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  CheckHitMap() = default;
  /// Destructor
  ~CheckHitMap() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult) override;


 private:
  /// Reference hit map taken from the CCDB and translated into QC binning
  std::shared_ptr<TH2F> mHistoRefHitMap = nullptr;    /// TOF reference hit map
  std::shared_ptr<TH2F> mHistoBinaryHitMap = nullptr; /// TOF binary (yes or no) hit map
  std::vector<std::pair<int, int>> mHitMoreThanRef;
  std::vector<std::pair<int, int>> mRefMoreThanHit;

  /// Messages to print on the output PAD
  MessagePad mShifterMessages;
  /// Message regarding the PHOS module (hole)
  MessagePad mPhosModuleMessage{ "PHOS", 13.f, 38.f, 16.f, 53.f }; // Values corresponding to the PHOS hole
  /// Flag to enable or disable the check with respect to the reference map
  bool mEnableReferenceHitMap = true;
  /// Name of the Path to get on CCDB for the ref. map
  std::string mRefMapCcdbPath = "/TOF/Calib/FEELIGHT";
  /// Timestamp to get on CCDB for the ref. map
  int mRefMapTimestamp = -1;
  int mNWithHits = 0;                 /// Number of half strips with hits
  int mNEnabled = 0;                  /// Number of enabled half strips
  int mMaxHitMoreThanRef = 2;         /// Maximum number of Hits more than Ref that is accepted
  int mMaxRefMoreThanHit = 317;       /// Maximum number of Refs more than Hits that is accepted (usual 5%of enabled channels)
  bool mEnablePadPerMismatch = false; /// Flag to enable showing where the mismatch happens in the plot with TPads

  ClassDefOverride(CheckHitMap, 2);
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_CHECKHITMAP_H
