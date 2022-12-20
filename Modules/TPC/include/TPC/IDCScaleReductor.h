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

//
// file   IDCScaleReductor.h
// author Marcel Lesch
//
#ifndef QC_MODULE_TPC_IDCSCALEREDUCTOR_H
#define QC_MODULE_TPC_IDCSCALEREDUCTOR_H

#include "QualityControl/Reductor.h"

namespace o2::quality_control_modules::tpc
{

/// \brief A Reductor of IDC 0 scale factors for TPC A and C side.

class IDCScaleReductor : public quality_control::postprocessing::Reductor
{
 public:
  IDCScaleReductor() = default;
  ~IDCScaleReductor() = default;

  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  void update(TObject* obj) override;

 private:
  struct {
    Float_t ScaleFactorASide;
    Float_t ScaleFactorCSide;
  } mIDC;

}; // class IDCScaleReductor : public quality_control::postprocessing::Reductor

} // namespace o2::quality_control_modules::tpc

#endif // QC_MODULE_TPC_IDCSCALEREDUCTOR_H
