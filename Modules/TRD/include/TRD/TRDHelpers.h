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
/// \file   TRDHelpers.h
///

#ifndef QC_MODULE_TRD_TRDHELPER_H
#define QC_MODULE_TRD_TRDHELPER_H

#include "QualityControl/TaskInterface.h"
#include "TRDQC/StatusHelper.h"

class TH2F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::trd
{

class TRDHelpers
{
 public:
  TRDHelpers() = default;
  ~TRDHelpers() = default;

  static void drawTrdLayersGrid(TH2F* hist, int unitspersection);
  static void drawChamberStatusOnMapAndLayers(const std::array<int, o2::trd::constants::MAXCHAMBER>* ptrChamber, std::shared_ptr<TH2F> hcdist, std::array<std::shared_ptr<TH2F>, o2::trd::constants::NLAYER> mLayers, int unitspersection);
  static void drawChamberStatusMask(int hcstat, std::pair<float, float> x, std::pair<float, float> y, std::shared_ptr<TH2F> hist);
  static bool isHalfChamberMasked(int hcid, const std::array<int, o2::trd::constants::MAXCHAMBER>* ptrChamber);
 
 private:
  // Chamber status values definitions, used for masking
  static const int mConfiguredChamberStatus = 3;
  static const int mEmptyChamberStatus = 0;
  static const int mErrorChamberStatus = 99;
};

} // namespace o2::quality_control_modules::trd

#endif
