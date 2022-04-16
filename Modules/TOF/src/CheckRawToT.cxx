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
/// \file   CheckRawToT.cxx
/// \author Nicolò Jacazio <nicolo.jacazio@cern.ch>
/// \brief  Checker for the ToT obtained in the TaskDigits
///

// QC
#include "TOF/CheckRawToT.h"
#include "QualityControl/QcInfoLogger.h"

using namespace std;

namespace o2::quality_control_modules::tof
{

void CheckRawToT::configure()
{

  if (auto param = mCustomParameters.find("MinRawTime"); param != mCustomParameters.end()) {
    mMinRawToT = ::atof(param->second.c_str());
  }
  if (auto param = mCustomParameters.find("MaxRawTime"); param != mCustomParameters.end()) {
    mMaxRawToT = ::atof(param->second.c_str());
  }
  mShifterMessages.configure(mCustomParameters);
}

Quality CheckRawToT::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{

  Quality result = Quality::Null;

  /// Mean of the TOF ToT histogram
  float ToTMean = 0.f;
  /// Number of events with ToT==0 (orphans)
  float ToTOrphanIntegral = 0.f;
  /// Number of events with ToT > 0 (excluding orphans)
  float ToTIntegral = 0.f;

  for (auto& [moName, mo] : *moMap) {
    if (!isObjectCheckable(mo)) {
      ILOG(Error, Support) << "Cannot check MO " << mo->getName() << " " << moName << " which is not of type " << getAcceptedType() << ENDM;
      continue;
    }
    ILOG(Debug, Devel) << "Checking " << mo->getName() << ENDM;

    if (mo->getName().find("ToT/") != std::string::npos) {
      auto* h = static_cast<TH1F*>(mo->getObject());
      if (h->GetEntries() == 0) {
        result = Quality::Medium;
      } else {
        // Set range to compute the average without the orphans
        h->GetXaxis()->SetRange(2, h->GetNbinsX());
        ToTMean = h->GetMean();
        // Reset the range
        h->GetXaxis()->SetRange();
        // Computing fractions
        ToTOrphanIntegral = h->Integral(1, 1);
        ToTIntegral = h->Integral(1, h->GetNbinsX());
        mo->addMetadata("mean", Form("%5.2f", ToTMean));
        if (ToTIntegral > 0) {
          mo->addMetadata("orphanfraction", Form("%5.2f%%", ToTOrphanIntegral * 100. / ToTIntegral));
        } else {
          mo->addMetadata("orphanfraction", "UNDEF");
        }

        if ((ToTMean > mMinRawToT) && (ToTMean < mMaxRawToT)) {
          result = Quality::Good;
        } else {
          ILOG(Warning, Support) << Form("ToT mean = %5.2f ns", ToTMean) << ENDM;
          result = Quality::Bad;
        }
      }
    }
  }
  return result;
}

void CheckRawToT::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  ILOG(Debug, Devel) << "Beautifying " << mo->getName() << ENDM;
  if (!isObjectCheckable(mo)) {
    ILOG(Error, Support) << "Cannot beautify MO " << mo->getName() << " which is not of type " << getAcceptedType() << ENDM;
    return;
  }
  if (mo->getName().find("ToT/") != std::string::npos) {
    auto* h = static_cast<TH1F*>(mo->getObject());
    auto msg = mShifterMessages.MakeMessagePad(h, checkResult);
    if (!msg) {
      return;
    }
    const auto& meta = mo->getMetadataMap();
    msg->AddText(Form("Mean value = %s", meta.at("mean").c_str()));
    msg->AddText(Form("Allowed range: %3.1f-%3.1f ns", mMinRawToT, mMaxRawToT));
    msg->AddText(Form("Orphan fraction = %s", meta.at("orphanfraction").c_str()));
    if (checkResult == Quality::Good) {
      msg->AddText("OK!");
    } else if (checkResult == Quality::Bad) {
      msg->AddText("Call TOF on-call.");
    } else if (checkResult == Quality::Medium) {
      ILOG(Info, Support) << "Quality::medium, setting to yellow";
      msg->AddText("IF TOF IN RUN email TOF on-call.");
    }
  } else {
    ILOG(Error, Support) << "Did not get correct histo from " << mo->GetName() << ENDM;
    return;
  }
}

} // namespace o2::quality_control_modules::tof
