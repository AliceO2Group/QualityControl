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

#include <array>
#include "TH2.h"
#include "EMCALReconstruction/Channel.h"
#include "EMCALBase/Geometry.h"
#include "EMCAL/OccupancyToFECReductor.h"
#include "MathUtils/Utils.h"

using namespace o2::quality_control_modules::emcal;

OccupancyToFECReductor::OccupancyToFECReductor() : ReductorTObject(), mGeometry(nullptr), mStats()
{
  mGeometry = o2::emcal::Geometry::GetInstanceFromRunNumber(300000);
}

void* OccupancyToFECReductor::getBranchAddress()
{
  return &mStats;
}

const char* OccupancyToFECReductor::getBranchLeafList()
{
  return "fecCounts[800]/D:averageFEC[800]:RMSFEC[8000]";
}

void OccupancyToFECReductor::update(TObject* obj)
{
  memset(mStats.mCountFEC, 0, sizeof(double) * 800);
  memset(mStats.mAverageFEC, 0, sizeof(double) * 800);
  memset(mStats.mRMSFEC, 0, sizeof(double) * 800);
  std::array<o2::math_utils::StatAccumulator, 800> statAccumulatorsSM;
  TH2* digitOccupancyHistogram = static_cast<TH2*>(obj);
  o2::math_utils::StatAccumulator statAccumulatorTotal;
  for (int icol = 0; icol < digitOccupancyHistogram->GetXaxis()->GetNbins(); icol++) {
    for (int irow = 0; irow < digitOccupancyHistogram->GetYaxis()->GetNbins(); irow++) {
      double count = digitOccupancyHistogram->GetBinContent(icol + 1, irow + 1);
      if (count) {
        statAccumulatorTotal.add(count);
        auto [smod, mod, phimod, etamod] = mGeometry->GetCellIndexFromGlobalRowCol(irow, icol);
        auto absCellID = mGeometry->GetCellAbsIDFromGlobalRowCol(irow, icol);
        auto [ddl, onlinerow, onlinecol] = mGeometry->getOnlineID(absCellID);
        auto hwaddress = mMapper.getMappingForDDL(ddl).getHardwareAddress(onlinerow, onlinecol, o2::emcal::ChannelType_t::HIGH_GAIN);
        auto localfec = mMapper.getFEEForChannelInDDL(ddl, o2::emcal::Channel::getFecIndexFromHwAddress(hwaddress), o2::emcal::Channel::getBranchIndexFromHwAddress(hwaddress));
        auto globalfec = smod * 40 + localfec;
        mStats.mCountFEC[globalfec] += count;
        statAccumulatorsSM[globalfec].add(count);
      }
    }
  }
  auto [mean, rms] = statAccumulatorTotal.getMeanRMS2<double>();
  for (int ism = 0; ism < 800; ism++) {
    auto [meanSM, rmsSM] = statAccumulatorsSM[ism].getMeanRMS2<double>();
    mStats.mAverageFEC[ism] = meanSM;
    mStats.mRMSFEC[ism] = rmsSM;
  }
}