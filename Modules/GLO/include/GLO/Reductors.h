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

#ifndef QUALITYCONTROL_K0SFITREDUCTOR_H
#define QUALITYCONTROL_K0SFITREDUCTOR_H

#include "QualityControl/ReductorTObject.h"

#include <TF1.h>

namespace o2::quality_control_modules::glo
{

class K0sFitReductor final : public quality_control::postprocessing::ReductorTObject
{
  void* getBranchAddress() final { return &mStats; };
  const char* getBranchLeafList() final { return "mean/F:sigma/F"; };
  void update(TObject* obj) final;

 private:
  struct {
    Float_t mean{ 0. };
    Float_t sigma{ 0. };
  } mStats;
};

class MTCReductor final : public quality_control::postprocessing::ReductorTObject
{
  void* getBranchAddress() final { return &mStats; };
  const char* getBranchLeafList() final;
  void update(TObject* obj) final;

 private:
  struct {
    Float_t mtc{ 0. };
  } mStats;

  Float_t mPt{ 0 };
};

class PVITSReductor final : public quality_control::postprocessing::ReductorTObject
{
  void* getBranchAddress() final { return &mStats; };
  const char* getBranchLeafList() final;
  void update(TObject* obj) final;

 private:
  struct {
    Float_t pol0{ 0. };
    Float_t pol1{ 0. };
  } mStats;

  Float_t mR0{ 0 }, mR1{ 0 };
};

} // namespace o2::quality_control_modules::glo

#endif
