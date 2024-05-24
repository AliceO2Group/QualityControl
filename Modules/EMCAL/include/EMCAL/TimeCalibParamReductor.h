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

/// \class TimeCalibParamReductor
/// \brief Dedicated reductor for EMCAL time calibration param
/// \ingroup EMCALQCCReductors
/// \author Markus Fasel <markus.fasel@cern.ch>, Oak Ridge National Laboratory
/// \since November 1st, 2022
///
/// Produces entries:
/// - Channels with failed fit for Full acceptance/Subdetector/Supermodule
/// - Fraction channels with failed fit for Full acceptance/Subdetector/Supermodule
/// - Mean time shift
/// - Sigma time shift
class TimeCalibParamReductor : public quality_control::postprocessing::ReductorTObject
{
 public:
  /// \brief Constructor
  TimeCalibParamReductor();

  /// \brief Destructor
  virtual ~TimeCalibParamReductor() = default;

  /// \brief Get branch address of structure with data
  /// \return Branch address
  void* getBranchAddress() override;

  /// \brief Get list of variables providede by reductor
  /// \return List of variables (leaflist)
  const char* getBranchLeafList() override;

  /// \brief Extract information from time calibration histogram and fill observables
  /// \param obj Input object to get the data from
  void update(TObject* obj) override;

 private:
  /// \brief Check whether a certain position is within the PHOS region
  /// \param column Column number of the position
  /// \param row Row number of the position
  bool isPHOSRegion(int column, int row) const;

  o2::emcal::Geometry* mGeometry; ///< EMCAL geometry
  struct {
    Int_t mFailedParams;            ///< Total number of failed time calibration params
    Int_t mFailedParamsEMCAL;       ///< Number of failed time calibration params in EMCAL
    Int_t mFailedParamsDCAL;        ///< Number of failed time calibration params in DCAL
    Int_t mFailedParamSM[20];       ///< Number of failed time calibration params per supermodule
    Int_t mSupermoduleMaxFailed;    ///< Index of the supermodule with the largest amount of failed time calibration params
    Double_t mFractionFailed;       ///< Total fraction of failed time calibration params
    Double_t mFractionFailedEMCAL;  ///< Fraction of failed time calibration params in EMCAL
    Double_t mFractionFailedDCAL;   ///< Fraction of failed time calibration params in DCAL
    Double_t mFractionFailedSM[20]; ///< Fraction of failed time calibration params per supermodule
    Double_t mMeanShiftGood;        ///< Mean time caliration param of good cells
    Double_t mSigmaShiftGood;       ///< Sigma of the time calibration params of good cells
  } mStats;                         ///< Trending data point
};

} // namespace o2::quality_control_modules::emcal

#endif // QUALITYCONTROL_TH2REDUCTOR_