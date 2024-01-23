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
/// \file   TH1ctpReductor.h
/// \author Lucia Anna Tarasovicova
///
#ifndef QUALITYCONTROL_TH1CTPREDUCTOR_H
#define QUALITYCONTROL_TH1CTPREDUCTOR_H

#include "QualityControl/Reductor.h"

namespace o2::quality_control_modules::ctp
{

/// \brief A Reductor which obtains the characteristics of TH1 including the bin contents.
/// Based on the example by Piotr Jan Konopka
/// It produces a branch in the format: "mean/D:stddev:entries:inputs[%i]:classes[%i]"
class TH1ctpReductor : public quality_control::postprocessing::Reductor
{
 public:
  TH1ctpReductor() = default;
  ~TH1ctpReductor() = default;

  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  void update(TObject* obj) override;

 private:
  static constexpr int nInputs = 48;
  static constexpr int nClasses = 64;
  struct {
    Double_t mean;
    Double_t stddev;
    Double_t entries;
    Double_t inputs[nInputs];
    Double_t classContent[nClasses];
  } mStats;
};

} // namespace o2::quality_control_modules::ctp

#endif // QUALITYCONTROL_TH1CTPREDUCTOR_H
