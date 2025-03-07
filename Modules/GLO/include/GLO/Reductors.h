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

namespace o2::quality_control_modules::glo
{

class K0sFitReductor final : public quality_control::postprocessing::ReductorTObject
{
  void* getBranchAddress() final { return &mStats; };
  const char* getBranchLeafList() final { return "mean/D:sigma"; };
  void update(TObject* obj) override;

 private:
  struct {
    Double_t mean;
    Double_t stddev;
  } mStats;
};

} // namespace o2::quality_control_modules::glo

#endif
