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
#include <TCanvas.h>
#include <TPaveText.h>

namespace o2::quality_control_modules::tpc
{

void* SeparationPowerReductor::getBranchAddress()
{
  return &mSeparationPower;
}

const char* SeparationPowerReductor::getBranchLeafList()
{
  return "meanPi/D:meanEl:separationPower";
}

void SeparationPowerReductor::update(TObject* obj)
{
  // The values for the separation power are saved in a TPaveText inside a TCanvas 'obj'.
  if (obj) {
    if (auto canvas = static_cast<TCanvas*>(obj)) {
      if (auto blocText = static_cast<TPaveText*>(canvas->GetPrimitive("TPave"))) {
        mSeparationPower.meanPi = getValue((TText*)blocText->GetLineWith("Mean Pi:"));
        mSeparationPower.meanEl = getValue((TText*)blocText->GetLineWith("Mean El:"));
        mSeparationPower.separationPower = getValue((TText*)blocText->GetLineWith("separationPower:"));
      }
    }
  } else {
    ILOG(Error, Support) << "No 'obj' found." << ENDM;
  }
}

double SeparationPowerReductor::getValue(TText* line)
{
  if (!line) {
    return 0.;
  }

  std::string text = static_cast<std::string>(line->GetTitle());
  const std::size_t posEndType = text.find(":");
  std::string quantity = text.substr(posEndType + 2, -1); // take string (excluding : and empty space) till end of line
  ILOG(Error, Support) << quantity << ENDM;

  return stod(quantity);
}

} // namespace o2::quality_control_modules::tpc
