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

#ifndef QUALITYCONTROL_EMCAL_BADCHANNELMAPREDUCTOR_H
#define QUALITYCONTROL_EMCAL_BADCHANNELMAPREDUCTOR_H

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

/// \brief Dedicated reductor for EMCAL bad channel map
///
/// Produces entries:
/// - Bad/Dead channels for Full acceptance/Subdetector/Supermodule
/// - Fraction Bad/Dead channels for Full acceptance/Subdetector/Supermodule
class BadChannelMapReductor : public quality_control::postprocessing::Reductor
{
 public:
  BadChannelMapReductor();
  virtual ~BadChannelMapReductor() = default;

  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  void update(TObject* obj) override;

 private:
  bool isPHOSRegion(int column, int row) const;

  o2::emcal::Geometry* mGeometry;
  struct {
    Int_t mBadChannelsTotal;
    Int_t mDeadChannelsTotal;
    Int_t mBadChannelsEMCAL;
    Int_t mBadChannelsDCAL;
    Int_t mDeadChannelsEMCAL;
    Int_t mDeadChannelsDCAL;
    Int_t mBadChannelsSM[20];
    Int_t mDeadChannelsSM[20];
    Int_t mSupermoduleMaxBad;
    Int_t mSupermoduleMaxDead;
    Double_t mFractionBadTotal;
    Double_t mFractionDeadTotal;
    Double_t mFractionBadEMCAL;
    Double_t mFractionBadDCAL;
    Double_t mFractionDeadEMCAL;
    Double_t mFractionDeadDCAL;
    Double_t mFractionBadSM[20];
    Double_t mFractionDeadSM[20];
  } mStats;
};

} // namespace o2::quality_control_modules::emcal

#endif // QUALITYCONTROL_TH2REDUCTOR_