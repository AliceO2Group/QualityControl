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

BadChannelMapReductor::BadChannelMapReductor() : ReductorTObject(), mGeometry(nullptr), mStats()
{
  mGeometry = o2::emcal::Geometry::GetInstanceFromRunNumber(300000);
}

void* BadChannelMapReductor::getBranchAddress()
{
  return &mStats;
}

const char* BadChannelMapReductor::getBranchLeafList()
{
  return "BadChannelsTotal/I:DeadChannelsTotal:NonGoodChannelsTotal:BadChannelsEMCAL:BadChannelsDCAL:DeadChannelsEMCAL:DeadChannelsDCAL:NonGoodChannelsEMCAL:NonGoodChannelsDCAL:BadChannelsSM[20]:DeadChannelsSM[20]:NonGoodChannelsSM[20]:SupermoduleMaxBad:SupermoduleMaxDead:SupermoduleMaxNonGood:FractionBadTotal/D:FractionDeadTotal:FractionNonGoodTotal:FractionBadEMCAL:FractionBadDCAL:FractionDeadEMCAL:FractionDeadDCAL:FractionNonGoodEMCAL:FractionNonGoodDCAL:FractionBadSupermodule[20]:FractionDeadSupermodule[20]:FractionNonGoodSupermodule[20]";
}

void BadChannelMapReductor::update(TObject* obj)
{
  const std::map<o2::emcal::EMCALSMType, int> channelsSMTYPE = { { o2::emcal::EMCAL_STANDARD, 1152 }, { o2::emcal::EMCAL_THIRD, 384 }, { o2::emcal::DCAL_STANDARD, 768 }, { o2::emcal::DCAL_EXT, 384 } };
  constexpr int CHANNELS_TOTAL = 17664, CHANNELS_EMC = 12288, CHANNELS_DCAL = CHANNELS_TOTAL - CHANNELS_EMC;
  memset(&mStats, 0, sizeof(mStats));
  auto badChannelMap = dynamic_cast<TH2*>(obj);
  if (!badChannelMap) {
    ILOG(Error, Support) << "Object " << obj->GetName() << " not a proper bad channel histogram, or does not exist. Not possible to analyse" << ENDM;
    return;
  }
  for (int icolumn = 0; icolumn < badChannelMap->GetXaxis()->GetNbins(); icolumn++) {
    for (int irow = 0; irow < badChannelMap->GetYaxis()->GetNbins(); irow++) {
      if (isPHOSRegion(icolumn, irow)) {
        continue;
      }
      auto status = badChannelMap->GetBinContent(icolumn + 1, irow + 1);
      try {
        auto [sm, mod, modrow, modcol] = mGeometry->GetCellIndexFromGlobalRowCol(irow, icolumn);
        if (status == 1) { // bad channel
          mStats.mNonGoodChannelsTotal++;
          mStats.mNonGoodChannelsSM[sm]++;
          mStats.mBadChannelsTotal++;
          mStats.mBadChannelsSM[sm]++;
          if (sm < 12) {
            mStats.mNonGoodChannelsEMCAL++;
            mStats.mBadChannelsEMCAL++;
          } else {
            mStats.mNonGoodChannelsDCAL++;
            mStats.mBadChannelsDCAL++;
          }
        } else if (status == 2) {
          mStats.mNonGoodChannelsTotal++;
          mStats.mNonGoodChannelsSM[sm]++;
          mStats.mDeadChannelsTotal++;
          mStats.mDeadChannelsSM[sm]++;
          if (sm < 12) {
            mStats.mNonGoodChannelsEMCAL++;
            mStats.mDeadChannelsEMCAL++;
          } else {
            mStats.mNonGoodChannelsDCAL++;
            mStats.mDeadChannelsDCAL++;
          }
        }
      } catch (o2::emcal::RowColException& e) {
        ILOG(Error, Support) << e.what() << ENDM;
      }
    }
  }
  mStats.mFractionBadTotal = static_cast<double>(mStats.mBadChannelsTotal) / static_cast<double>(CHANNELS_TOTAL);
  mStats.mFractionDeadTotal = static_cast<double>(mStats.mDeadChannelsTotal) / static_cast<double>(CHANNELS_TOTAL);
  mStats.mFractionNonGoodTotal = static_cast<double>(mStats.mNonGoodChannelsTotal) / static_cast<double>(CHANNELS_TOTAL);
  mStats.mFractionBadEMCAL = static_cast<double>(mStats.mBadChannelsEMCAL) / static_cast<double>(CHANNELS_EMC);
  mStats.mFractionDeadEMCAL = static_cast<double>(mStats.mDeadChannelsEMCAL) / static_cast<double>(CHANNELS_EMC);
  mStats.mFractionBadDCAL = static_cast<double>(mStats.mBadChannelsDCAL) / static_cast<double>(CHANNELS_DCAL);
  mStats.mFractionDeadDCAL = static_cast<double>(mStats.mDeadChannelsDCAL) / static_cast<double>(CHANNELS_DCAL);
  mStats.mFractionNonGoodEMCAL = static_cast<double>(mStats.mNonGoodChannelsEMCAL) / static_cast<double>(CHANNELS_EMC);
  mStats.mFractionNonGoodDCAL = static_cast<double>(mStats.mNonGoodChannelsDCAL) / static_cast<double>(CHANNELS_DCAL);
  int currentbad = -1, currentdead = -1, currentnongood = -1;
  for (int ism = 0; ism < 20; ism++) {
    try {
      auto smtype = mGeometry->GetSMType(ism);
      auto nchannels = channelsSMTYPE.find(smtype);
      if (nchannels == channelsSMTYPE.end()) {
        ILOG(Error, Support) << "Unhandled Supermodule type" << ENDM;
        continue;
      }
      mStats.mFractionDeadSM[ism] = static_cast<double>(mStats.mDeadChannelsSM[ism]) / nchannels->second;
      mStats.mFractionBadSM[ism] = static_cast<double>(mStats.mBadChannelsSM[ism]) / static_cast<double>(nchannels->second);
      mStats.mFractionNonGoodSM[ism] = static_cast<double>(mStats.mNonGoodChannelsSM[ism]) / static_cast<double>(nchannels->second);
      if (mStats.mBadChannelsSM[ism] > currentbad) {
        currentbad = mStats.mBadChannelsSM[ism];
        mStats.mSupermoduleMaxBad = ism;
      }
      if (mStats.mDeadChannelsSM[ism] > currentdead) {
        currentdead = mStats.mDeadChannelsSM[ism];
        mStats.mSupermoduleMaxDead = ism;
      }
      if (mStats.mNonGoodChannelsSM[ism] > currentnongood) {
        currentnongood = mStats.mNonGoodChannelsSM[ism];
        mStats.mSupermoduleMaxNonGood = ism;
      }
    } catch (o2::emcal::SupermoduleIndexException& e) {
      ILOG(Error, Support) << e.what() << ENDM;
    }
  }
}

bool BadChannelMapReductor::isPHOSRegion(int column, int row) const
{
  return (column >= 32 && column < 64) && (row >= 128 && row < 200);
}