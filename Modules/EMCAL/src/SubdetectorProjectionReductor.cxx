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
#include "EMCAL/SubdetectorProjectionReductor.h"
#include "TH2.h"

using namespace o2::quality_control_modules::emcal;

void* SubdetectorProjectionReductor::getBranchAddress()
{
  return &mStats;
}

const char* SubdetectorProjectionReductor::getBranchLeafList()
{
  return "CountsTotal/D:CountsEMCAL:CountsDCAL:MeanTotal:MeanEMCAL:MeanDCAL:SigmaTotal:SigmaEMCAL:SigmaDCAL";
}

void SubdetectorProjectionReductor::update(TObject* obj)
{
  memset(&mStats, 0, sizeof(mStats));
  auto inputhist = dynamic_cast<TH2*>(obj);
  for (auto ibin = 0; ibin < 3; ibin++) {
    std::unique_ptr<TH1> projectionSM(inputhist->ProjectionY("detprojection", ibin + 1, ibin + 1));
    auto counts = projectionSM->GetEntries(),
         mean = projectionSM->GetMean(),
         sigma = projectionSM->GetRMS();
    switch (ibin) {
      case 0:
        mStats.mCountsTotal = counts;
        mStats.mMeanTotal = mean;
        mStats.mSigmaTotal = sigma;
        break;
      case 1:
        mStats.mCountsEMCAL = counts;
        mStats.mMeanEMCAL = mean;
        mStats.mSigmaEMCAL = sigma;
        break;
      case 2:
        mStats.mCountsDCAL = counts;
        mStats.mMeanDCAL = mean;
        mStats.mSigmaDCAL = sigma;
    };
  }
}