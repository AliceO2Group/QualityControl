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

#include <memory>
#include "EMCAL/SupermoduleProjectionReductor.h"
#include "TH2.h"

using namespace o2::quality_control_modules::emcal;

void* SupermoduleProjectionReductorBase::getBranchAddress()
{
  return &mStats;
}

const char* SupermoduleProjectionReductorBase::getBranchLeafList()
{
  return "smCounts[20]/D:smMean[20]:smSigma[20]:smMax[20]";
}

void SupermoduleProjectionReductorBase::update(TObject* obj)
{
  memset(&mStats, 0, sizeof(mStats));
  auto inputhist = dynamic_cast<TH2*>(obj);
  for (auto ism = 0; ism < 20; ism++) {
    std::unique_ptr<TH1> projectionSM(mSupermoduleAxisX ? inputhist->ProjectionY("smprojection", ism + 1, ism + 1) : inputhist->ProjectionX("smprojection", ism + 1, ism + 1));
    mStats.mCountSM[ism] = projectionSM->GetEntries();
    mStats.mMeanSM[ism] = projectionSM->GetMean();
    mStats.mSigmaSM[ism] = projectionSM->GetRMS();
    mStats.mMaxSM[ism] = projectionSM->GetXaxis()->GetBinCenter(projectionSM->GetMaximumBin());
  }
}