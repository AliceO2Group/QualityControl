// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   ChannelTimeCalibrationCheck.cxx
/// \author Milosz Filus
///

#include <TH2F.h>
#include <TText.h>
#include "FT0/ChannelTimeCalibrationCheck.h"
#include "DataFormatsFT0/RawEventData.h"

namespace o2::quality_control_modules::ft0
{

void ChannelTimeCalibrationCheck::configure(std::string)
{
  mMeanWarning = std::stod(mCustomParameters.at(MEAN_WARNING_KEY));
  mMeanError = std::stod(mCustomParameters.at(MEAN_ERROR_KEY));
  mRMSWarning = std::stod(mCustomParameters.at(RMS_WARNING_KEY));
  mRMSError = std::stod(mCustomParameters.at(RMS_ERROR_KEY));
  mMinEntries = std::stoi(mCustomParameters.at(MIN_ENTRIES_KEY));
}

Quality ChannelTimeCalibrationCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality currentQuality = Quality::Bad;
  for (auto [name, obj] : *moMap) {
    (void)name;
    if (obj->getName() == "Calibrated_time_per_channel") {
      currentQuality = Quality::Good;
      auto histogram = dynamic_cast<TH2F*>(obj->getObject());
      for (size_t chID = 0; chID < o2::ft0::Nchannels_FT0; ++chID) {
        auto channelProjection = histogram->ProjectionY(("Times per channel: " + std::to_string(chID)).c_str(), chID, chID);
        if (channelProjection->GetEntries() < mMinEntries) {
          return Quality::Bad;
        }
        if (std::fabs(channelProjection->GetMean()) > mMeanError) {
          return Quality::Bad;
        } else if (std::fabs(channelProjection->GetMean()) > mMeanWarning) {
          currentQuality = Quality::Medium;
        }
        if (channelProjection->GetRMS() > mRMSError) {
          return Quality::Bad;
        } else if (channelProjection->GetRMS() > mRMSWarning) {
          currentQuality = Quality::Medium;
        }
      }
    }
  }
  return currentQuality;
}

void ChannelTimeCalibrationCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality quality)
{
  auto* h = dynamic_cast<TH2F*>(mo->getObject());
  auto* tInfo = new TText();

  if (Quality::Good == quality) {
    tInfo->SetText(0.2, 0.8, "Calibration Quality = GOOD");
    tInfo->SetTextColor(kGreen);
  } else if (Quality::Medium == quality) {
    tInfo->SetText(0.2, 0.8, "Calibration Quality = MEDIUM");
    tInfo->SetTextColor(kYellow);
  } else if (Quality::Bad == quality) {
    tInfo->SetText(0.2, 0.8, "Calibration Quality = BAD");
    tInfo->SetTextColor(kRed);
  }
  tInfo->SetTextSize(23);
  tInfo->SetNDC();
  h->GetListOfFunctions()->Add(tInfo);
}

} // namespace o2::quality_control_modules::ft0