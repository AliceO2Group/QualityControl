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
/// \file   TH2ElecMapReductor.h
/// \author Andrea Ferrero
///
#ifndef QUALITYCONTROL_TH2ELECMAPREDUCTOR_H
#define QUALITYCONTROL_TH2ELECMAPREDUCTOR_H

#include "QualityControl/Reductor.h"
#include "MCHRawCommon/DataFormats.h"
#include "MCHRawElecMap/Mapper.h"
#include <limits>

namespace o2::quality_control_modules::muonchambers
{

/// \brief A Reductor which extracts the average features from a 2D map in electronics coordinates
///
/// A Reductor which extracts the average features from a 2D map in electronics coordinates

class TH2ElecMapReductor : public quality_control::postprocessing::Reductor
{
 public:
  TH2ElecMapReductor(float min = std::numeric_limits<float>::min(), float max = std::numeric_limits<float>::max());
  ~TH2ElecMapReductor() = default;

  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  void update(TObject* obj) override;

  float getChamberValue(int chid);
  float getDeValue(int deid, int cathode = 2);
  float getOrbits() { return meanOrbits; }
  int getNumPads(int deid, int cathode);
  int getNumPads(int deid)
  {
    return getNumPads(deid, 0) + getNumPads(deid, 1);
  }
  int getNumPadsBad(int deid, int cathode);
  int getNumPadsBad(int deid)
  {
    return getNumPadsBad(deid, 0) + getNumPadsBad(deid, 1);
  }
  int getNumPadsNoStat(int deid, int cathode);
  int getNumPadsNoStat(int deid)
  {
    return getNumPadsNoStat(deid, 0) + getNumPadsNoStat(deid, 1);
  }

 private:
  static constexpr int sDeNum{ 156 };

  int checkPadMapping(uint16_t feeId, uint8_t linkId, uint8_t eLinkId, o2::mch::raw::DualSampaChannelId channel, int& cid);

  o2::mch::raw::Elec2DetMapper mElec2DetMapper;
  o2::mch::raw::Det2ElecMapper mDet2ElecMapper;
  o2::mch::raw::FeeLink2SolarMapper mFeeLink2SolarMapper;
  o2::mch::raw::Solar2FeeLinkMapper mSolar2FeeLinkMapper;

  float mMin;
  float mMax;

  int deNumPads[2][sDeNum];
  int deNumPadsBad[2][sDeNum];
  int deNumPadsNoStat[2][sDeNum];
  float deValues[3][sDeNum];
  float chValues[10];
  float meanOrbits;
  float entries;
};

} // namespace o2::quality_control_modules::muonchambers

#endif // QUALITYCONTROL_TH2ELECMAPREDUCTOR_H
