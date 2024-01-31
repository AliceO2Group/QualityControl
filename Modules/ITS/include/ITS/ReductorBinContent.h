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
/// \file   ReductorBinContent.h
/// \author Artem Kotliarov
///
#ifndef QUALITYCONTROL_REDUCTORBINCONTENT_H
#define QUALITYCONTROL_REDUCTORBINCONTENT_H

#include "QualityControl/ReductorTObject.h"
#include <vector>

namespace o2::quality_control_modules::its
{

class ReductorBinContent : public quality_control::postprocessing::ReductorTObject
{
 public:
  ReductorBinContent() = default;
  virtual ~ReductorBinContent() = default;

  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  void update(TObject* obj) override;

  void setParams(Int_t Flags, Int_t Triggers)
  {
    nFlags = Flags;
    nTriggers = Triggers;
  }

 private:
  int nFlags = 3;
  int nTriggers = 13;

  struct mystat {
    // std::vector<Double_t> binContent;  // Bin content in a specified slice
    Double_t binContent[100];
    Double_t integral[100]; // Integral over all Fee I
  };

  mystat mStats;
};
} // namespace o2::quality_control_modules::its

#endif // QUALITYCONTROL_REDUCTORBINCONTENT_H
