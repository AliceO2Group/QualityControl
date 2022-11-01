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
#include <map>
#include <TH2.h>
#include <QualityControl/QcInfoLogger.h>
#include <EMCALBase/Geometry.h>
#include <EMCAL/BadChannelMapReductor.h>

using namespace o2::quality_control_modules::emcal;

BadChannelMapReductor::BadChannelMapReductor() : Reductor(), mGeometry(nullptr), mStats()
{
  mGeometry = o2::emcal::Geometry::GetInstanceFromRunNumber(300000);
}

void* BadChannelMapReductor::getBranchAddress()
{
  return &mStats;
}

const char* BadChannelMapReductor::getBranchLeafList()
{
  return "BadChannelsTotal/I:DeadChannelsTotal:BadChannelsEMCAL:BadChannelsDCAL:DeadChannelsEMCAL:DeadChannelsDCAL:BadChannelsSM[20]:DeadChannelsSM[20]:SupermoduleMaxBad:SupermoduleMaxDead:FractionBadTotal/D:FractionDeadTotal:FractionBadEMCAL:FractionBadDCAL:FractionDeadEMCAL:FractionDeadDCAL:FractionBadSupermodule[20]:FractionDeadSupermodule[20]";
}

void BadChannelMapReductor::update(TObject* obj)
{
  const std::map<o2::emcal::EMCALSMType, int> channelsSMTYPE = { { o2::emcal::EMCAL_STANDARD, 1152 }, { o2::emcal::EMCAL_THIRD, 384 }, { o2::emcal::DCAL_STANDARD, 768 }, { o2::emcal::DCAL_EXT, 384 } };
  constexpr int CHANNELS_TOTAL = 17664, CHANNELS_EMC = 12288, CHANNELS_DCAL = CHANNELS_TOTAL - CHANNELS_EMC;
  memset(&mStats, 0, sizeof(mStats));
  auto badChannelMap = static_cast<TH2*>(obj);
  for (int icolumn = 0; icolumn < badChannelMap->GetXaxis()->GetNbins(); icolumn++) {
    for (int irow = 0; irow < badChannelMap->GetYaxis()->GetNbins(); irow++) {
      if (isPHOSRegion(icolumn, irow)) {
        continue;
      }
      auto status = badChannelMap->GetBinContent(icolumn + 1, irow + 1);
      auto [sm, mod, modrow, modcol] = mGeometry->GetCellIndexFromGlobalRowCol(irow, icolumn);
      if (status == 1) { // bad channel
        mStats.mBadChannelsTotal++;
        mStats.mBadChannelsSM[sm]++;
        if (sm < 12) {
          mStats.mBadChannelsEMCAL++;
        } else {
          mStats.mBadChannelsDCAL++;
        }
      } else if (status == 2) {
        mStats.mDeadChannelsTotal++;
        mStats.mDeadChannelsSM[sm]++;
        if (sm < 12) {
          mStats.mDeadChannelsEMCAL++;
        } else {
          mStats.mDeadChannelsDCAL++;
        }
      }
    }
  }
  mStats.mFractionBadTotal = static_cast<double>(mStats.mBadChannelsTotal) / static_cast<double>(CHANNELS_TOTAL);
  mStats.mFractionDeadTotal = static_cast<double>(mStats.mDeadChannelsTotal) / static_cast<double>(CHANNELS_TOTAL);
  mStats.mFractionBadEMCAL = static_cast<double>(mStats.mBadChannelsEMCAL) / static_cast<double>(CHANNELS_EMC);
  mStats.mFractionDeadEMCAL = static_cast<double>(mStats.mDeadChannelsEMCAL) / static_cast<double>(CHANNELS_EMC);
  mStats.mFractionBadDCAL = static_cast<double>(mStats.mBadChannelsDCAL) / static_cast<double>(CHANNELS_DCAL);
  mStats.mFractionDeadDCAL = mStats.mDeadChannelsDCAL / CHANNELS_DCAL;
  int currentbad = -1, currentdead = -1;
  for (int ism = 0; ism < 20; ism++) {
    auto smtype = mGeometry->GetSMType(ism);
    auto nchannels = channelsSMTYPE.find(smtype);
    if (nchannels == channelsSMTYPE.end()) {
      ILOG(Error, Support) << "Unhandled Supermodule type" << ENDM;
      continue;
    }
    mStats.mFractionDeadSM[ism] = static_cast<double>(mStats.mDeadChannelsSM[ism]) / nchannels->second;
    mStats.mFractionBadSM[ism] = static_cast<double>(mStats.mBadChannelsSM[ism]) / static_cast<double>(nchannels->second);
    if (mStats.mBadChannelsSM[ism] > currentbad) {
      currentbad = mStats.mBadChannelsSM[ism];
      mStats.mSupermoduleMaxBad = ism;
    }
    if (mStats.mDeadChannelsSM[ism] > currentdead) {
      currentdead = mStats.mDeadChannelsSM[ism];
      mStats.mSupermoduleMaxDead = ism;
    }
  }
}

bool BadChannelMapReductor::isPHOSRegion(int column, int row) const
{
  return (column >= 32 && column < 64) && (row >= 128 && row < 200);
}