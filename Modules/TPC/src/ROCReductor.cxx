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

//
// file  ROCReductor.cxx
// author Cindy Mordasini
// author Marcel Lesch
//

#include "TPC/ROCReductor.h"
#if __has_include("TPCBase/CalDet.h")
#include "TPCBase/CalDet.h"
#else
#include "TPCBaseRecSim/CalDet.h"
#endif
#include "TPCBase/CalArray.h"
#include "TPCQC/CalPadWrapper.h"
#include <TMath.h>
#include <vector>
#include <algorithm>
#include <cmath>

namespace o2::quality_control_modules::tpc
{

void* ROCReductor::getBranchAddress()
{
  return &mCalPad;
} // void* ROCReductor::getBranchAddress()

const char* ROCReductor::getBranchLeafList()
{
  return "entries[72]/I:mean[72]/F:stddev[72]:median[72]:rms[72]";
} // const char* ROCReductor::getBranchLeafList()

void ROCReductor::update(TObject* obj)
{
  if (obj) {
    auto wrappedCalDet = dynamic_cast<o2::tpc::qc::CalPadWrapper*>(obj);
    if (wrappedCalDet) {
      o2::tpc::CalPad* pcalDet = wrappedCalDet->getObj();
      auto& calDet = *pcalDet;

      for (size_t iROC = 0; iROC < calDet.getData().size(); ++iROC) {
        auto& calArray = calDet.getCalArray(iROC);
        auto& data = calArray.getData();

        // Remove pads which are empty from consideration
        data.erase(std::remove_if(data.begin(), data.end(), [](const auto& value) { return (std::isnan(value) || value <= 0); }), data.end());

        mCalPad.entries[iROC] = static_cast<int>(data.size());
        mCalPad.mean[iROC] = TMath::Mean(data.begin(), data.end());
        mCalPad.stddev[iROC] = TMath::StdDev(data.begin(), data.end());
        mCalPad.median[iROC] = TMath::Median(data.size(), data.data());
        mCalPad.rms[iROC] = TMath::RMS(data.size(), data.data());

      } // for (size_t iROC = 0; iROC < calDet.getData().size(); ++iROC)
    }   // if (wrappedCalDet)
  }     // if(obj)
} // void ROCReductor::update(TObject* obj)

} // namespace o2::quality_control_modules::tpc
