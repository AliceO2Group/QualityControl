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
// file   ROCReductor.h
// author Cindy Mordasini
// author Marcel Lesch
//
#ifndef QC_MODULE_TPC_ROCREDUCTOR_H
#define QC_MODULE_TPC_ROCREDUCTOR_H

#include "QualityControl/ReductorTObject.h"
#if __has_include("TPCBase/CalDet.h")
#include "TPCBase/CalDet.h"
#else
#include "TPCBaseRecSim/CalDet.h"
#endif
//#include "CCDB/TObjectWrapper.h"

namespace o2::quality_control_modules::tpc
{

/// \brief A Reductor of ROC, stores mean, standard deviation and median for each ROC.
///
/// A Reductor of ROC, stores entries, mean, standard deviation, median and rms for each ROC.
/// It produces a branch in the format: "entries[72]/I:mean[72]/F:stddev[72]:median[72]:rms[72]"
class ROCReductor : public quality_control::postprocessing::ReductorTObject
{
 public:
  ROCReductor() = default;
  ~ROCReductor() = default;

  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  void update(TObject* obj) override;

 private:
  struct {
    Int_t entries[72];
    Float_t mean[72];
    Float_t stddev[72];
    Float_t median[72];
    Float_t rms[72];
  } mCalPad;

}; // class ROCReductor : public quality_control::postprocessing::Reductor

} // namespace o2::quality_control_modules::tpc

#endif // QC_MODULE_TPC_ROCREDUCTOR_H
