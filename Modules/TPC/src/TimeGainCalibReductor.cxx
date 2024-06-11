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
/// \file   TimeGainCalibReductor.cxx
/// \author Marcel Lesch
///

#include "TPC/TimeGainCalibReductor.h"
#include "QualityControl/QcInfoLogger.h"
#include <DataFormatsTPC/CalibdEdxCorrection.h>

namespace o2::quality_control_modules::tpc
{

void* TimeGainCalibReductor::getBranchAddress()
{
  return &mStats;
}

const char* TimeGainCalibReductor::getBranchLeafList()
{
  return "meanEntries[2][5]/F:stddevEntries[2][5]:meanGain[2][5]:diffCorrectionTgl[2][5]";
}

bool TimeGainCalibReductor::update(ConditionRetriever& retriever)
{
  // Protection in case something changes in the enums, else getBranchLeafList() is not properly set up
  if (o2::tpc::CHARGETYPES != 2 || o2::tpc::GEMSTACKSPERSECTOR != 4) {
    ILOG(Error, Support) << "Error in TimeGainCalibReductor: TPC setups not matching expected default" << ENDM;
  }

  if (auto timeGainCalib = retriever.retrieve<o2::tpc::CalibdEdxCorrection>()) {

    constexpr std::array<o2::tpc::ChargeType, 2> charges = { o2::tpc::Max, o2::tpc::Tot };
    constexpr std::array<o2::tpc::GEMstack, 4> stacks = { o2::tpc::IROCgem, o2::tpc::OROC1gem, o2::tpc::OROC2gem, o2::tpc::OROC3gem };

    for (int iCharge = 0; iCharge < o2::tpc::CHARGETYPES; iCharge++) {
      auto charge = charges[iCharge];

      // Mean and stddev of entries over all sectors and stacks
      float sum[o2::tpc::GEMSTACKSPERSECTOR + 1] = { 0. };
      float sumSquare[o2::tpc::GEMSTACKSPERSECTOR + 1] = { 0. };
      float counter[o2::tpc::GEMSTACKSPERSECTOR + 1] = { 0. };

      for (int sector = 0; sector < o2::tpc::SECTORSPERSIDE * o2::tpc::SIDES; ++sector) {
        for (int iStack = 0; iStack < o2::tpc::GEMSTACKSPERSECTOR; iStack++) {
          auto stack = stacks[iStack];
          float entry = (float)(timeGainCalib->getEntries(o2::tpc::StackID{ sector, stack }, charge));

          // Quantity per stack type
          sum[iStack] += entry;
          sumSquare[iStack] += entry * entry;
          counter[iStack] += 1.;

          // Quantity averaged over all stack types
          sum[o2::tpc::GEMSTACKSPERSECTOR] += entry;
          sumSquare[o2::tpc::GEMSTACKSPERSECTOR] += entry * entry;
          counter[o2::tpc::GEMSTACKSPERSECTOR] += 1.;
        }
      }

      for (int iStack = 0; iStack < o2::tpc::GEMSTACKSPERSECTOR + 1; iStack++) {
        if (counter[iStack] != 0) {
          mStats.meanEntries[iCharge][iStack] = sum[iStack] / counter[iStack];
        } else {
          mStats.meanEntries[iCharge][iStack] = 0.;
        }
        if (counter[iStack] > 1) {
          mStats.stddevEntries[iCharge][iStack] = (sumSquare[iStack] - (sum[iStack] * sum[iStack]) / counter[iStack]) / (counter[iStack] - 1.);
        } else {
          mStats.stddevEntries[iCharge][iStack] = 0.;
        }
      }

      double tgl = 1.;
      // Gain averaged over all sectors for a stack type
      for (int iStack = 0; iStack < o2::tpc::GEMSTACKSPERSECTOR; iStack++) {
        auto stack = stacks[iStack];
        mStats.meanGain[iCharge][iStack] = timeGainCalib->getMeanParam(stack, charge, 0);
        mStats.diffCorrectionTgl[iCharge][iStack] = tgl * (timeGainCalib->getMeanParam(stack, charge, 1) + tgl * (timeGainCalib->getMeanParam(stack, charge, 2) + tgl * (timeGainCalib->getMeanParam(stack, charge, 3) + tgl * timeGainCalib->getMeanParam(stack, charge, 4))));
      }

      // Gain averaged over all sectors for a stack type
      mStats.meanGain[iCharge][o2::tpc::GEMSTACKSPERSECTOR] = timeGainCalib->getMeanParam(charge, 0);
      mStats.diffCorrectionTgl[iCharge][o2::tpc::GEMSTACKSPERSECTOR] = tgl * (timeGainCalib->getMeanParam(charge, 1) + tgl * (timeGainCalib->getMeanParam(charge, 2) + tgl * (timeGainCalib->getMeanParam(charge, 3) + tgl * timeGainCalib->getMeanParam(charge, 4))));
    }
    return true;
  }
  return false;
}

} // namespace o2::quality_control_modules::tpc