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
#ifndef QUALITYCONTROL_EMCAL_OCCUPANCYTOFECREDUCTOR_H
#define QUALITYCONTROL_EMCAL_OCCUPANCYTOFECREDUCTOR_H

#include "QualityControl/ReductorTObject.h"
#include "EMCALBase/Mapper.h"

namespace o2
{
namespace emcal
{
class Geometry;
} // namespace emcal
} // namespace o2

namespace o2::quality_control_modules::emcal
{

/// \class OccupancyToFECReductor
/// \brief Reductor for Occupancy histograms exporting to FEC granularity
/// \ingroup EMCALQCCReductors
/// \author Markus Fasel <markus.fasel@cern.ch>, Oak Ridge National Laboratory
/// \since May 22nd, 2024
///
/// Extracting number of entries above 0 for each FEC area. In addition extracting
/// also mean and rms per FEC, and mean and rms over all non-0 objects
///
/// The reductor exports 2400 data points, therefore it should be used with care.
/// One should only enable the reductor manually in case trending per supermodule
/// indicates a problem.
class OccupancyToFECReductor : public quality_control::postprocessing::ReductorTObject
{
 public:
  /// \brief Constructor
  OccupancyToFECReductor();

  /// \brief Destructor
  virtual ~OccupancyToFECReductor() = default;

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
  o2::emcal::Geometry* mGeometry;    ///< EMCAL Geometry
  o2::emcal::MappingHandler mMapper; ///< EMCAL Mapper (to find the FEC ID from the online ID of a channel)
  struct {
    Double_t mCountFEC[800];   ///< Counts per FEC
    Double_t mAverageFEC[800]; ///< Average value per FEC
    Double_t mRMSFEC[800];     ///< RMS value per FEC
  } mStats;
};

} // namespace o2::quality_control_modules::emcal

#endif // QUALITYCONTROL_EMCAL_OCCUPANCYTOFECREDUCTOR_H