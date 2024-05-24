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
#ifndef QUALITYCONTROL_EMCAL_OCCUPANCYREDUCTOR_H
#define QUALITYCONTROL_EMCAL_OCCUPANCYREDUCTOR_H

#include "QualityControl/ReductorTObject.h"

namespace o2
{
namespace emcal
{
class Geometry;
} // namespace emcal
} // namespace o2

namespace o2::quality_control_modules::emcal
{

/// \class OccupancyReductor
/// \brief Reductor for Occupancy histograms
/// \ingroup EMCALQCCReductors
/// \author Cristina Terrevoli
/// \since November 2nd, 2022
///
/// Extracting number of entries above 0 for each supermodule area. In addition extracting
/// also mean and rms per supermodule, and mean and rms over all non-0 objects
class OccupancyReductor : public quality_control::postprocessing::ReductorTObject
{
 public:
  /// \brief Constructor
  OccupancyReductor();

  /// \brief Destructor
  virtual ~OccupancyReductor() = default;

  /// \brief Get branch address of structure with data
  /// \return Branch address
  void* getBranchAddress() override;

  /// \brief Get list of variables providede by reductor
  /// \return List of variables (leaflist)
  const char* getBranchLeafList() override;

  /// \brief Calculate observables per SM and update tree
  /// \param obj Input object to get the data from
  void update(TObject* obj) override;

 private:
  o2::emcal::Geometry* mGeometry; ///< EMCAL Geometry
  struct {
    Double_t mCountTotal;    ///< Total counts
    Double_t mAverage;       ///< Average value
    Double_t mRMS;           ///< RMS value
    Double_t mCountSM[20];   ///< Counts per SM
    Double_t mAverageSM[20]; ///< Average value per SM
    Double_t mRMSSM[20];     ///< RMS value per SM
  } mStats;
};

} // namespace o2::quality_control_modules::emcal

#endif // QUALITYCONTROL_EMCAL_OCCUPANCYREDUCTOR_H