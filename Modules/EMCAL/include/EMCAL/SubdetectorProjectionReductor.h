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
#ifndef QUALITYCONTROL_EMCAL_SUDETECTORPROJECTIONREDUCTOR_H
#define QUALITYCONTROL_EMCAL_SUDETECTORPROJECTIONREDUCTOR_H

#include "QualityControl/Reductor.h"

namespace o2::quality_control_modules::emcal
{

/// \brief Reductor of slices per subdetector.
///
/// Obtaining number of entries, mean, sigma and max for each slice
/// of the input histogram (subdetector dimension)
class SubdetectorProjectionReductor : public quality_control::postprocessing::Reductor
{
 public:
  SubdetectorProjectionReductor() = default;
  virtual ~SubdetectorProjectionReductor() = default;

  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  void update(TObject* obj) override;

 private:
  struct {
    Double_t mCountsTotal;
    Double_t mCountsEMCAL;
    Double_t mCountsDCAL;
    Double_t mMeanTotal;
    Double_t mMeanEMCAL;
    Double_t mMeanDCAL;
    Double_t mSigmaTotal;
    Double_t mSigmaEMCAL;
    Double_t mSigmaDCAL;
  } mStats;
};

} // namespace o2::quality_control_modules::emcal

#endif // QUALITYCONTROL_EMCAL_SUDETECTORPROJECTIONREDUCTOR_H