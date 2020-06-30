// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   THnSparseReductor.h
/// \author Ivan Ravasenga on the model from Piotr Konopka
///
#ifndef QUALITYCONTROL_THNSPARSEREDUCTOR_H
#define QUALITYCONTROL_THNSPARSEREDUCTOR_H

#include "QualityControl/Reductor.h"

namespace o2::quality_control_modules::common
{

/// \brief A Reductor which obtains the most popular characteristics of TH1.
///
/// A Reductor which obtains the most popular characteristics of TH1.
/// It produces a branch in the format: "mean/D:stddev:entries"
class THnSparseReductor : public quality_control::postprocessing::Reductor
{
 public:
  THnSparseReductor() = default;
  ~THnSparseReductor() = default;

  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  void update(TObject* obj) override;

 private:
  static constexpr int nDim = 5;
  struct {
    Double_t mean[nDim];// mean of each axis (up to 5 axes)
    Double_t stddev[nDim];// stddev of each axis (up to 5 axes)
    Double_t entries[nDim];
  } mStats;
};

} // namespace o2::quality_control_modules::common

#endif //QUALITYCONTROL_TH1REDUCTOR_H
