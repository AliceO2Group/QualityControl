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
#include "TOF/Utils.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/MonitorObject.h"

#include <DataFormatsQualityControl/FlagType.h>
#include <DataFormatsQualityControl/FlagTypeFactory.h>

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::tof
{

void CheckRawToT::configure()
{
  utils::parseDoubleParameter(mCustomParameters, "MinEntriesBeforeMessage", mMinEntriesBeforeMessage);
  utils::parseFloatParameter(mCustomParameters, "MinAllowedToT", mMinAllowedToT);
  utils::parseFloatParameter(mCustomParameters, "MaxAllowedToT", mMaxAllowedToT);
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
    if (!mo->encapsulatedInheritFrom(mAcceptedType)) {
      ILOG(Error, Support) << "Cannot check MO " << mo->getName() << " " << moName << " which is not of type " << mAcceptedType << ENDM;
      continue;
    }
    ILOG(Debug, Devel) << "Checking " << mo->getName() << ENDM;

    if (mo->getName().find("ToT/") != std::string::npos) {
      auto* h = static_cast<TH1F*>(mo->getObject());
      if (h->GetEntries() == 0) {
        result = Quality::Medium;
        result.addFlag(FlagTypeFactory::NoDetectorData(),
                       "Empty histogram (no counts)");
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

        if ((ToTMean > mMinAllowedToT) && (ToTMean < mMaxAllowedToT)) {
          result = Quality::Good;
        } else {
          ILOG(Warning, Support) << Form("ToT mean = %5.2f ns", ToTMean) << ENDM;
          result = Quality::Bad;
          result.addFlag(FlagTypeFactory::Unknown(),
                         "ToT mean out of expected range");
        }
      }
    }
  }
  return result;
}

void CheckRawToT::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  ILOG(Debug, Devel) << "Beautifying " << mo->getName() << ENDM;
  if (!mo->encapsulatedInheritFrom(mAcceptedType)) {
    ILOG(Error, Support) << "Cannot beautify MO " << mo->getName() << " which is not of type " << mAcceptedType << ENDM;
    return;
  }
  if (mo->getName().find("ToT/") != std::string::npos) {
    auto* h = static_cast<TH1F*>(mo->getObject());
    auto msg = mShifterMessages.MakeMessagePad(h, checkResult);
    if (!msg) {
      return;
    }
    const auto& meta = mo->getMetadataMap();
    auto getMetaData = [&meta, &mo](const char* key) {
      if (meta.find(key) == meta.end()) {
        ILOG(Warning, Support) << "Looking for key '" << key << "' in metadata of " << mo->getName() << ", not found!" << ENDM;
        return static_cast<const char*>(Form("'Key %s not found'", key));
      }
      return meta.at(key).c_str();
    };
    msg->AddText(Form("Mean value = %s", getMetaData("mean")));
    msg->AddText(Form("Allowed range: %3.1f-%3.1f ns", mMinAllowedToT, mMaxAllowedToT));
    msg->AddText(Form("Orphan fraction = %s", getMetaData("orphanfraction")));

    if (h->GetEntries() < mMinEntriesBeforeMessage) { // Checking that the histogram has enough entries before printing messages
      msg->AddText("Cannot establish quality yet");
      msg->SetTextColor(kWhite);
      return;
    }

    if (checkResult == Quality::Good) {
      msg->AddText("OK!");
    } else if (checkResult == Quality::Bad) {
      msg->AddText("Call TOF on-call.");
    } else if (checkResult == Quality::Medium) {
      ILOG(Info, Support) << "Quality::medium, setting to yellow" << ENDM;
      msg->AddText("IF TOF IN RUN email TOF on-call.");
    }
  } else {
    ILOG(Error, Support) << "Did not get correct histo from " << mo->GetName() << ENDM;
    return;
  }
}

} // namespace o2::quality_control_modules::tof
