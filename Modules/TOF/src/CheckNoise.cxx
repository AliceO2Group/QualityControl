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
/// \file   CheckNoise.cxx
/// \author Nicolò Jacazio nicolo.jacazio@cern.ch
/// \brief  Checker for the hit map hit obtained with the TaskDigits
///

// QC
#include "TOF/CheckNoise.h"
#include "QualityControl/QcInfoLogger.h"

using namespace std;

namespace o2::quality_control_modules::tof
{

void CheckNoise::configure()
{
  mShifterMessages.configure(mCustomParameters);
}

Quality CheckNoise::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{

  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {
    if (!isObjectCheckable(mo)) {
      ILOG(Error, Support) << "Cannot check MO " << mo->getName() << " " << moName << " which is not of type " << getAcceptedType() << ENDM;
      continue;
    }
    if (mo->getName() != mAcceptedName) {
      ILOG(Error, Support) << "Cannot check MO " << mo->getName() << " " << moName << " which does not have name " << mAcceptedName << ENDM;
      continue;
    }

    ILOG(Debug, Devel) << "Checking " << mo->getName() << ENDM;
    const auto* h = static_cast<TH1F*>(mo->getObject());

    for (int i = 0; i < h->GetNbinsX(); i++) {
      if (h->GetBinContent(i + 1) <= mMaxNoiseRate) {
        continue;
      }
      result = Quality::Medium;
      locateChannel(i);
      std::string channelFormat = Form("%i %i %i %i %i %i", locatedSupermodule, locatedLink, locatedTrm, locatedChain, locatedTdc, locatedChannel);
      mo->addMetadata(Form("noisyChannel%i", i), channelFormat);
      mShifterMessages.AddMessage(channelFormat);
    }
  }
  return result;
}

void CheckNoise::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  ILOG(Debug, Devel) << "Beautifying " << mo->getName() << ENDM;
  if (!isObjectCheckable(mo)) {
    ILOG(Error, Support) << "Cannot beautify MO " << mo->getName() << " which is not of type " << getAcceptedType() << ENDM;
    return;
  }
  if (mo->getName() == mAcceptedName) {
    auto* h = static_cast<TH2F*>(mo->getObject());
    auto msg = mShifterMessages.MakeMessagePad(h, checkResult);
    if (!msg) {
      return;
    }
  } else {
    ILOG(Error, Support) << "Did not get correct histo from " << mo->GetName() << ENDM;
    return;
  }
}

} // namespace o2::quality_control_modules::tof
