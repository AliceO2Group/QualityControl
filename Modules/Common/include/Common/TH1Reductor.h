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
/// \file   TH1Reductor.h
/// \author Piotr Konopka
///
#ifndef QUALITYCONTROL_TH1REDUCTOR_H
#define QUALITYCONTROL_TH1REDUCTOR_H

#include "QualityControl/ReductorTObject.h"

namespace o2::quality_control_modules::common
{

/// \brief A Reductor which obtains the most popular characteristics of TH1.
///
/// A Reductor which obtains the most popular characteristics of TH1.
/// It produces a branch in the format: "mean/D:stddev:entries"
class TH1Reductor : public quality_control::postprocessing::ReductorTObject
{
 public:
  TH1Reductor() = default;
  ~TH1Reductor() = default;

  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  void update(TObject* obj) override;

 private:
  struct {
    Double_t mean;
    Double_t stddev;
    Double_t entries;
  } mStats;
};

} // namespace o2::quality_control_modules::common

#endif //QUALITYCONTROL_TH1REDUCTOR_H
