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
/// \file   TRDReductor.h
///
#ifndef TRD_REDUCTOR_H
#define TRD_REDUCTOR_H

#include "QualityControl/Reductor.h"

namespace o2::quality_control_modules::trd
{

/// \brief A Reductor which fetch PulseHeight mo (hist) and find position of t0 peak

class TRDReductor : public quality_control::postprocessing::Reductor
{
 public:
  TRDReductor() = default;
  ~TRDReductor() = default;

  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  void update(TObject* obj) override;

 private:
  struct {
    Double_t t0peak;
    Double_t chi2byNDF;
    Int_t BinWithMaxContent;
  } mStats;
};

} // namespace o2::quality_control_modules::trd

#endif // TRD_REDUCTOR_H