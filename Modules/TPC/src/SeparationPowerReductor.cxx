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
// \file   SeparationPowerReductor.cxx
// \author Marcel Lesch
//
#include "TPC/SeparationPowerReductor.h"
#include "QualityControl/QcInfoLogger.h"
#include <boost/algorithm/string.hpp>
#include <string>
#include <TProfile.h>

namespace o2::quality_control_modules::tpc
{

void* SeparationPowerReductor::getBranchAddress()
{
  return &mSeparationPower;
}

const char* SeparationPowerReductor::getBranchLeafList()
{
  return "amplitudePi/F:meanPi:sigmaPi:amplitudeEl:meanEl:sigmaEl:separationPower:chiSquareOverNdf";
}

void SeparationPowerReductor::update(TObject* obj)
{
  // The values for the separation power are saved in a TProfile
  if (obj) {
    if (auto profile = static_cast<TProfile*>(obj)) {
      mSeparationPower.amplitudePi = profile->GetBinContent(1);
      mSeparationPower.meanPi = profile->GetBinContent(2);
      mSeparationPower.sigmaPi = profile->GetBinContent(3);

      mSeparationPower.amplitudeEl = profile->GetBinContent(4);
      mSeparationPower.meanEl = profile->GetBinContent(5);
      mSeparationPower.sigmaEl = profile->GetBinContent(6);

      mSeparationPower.separationPower = profile->GetBinContent(7);
      mSeparationPower.chiSquareOverNdf = profile->GetBinContent(8);
    }
  } else {
    ILOG(Error, Support) << "No 'obj' found." << ENDM;
  }
}

} // namespace o2::quality_control_modules::tpc
