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
/// \file   TH2XlineReductor.h
/// \author Ivan Ravasenga on the model from Piotr Konopka
///
#ifndef QUALITYCONTROL_TH2XLINEREDUCTOR_H
#define QUALITYCONTROL_TH2XLINEREDUCTOR_H

#include "QualityControl/Reductor.h"
#include <vector>

namespace o2::quality_control_modules::its
{

/// \brief A Reductor which obtains specific characteristics of TH2: mean and stddev of bin contents
/// for each y bin (each row of the TH2)
///
/// A Reductor which obtains specific characteristics of TH2.
class TH2XlineReductor : public quality_control::postprocessing::Reductor
{
 public:
  TH2XlineReductor() = default;
  ~TH2XlineReductor() = default;

  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  void update(TObject* obj) override;

 private:
  static constexpr int NDIM = 20;
  struct mystat {
    Double_t mean[NDIM];        //mean of the bin contents of each row (1 value per row)
    Double_t stddev[NDIM];      //stddev of the bin contents of each row (1 value per row)
    Double_t entries[NDIM];     //entries of each row (1 value per row)
    Double_t mean_scaled[NDIM]; //mean scaled with number of active pixels in a stave to get the occupancy
  };
  mystat mStats;
};

} // namespace o2::quality_control_modules::its

#endif //QUALITYCONTROL_TH2XLINEREDUCTOR_H
