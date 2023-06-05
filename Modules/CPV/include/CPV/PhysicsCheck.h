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
/// \file   PhysicsCheck.h
/// \author Sergey Evdokimov
///

#ifndef QC_MODULE_CPV_CPVPHYSICSCHECK_H
#define QC_MODULE_CPV_CPVPHYSICSCHECK_H

#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::cpv
{

/// \brief  CPV PhysicsCheck
/// \author Sergey Evdokimov
///
class PhysicsCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  PhysicsCheck() = default;
  /// Destructor
  ~PhysicsCheck() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override { return "TH1"; }

 private:
  int getRunNumberFromMO(std::shared_ptr<MonitorObject> mo);

  // amplitude check parameters
  float mAmplitudeRangeL[3] = { 20., 20., 20. };
  float mAmplitudeRangeR[3] = { 1000., 1000., 1000. };
  float mMinEventsToCheckAmplitude[3] = { 100, 100, 100 };
  float mMinAmplitudeMean[3] = { 5., 5., 5. };
  float mMaxAmplitudeMean[3] = { 200., 200., 200. };
  // cluster check parameters
  float mMinEventsToCheckClusters[3] = { 10., 10., 10. };
  float mMinClusterSize[3] = { 2., 2., 2. };
  float mMaxClusterSize[3] = { 5., 5., 5. };
  float mCluEnergyRangeL[3] = { 20., 20., 20. };
  float mCluEnergyRangeR[3] = { 1000., 1000., 1000. };
  float mMinCluEnergyMean[3] = { 5., 5., 5. };
  float mMaxCluEnergyMean[3] = { 200., 200., 200. };
  // digits check parameters
  int mMinEventsToCheckDigitMap[3] = { 10000, 10000, 10000 };
  int mNCold3GassiplexAllowed[3] = { 10, 10, 10 };
  int mNHot3GassiplexAllowed[3] = { 10, 10, 10 };
  float mHot3GassiplexCriterium[3] = { 10., 10., 10. };
  float mCold3GassiplexCriterium[3] = { 0.1, 0.1, 0.1 };
  float mHot3GassiplexOccurance[3] = { 0.1, 0.1, 0.1 };
  float mCold3GassiplexOccurance[3] = { 1.e-6, 1.e-6, 1.e-6 };
  float mMinDigitsPerEvent[3] = { 0., 0., 0 };
  float mMaxDigitsPerEvent[3] = { 300., 300., 300 };
  // errors check parameters
  float mErrorOccuranceThreshold[20] = {
    0.,
    0.,
    0.,
    0.,
    0.,
    0.,
    0.,
    0.,
    0.,
    0.,
    0.,
    0.,
    0.,
    0.,
    0.,
    0.,
    0.,
    0.,
    0.,
    0.
  };
  const char* mErrorLabel[20] = {
    "ok",
    "no payload",
    "rdh decod",
    "rdh invalid",
    "not cpv rdh",
    "no stopbit",
    "page not found",
    "0 offset to next",
    "payload incomplete",
    "no cpv header",
    "no cpv trailer",
    "cpv header invalid",
    "cpv trailer invalid",
    "segment header err",
    "row header error",
    "EOE header error",
    "pad error",
    "unknown word",
    "pad address",
    "wrong data format"
  };

  bool mIsConfigured = false; // had configure() been called already?

  ClassDefOverride(PhysicsCheck, 2);
};

} // namespace o2::quality_control_modules::cpv

#endif // QC_MODULE_CPV_CPVPHYSICSCHECK_H
