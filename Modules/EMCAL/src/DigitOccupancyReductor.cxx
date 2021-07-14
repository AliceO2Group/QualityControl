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

#include "EMCALBase/Geometry.h"
#include "EMCAL/DigitOccupancyReductor.h"
#include "TH2.h"

using namespace o2::quality_control_modules::emcal;

DigitOccupancyReductor::DigitOccupancyReductor() : Reductor(), mGeometry(nullptr), mStats()
{
  mGeometry = o2::emcal::Geometry::GetInstanceFromRunNumber(300000);
}

void* DigitOccupancyReductor::getBranchAddress()
{
  return &mStats;
}

const char* DigitOccupancyReductor::getBranchLeafList()
{
  return "totalCounts/D:smCounts[20]";
}

void DigitOccupancyReductor::update(TObject* obj)
{
  memset(mStats.mCountSM, 0, sizeof(double) * 20);
  TH2* digitOccupancyHistogram = static_cast<TH2*>(obj);
  mStats.mCountTotal = digitOccupancyHistogram->GetEntries();
  for (int icol = 0; icol < digitOccupancyHistogram->GetXaxis()->GetNbins(); icol++) {
    for (int irow = 0; irow < digitOccupancyHistogram->GetYaxis()->GetNbins(); irow++) {
      double count = digitOccupancyHistogram->GetBinContent(icol + 1, irow + 1);
      if (count) {
        auto cellindex = mGeometry->GetCellIndexFromGlobalRowCol(irow, icol); // To implement:Cell abs ID from glob row / col
        auto smod = std::get<0>(cellindex);
        mStats.mCountSM[smod] += count;
      }
    }
  }
}