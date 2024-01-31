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
#ifndef QUALITYCONTROL_EMCAL_DIGITOCCUPANCYREDUCTOR_H
#define QUALITYCONTROL_EMCAL_DIGITOCCUPANCYREDUCTOR_H

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

/// \brief Reductor for Occupancy histograms
///
/// Extracting number of entries above 0 for each supermodule area.
class OccupancyReductor : public quality_control::postprocessing::ReductorTObject
{
 public:
  OccupancyReductor();
  virtual ~OccupancyReductor() = default;

  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  void update(TObject* obj) override;

 private:
  o2::emcal::Geometry* mGeometry;
  struct {
    Double_t mCountTotal;
    Double_t mCountSM[20];
  } mStats;
};

} // namespace o2::quality_control_modules::emcal

#endif // QUALITYCONTROL_EMCAL_DIGITOCCUPANCYREDUCTOR_H