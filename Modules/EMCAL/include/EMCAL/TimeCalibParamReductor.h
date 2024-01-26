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

#ifndef QUALITYCONTROL_EMCAL_TIMECALIBPARAMREDUCTOR_H
#define QUALITYCONTROL_EMCAL_TIMECALIBPARAMREDUCTOR_H

#include "QualityControl/ReductorTObject.h"

namespace o2
{
namespace emcal
{
class Geometry;
}
} // namespace o2

namespace o2::quality_control_modules::emcal
{

/// \brief Dedicated reductor for EMCAL time calibration param
///
/// Produces entries:
/// - Channels with failed fit for Full acceptance/Subdetector/Supermodule
/// - Fraction channels with failed fit for Full acceptance/Subdetector/Supermodule
/// - Mean time shift
/// - Sigma time shift
class TimeCalibParamReductor : public quality_control::postprocessing::ReductorTObject
{
 public:
  TimeCalibParamReductor();
  virtual ~TimeCalibParamReductor() = default;

  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  void update(TObject* obj) override;

 private:
  bool isPHOSRegion(int column, int row) const;

  o2::emcal::Geometry* mGeometry;
  struct {
    Int_t mFailedParams;
    Int_t mFailedParamsEMCAL;
    Int_t mFailedParamsDCAL;
    Int_t mFailedParamSM[20];
    Int_t mSupermoduleMaxFailed;
    Double_t mFractionFailed;
    Double_t mFractionFailedEMCAL;
    Double_t mFractionFailedDCAL;
    Double_t mFractionFailedSM[20];
    Double_t mMeanShiftGood;
    Double_t mSigmaShiftGood;
  } mStats;
};

} // namespace o2::quality_control_modules::emcal

#endif // QUALITYCONTROL_TH2REDUCTOR_