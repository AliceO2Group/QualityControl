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
/// \file   TH2Reductor.h
/// \author Piotr Konopka
///
#ifndef QUALITYCONTROL_EMCAL_DIGITOCCUPANCYREDUCTOR_H
#define QUALITYCONTROL_EMCAL_DIGITOCCUPANCYREDUCTOR_H

#include "QualityControl/Reductor.h"

namespace o2
{
namespace emcal
{
class Geometry;
}
} // namespace o2

namespace o2::quality_control_modules::emcal
{

/// \brief A Reductor which obtains the most popular characteristics of TH2.
///
/// A Reductor which obtains the most popular characteristics of TH2.
/// It produces a branch in the format: "sumw/D:sumw2:sumwx:sumwx2:sumwy:sumwy2:sumwxy:entries"
class DigitOccupancyReductor : public quality_control::postprocessing::Reductor
{
 public:
  DigitOccupancyReductor();
  virtual ~DigitOccupancyReductor() = default;

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

#endif //QUALITYCONTROL_TH2REDUCTOR_