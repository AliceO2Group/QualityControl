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
#include <vector>
#include <TH2.h>
#include <TMath.h>
#include <QualityControl/QcInfoLogger.h>
#include <EMCALBase/Geometry.h>
#include <EMCAL/TimeCalibParamReductor.h>

using namespace o2::quality_control_modules::emcal;

TimeCalibParamReductor::TimeCalibParamReductor() : Reductor(), mGeometry(nullptr), mStats()
{
  mGeometry = o2::emcal::Geometry::GetInstanceFromRunNumber(300000);
}

void* TimeCalibParamReductor::getBranchAddress()
{
  return &mStats;
}

const char* TimeCalibParamReductor::getBranchLeafList()
{
  return "FailedChannelsTotal/I:FailedChannelsEMCAL:FailedChannelsDCAL:FailedChannelsSM[20]:SupermoduleMaxFailed:FractionFailedTotal/D:FractionFailedEMCAL:FractionFaileDCAL:FractionFailedSupermodule[20]:MeanShift:SigmaShift";
}

void TimeCalibParamReductor::update(TObject* obj)
{
  const std::map<o2::emcal::EMCALSMType, int> channelsSMTYPE = { { o2::emcal::EMCAL_STANDARD, 1152 }, { o2::emcal::EMCAL_THIRD, 384 }, { o2::emcal::DCAL_STANDARD, 768 }, { o2::emcal::DCAL_EXT, 384 } };
  constexpr int CHANNELS_TOTAL = 17664, CHANNELS_EMC = 12288, CHANNELS_DCAL = CHANNELS_TOTAL - CHANNELS_EMC;
  memset(&mStats, 0, sizeof(mStats));
  auto timeCalib = static_cast<TH1*>(obj);
  std::vector<double> tcpGood;
  for (auto itower = 0; itower < timeCalib->GetXaxis()->GetNbins(); itower++) {
    auto param = timeCalib->GetBinContent(itower + 1);
    if (std::abs(param) > 100) {
      // Consider fit failed if the time calibration param is larger than 100
      mStats.mFailedParams++;
      auto [sm, mod, modphi, modeta] = mGeometry->GetCellIndex(itower);
      mStats.mFailedParamSM[sm]++;
      if (sm >= 12) {
        mStats.mFailedParamsDCAL++;
      } else {
        mStats.mFailedParamsEMCAL++;
      }
    } else {
      tcpGood.emplace_back(param);
    }
  }
  mStats.mFractionFailed = static_cast<double>(mStats.mFailedParams) / static_cast<double>(CHANNELS_TOTAL);
  mStats.mFractionFailedEMCAL = static_cast<double>(mStats.mFractionFailedEMCAL) / static_cast<double>(CHANNELS_EMC);
  mStats.mFractionFailedDCAL = static_cast<double>(mStats.mFailedParamsDCAL) / static_cast<double>(CHANNELS_DCAL);
  int currentmax = -1;
  for (int ism = 0; ism < 20; ism++) {
    auto smtype = mGeometry->GetSMType(ism);
    auto nchannels = channelsSMTYPE.find(smtype);
    if (nchannels == channelsSMTYPE.end()) {
      ILOG(Error, Support) << "Unhandled Supermodule type" << ENDM;
      continue;
    }
    mStats.mFractionFailedSM[ism] = static_cast<double>(mStats.mFailedParamSM[ism]) / static_cast<double>(nchannels->second);
    if (mStats.mFailedParamSM[ism] > currentmax) {
      currentmax = mStats.mFailedParamSM[ism];
      mStats.mSupermoduleMaxFailed = ism;
    }
  }
  mStats.mMeanShiftGood = TMath::Mean(tcpGood.begin(), tcpGood.end());
  mStats.mSigmaShiftGood = TMath::Mean(tcpGood.begin(), tcpGood.end());
}

bool TimeCalibParamReductor::isPHOSRegion(int column, int row) const
{
  return (column >= 32 && column < 64) && (row >= 128 && row < 200);
}