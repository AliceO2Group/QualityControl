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
/// \file   THnSparse5Reductor.h
/// \author Ivan Ravasenga on the model from Piotr Konopka
///
#ifndef QUALITYCONTROL_THNSPARSE5REDUCTOR_H
#define QUALITYCONTROL_THNSPARSE5REDUCTOR_H

#include "QualityControl/ReductorTObject.h"

namespace o2::quality_control_modules::common
{

/// \brief A Reductor which obtains the most popular characteristics of THnSparse up to 5 dimensions.
///
/// A Reductor which obtains the most popular characteristics of THnSparse up to 5 dimensions.
/// It produces a branch in the format: "mean[NDIM]/D:stddev[NDIM]:entries[NDIM] where NDIM=5"
class THnSparse5Reductor : public quality_control::postprocessing::ReductorTObject
{
 public:
  THnSparse5Reductor() = default;
  ~THnSparse5Reductor() = default;

  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  void update(TObject* obj) override;

 private:
  static constexpr int NDIM = 5;
  struct {
    Double_t mean[NDIM];   // mean of each axis (up to 5 axes)
    Double_t stddev[NDIM]; // stddev of each axis (up to 5 axes)
    Double_t entries[NDIM];
  } mStats;
};

} // namespace o2::quality_control_modules::common

#endif //QUALITYCONTROL_THNSPARSE5REDUCTOR_H
