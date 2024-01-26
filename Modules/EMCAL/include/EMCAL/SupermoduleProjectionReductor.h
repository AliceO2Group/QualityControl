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
#ifndef QUALITYCONTROL_EMCAL_SUPERMODULEPROJECTIONREDUCTOR_H
#define QUALITYCONTROL_EMCAL_SUPERMODULEPROJECTIONREDUCTOR_H

#include "QualityControl/ReductorTObject.h"
namespace o2::quality_control_modules::emcal
{

/// \brief Reductor of slices per supermodule.
///
/// Obtaining number of entries, mean, sigma and max for each slice
/// of the input histogram (supermodule dimension)
class SupermoduleProjectionReductorBase : public quality_control::postprocessing::ReductorTObject
{
 public:
  SupermoduleProjectionReductorBase() = default;
  virtual ~SupermoduleProjectionReductorBase() = default;

  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  void update(TObject* obj) override;

  void setSupermoduleAxisX() { mSupermoduleAxisX = true; }
  void setSupermoduleAxisY() { mSupermoduleAxisX = false; }

 private:
  struct {
    Double_t mCountSM[20];
    Double_t mMeanSM[20];
    Double_t mSigmaSM[20];
    Double_t mMaxSM[20];
  } mStats;
  Bool_t mSupermoduleAxisX = true;
};

class SupermoduleProjectionReductorX : public SupermoduleProjectionReductorBase
{
 public:
  SupermoduleProjectionReductorX() : SupermoduleProjectionReductorBase() { setSupermoduleAxisX(); }
};

class SupermoduleProjectionReductorY : public SupermoduleProjectionReductorBase
{
 public:
  SupermoduleProjectionReductorY() : SupermoduleProjectionReductorBase() { setSupermoduleAxisY(); }
};

} // namespace o2::quality_control_modules::emcal

#endif // QUALITYCONTROL_EMCAL_SUPERMODULEPROJECTIONREDUCTOR_H