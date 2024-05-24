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
#include "EMCALBase/Geometry.h"
#include "EMCAL/OccupancyReductor.h"
#include "MathUtils/Utils.h"

using namespace o2::quality_control_modules::emcal;

OccupancyReductor::OccupancyReductor() : ReductorTObject(), mGeometry(nullptr), mStats()
{
  mGeometry = o2::emcal::Geometry::GetInstanceFromRunNumber(300000);
}

void* OccupancyReductor::getBranchAddress()
{
  return &mStats;
}

const char* OccupancyReductor::getBranchLeafList()
{
  return "totalCounts/D:average:RMS:smCounts[20]:averageSM[20]:RMSSM[20]";
}

void OccupancyReductor::update(TObject* obj)
{
  memset(mStats.mCountSM, 0, sizeof(double) * 20);
  memset(mStats.mAverageSM, 0, sizeof(double) * 20);
  memset(mStats.mRMSSM, 0, sizeof(double) * 20);
  std::array<o2::math_utils::StatAccumulator, 20> statAccumulatorsSM;
  TH2* digitOccupancyHistogram = static_cast<TH2*>(obj);
  o2::math_utils::StatAccumulator statAccumulatorTotal;
  mStats.mCountTotal = digitOccupancyHistogram->GetEntries();
  for (int icol = 0; icol < digitOccupancyHistogram->GetXaxis()->GetNbins(); icol++) {
    for (int irow = 0; irow < digitOccupancyHistogram->GetYaxis()->GetNbins(); irow++) {
      double count = digitOccupancyHistogram->GetBinContent(icol + 1, irow + 1);
      if (count) {
        statAccumulatorTotal.add(count);
        auto cellindex = mGeometry->GetCellIndexFromGlobalRowCol(irow, icol);
        auto smod = std::get<0>(cellindex);
        mStats.mCountSM[smod] += count;
        statAccumulatorsSM[smod].add(count);
      }
    }
  }
  auto [mean, rms] = statAccumulatorTotal.getMeanRMS2<double>();
  mStats.mAverage = mean;
  mStats.mRMS = rms;
  for (int ism = 0; ism < 20; ism++) {
    auto [meanSM, rmsSM] = statAccumulatorsSM[ism].getMeanRMS2<double>();
    mStats.mAverageSM[ism] = meanSM;
    mStats.mRMSSM[ism] = rmsSM;
  }
}