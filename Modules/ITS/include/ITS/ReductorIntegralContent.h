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
/// \file   ReductorIntegralContent.h
/// \author Artem Kotliarov
///
#ifndef QUALITYCONTROL_REDUCTORINTEGRALCONTENT_H
#define QUALITYCONTROL_REDUCTORINTEGRALCONTENT_H

// #include "QualityControl/Reductor.h"
#include "QualityControl/ReductorTObject.h"
#include <vector>

namespace o2::quality_control_modules::its
{

class ReductorIntegralContent : public quality_control::postprocessing::ReductorTObject

{
 public:
  ReductorIntegralContent() = default;
  virtual ~ReductorIntegralContent() = default;

  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  void update(TObject* obj) override;

  void setParams(Int_t Bins)
  {
    nBins = Bins;
  }

 private:
  int nBins = 13;
  int FeeBoundary[10] = { 0, 35, 83, 143, 191, 251, 293, 335, 383, 431 };
  struct mystat {
    // std::vector<Double_t> binContent;  // Bin content in a specified slice
    Double_t integral[100];
  };

  mystat mStats;
};
} // namespace o2::quality_control_modules::its

#endif // QUALITYCONTROL_REDUCTORINTEGRALCONTENT_H
