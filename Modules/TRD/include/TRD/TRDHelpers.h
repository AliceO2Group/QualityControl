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

  void drawTrdLayersGrid(TH2F* hist, int unitspersection);
  void drawChamberStatusOn2D(std::shared_ptr<TH2F> hist, const std::array<int, o2::trd::constants::MAXCHAMBER>* ptrChamber);
  void drawChamberStatusOnLayers(std::array<std::shared_ptr<TH2F>, o2::trd::constants::NLAYER> mLayers, const std::array<int, o2::trd::constants::MAXCHAMBER>* ptrChamber, int unitspersection);

 private:
  void drawHashOnLayers(int layer, int hcid, int hcstat, int rowstart, int rowend, int unitspersection, std::array<std::shared_ptr<TH2F>, o2::trd::constants::NLAYER> mLayers);
  bool isHalfChamberMasked(int hcid, const std::array<int, o2::trd::constants::MAXCHAMBER>* ptrChamber);
  void setBoxChamberStatus(int hcstat, TLine* box[], std::shared_ptr<TH2F> hist);

  // Chamber status values definitions, used for masking
  int mConfiguredChamberStatus = 3;
  int mEmptyChamberStatus = 0;
  int mErrorChamberStatus = 99;
};

} // namespace o2::quality_control_modules::trd

#endif
