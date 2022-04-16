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
/// \file   CheckRawMultiplicity.cxx
/// \author Nicolò Jacazio <nicolo.jacazio@cern.ch>
/// \brief  Checker for the raw hit multiplicity obtained with the TaskDigits
///

// QC
#include "TOF/CheckRawMultiplicity.h"
#include "QualityControl/QcInfoLogger.h"

using namespace std;

namespace o2::quality_control_modules::tof
{

void CheckRawMultiplicity::configure()
{
  if (auto param = mCustomParameters.find("RunningMode"); param != mCustomParameters.end()) {
    mRunningMode = ::atoi(param->second.c_str());
  }
  switch (mRunningMode) {
    case kModeCollisions:
    case kModeCosmics:
      break;
    default:
      ILOG(Fatal, Support) << "Run mode not correct " << mRunningMode << ENDM;
      break;
  }
  if (auto param = mCustomParameters.find("MinRawHits"); param != mCustomParameters.end()) {
    mMinRawHits = ::atof(param->second.c_str());
  }
  if (auto param = mCustomParameters.find("MaxRawHits"); param != mCustomParameters.end()) {
    mMaxRawHits = ::atof(param->second.c_str());
  }
  if (auto param = mCustomParameters.find("MaxFractAtZeroMult"); param != mCustomParameters.end()) {
    mMaxFractAtZeroMult = ::atof(param->second.c_str());
  }
  if (auto param = mCustomParameters.find("MaxFractAtLowMult"); param != mCustomParameters.end()) {
    mMaxFractAtLowMult = ::atof(param->second.c_str());
  }
  mShifterMessages.configure(mCustomParameters);
}

Quality CheckRawMultiplicity::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{

  Quality result = Quality::Null;

  /// Mean of the TOF hit multiplicity histogram
  float hitsMean = 0.f;
  /// Number of events with 0 TOF hits
  float hitsZeroMultIntegral = 0.f;
  /// Number of events with low TOF hits multiplicity
  float hitsLowMultIntegral = 0.f;
  /// Number of events with TOF hits multiplicity > 0
  float hitsIntegral = 0.f;

  for (auto& [moName, mo] : *moMap) {
    if (!isObjectCheckable(mo)) {
      ILOG(Error, Support) << "Cannot check MO " << mo->getName() << " " << moName << " which is not of type " << getAcceptedType() << ENDM;
      continue;
    }
    ILOG(Debug, Devel) << "Checking " << mo->getName() << ENDM;
    if (mo->getName() == "Multiplicity/Integrated") {
      const auto* h = static_cast<TH1I*>(mo->getObject());
      if (h->GetEntries() == 0) { // Histogram is empty
        result = Quality::Medium;
        mShifterMessages.AddMessage("No counts!");
      } else { // Histogram is non empty

        // Computing variables to check
        hitsMean = h->GetMean();
        hitsZeroMultIntegral = h->Integral(1, 1);
        hitsLowMultIntegral = h->Integral(1, 10);
        hitsIntegral = h->Integral(2, h->GetNbinsX());

        mo->addMetadata("mean", Form("%5.2f", hitsMean));
        mo->addMetadata("integ0mult", Form("%f", hitsZeroMultIntegral));
        mo->addMetadata("integlowmult", Form("%f", hitsLowMultIntegral));
        mo->addMetadata("integ", Form("%f", hitsIntegral));

        if (hitsIntegral > 0) {
          mo->addMetadata("frac0mult", Form("%5.2f%%", hitsZeroMultIntegral * 100. / hitsIntegral));
        } else {
          mo->addMetadata("frac0mult", "UNDEF");
        }

        if (hitsIntegral == 0) { //if only "0 hits per event" bin is filled -> error
          if (h->GetBinContent(1) > 0) {
            result = Quality::Bad;
            mShifterMessages.AddMessage("Only events at 0 filled!");
          }
        } else {
          const bool isZeroBinContentHigh = (hitsZeroMultIntegral > (mMaxFractAtZeroMult * hitsIntegral));
          const bool isLowMultContentHigh = (hitsLowMultIntegral > (mMaxFractAtLowMult * hitsIntegral));
          const bool isAverageLow = (hitsMean < mMinRawHits);
          const bool isAverageHigh = (hitsMean > mMaxRawHits);
          switch (mRunningMode) {
            case kModeCollisions: // Collisions
              if (isZeroBinContentHigh) {
                result = Quality::Medium;
                mShifterMessages.AddMessage("Zero-multiplicity counts are high");
                mShifterMessages.AddMessage(Form("(%.2f higher than total)!", mMaxFractAtZeroMult));
              } else if (isLowMultContentHigh) {
                result = Quality::Medium;
                mShifterMessages.AddMessage("Low-multiplicity counts are high");
                mShifterMessages.AddMessage(Form("(%.2f higher than total)!", mMaxFractAtLowMult));
              } else if (isAverageLow) {
                result = Quality::Medium;
                mShifterMessages.AddMessage(Form("Average lower than expected (%.2f)!", mMinRawHits));
              } else if (isAverageHigh) {
                result = Quality::Medium;
                mShifterMessages.AddMessage(Form("Average higher than expected (%.2f)!", mMaxRawHits));
              } else {
                result = Quality::Good;
                mShifterMessages.AddMessage("Average within limits");
              }
              break;
            case kModeCosmics: // Cosmics
              if (hitsMean < 10.) {
                result = Quality::Good;
                mShifterMessages.AddMessage("Average within limits");
              } else {
                result = Quality::Medium;
                mShifterMessages.AddMessage("Average outside limits!");
              }
              break;
            default:
              ILOG(Fatal, Support) << "Not running in correct mode " << mRunningMode << ENDM;
              break;
          }
        }
      }
    }
  }
  return result;
}

void CheckRawMultiplicity::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  ILOG(Debug, Devel) << "Beautifying " << mo->getName() << ENDM;
  if (!isObjectCheckable(mo)) {
    ILOG(Error, Support) << "Cannot beautify MO " << mo->getName() << " which is not of type " << getAcceptedType() << ENDM;
    return;
  }
  if (mo->getName() == "Multiplicity/Integrated") {
    auto* h = static_cast<TH1I*>(mo->getObject());
    auto msg = mShifterMessages.MakeMessagePad(h, checkResult);
    if (!msg) {
      return;
    }
    const auto& meta = mo->getMetadataMap();
    msg->AddText(Form("Mean value = %s", meta.at("mean").c_str()));
    msg->AddText(Form("Reference range: %5.2f-%5.2f", mMinRawHits, mMaxRawHits));
    msg->AddText(Form("Events with 0 hits = %s", meta.at("frac0mult").c_str()));

    if (checkResult == Quality::Good) {
      msg->AddText("OK!");
    } else if (checkResult == Quality::Bad) {
      msg->AddText("Call TOF on-call.");
    } else if (checkResult == Quality::Medium) {
      ILOG(Info, Support) << "Quality::medium, setting to yellow";
      msg->AddText("IF TOF IN RUN email TOF on-call.");
    }
  } else if (mo->getName() == "Multiplicity/SectorIA" ||
             mo->getName() == "Multiplicity/SectorOA" ||
             mo->getName() == "Multiplicity/SectorIC" ||
             mo->getName() == "Multiplicity/SectorOC") {
    auto* h = static_cast<TH1I*>(mo->getObject());
    auto msg = mShifterMessages.MakeMessagePad(h, checkResult);
    if (!msg) {
      return;
    }
    msg->AddText(Form("Mean value = %5.2f", h->GetMean()));
  } else {
    ILOG(Error, Support) << "Did not get correct histo from " << mo->GetName() << ENDM;
    return;
  }
}

} // namespace o2::quality_control_modules::tof
