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
/// \file   TrackletsCheck.cxx
/// \author Deependra Sharma
///

#include "TRD/TrackletsCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include "Common/Utils.h"
// ROOT
#include <TH1.h>
#include <TH2.h>

#include <DataFormatsQualityControl/FlagReasons.h>

#include "CCDB/BasicCCDBManager.h"
#include "TRDQC/StatusHelper.h"

using namespace o2::quality_control;
using namespace o2::quality_control_modules::common;

namespace o2::quality_control_modules::trd
{

void TrackletsCheck::configure()
{
  mIntegralThreshold = getFromConfig<float>(mCustomParameters, "integralthreshold", 100.f);
  mRatioThreshold = getFromConfig<float>(mCustomParameters, "ratiothreshold", .9f);               // 90% of counts must be above the threshold
  mZeroBinRatioThreshold = getFromConfig<float>(mCustomParameters, "zerobinratiotheshold", .01f); // 1% of counts can be in this bin
}

Quality TrackletsCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;
    if (mo->getName() == "trackletspertimeframe") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());

      result = Quality::Good;
      float ratioabove;
      if (h->Integral(0, mIntegralThreshold) > 0 && h->Integral(mIntegralThreshold, h->GetXaxis()->GetXmax()) > 0) {
        ratioabove = h->Integral(0, mIntegralThreshold) / h->Integral(mIntegralThreshold, h->GetXaxis()->GetXmax());
      }
      if (ratioabove < mRatioThreshold) {
        result = Quality::Good;
      } else {
        result = Quality::Bad;
        result.addReason(FlagReasonFactory::Unknown(),
                         "The Ratio of tracklet counts below the threshold of " + std::to_string(mIntegralThreshold) + " is too high " + std::to_string(mRatioThreshold) + " %");
      }
      auto integral = h->Integral(1, h->GetXaxis()->GetXmax());
      if (integral <= 0.0) {
        integral = 1;
      }
      if (h->GetBinContent(0) / integral < mZeroBinRatioThreshold) {
        // too many counts in 0 bin.
        result = Quality::Bad;
        result.addReason(FlagReasonFactory::Unknown(),
                         "The Ratio of tracklet counts in bin zero is too high, ratio : " + std::to_string((h->GetBinContent(0) / integral < mZeroBinRatioThreshold)) + " threshold of " + std::to_string(mIntegralThreshold) + " %");
      }
    }
  }
  return result;
}

std::string TrackletsCheck::getAcceptedType() { return "TH1"; }

void TrackletsCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "trackletspertimeframe") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());

    if (checkResult == Quality::Good) {
      h->SetFillColor(kGreen);
      h->SetLineColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      ILOG(Info, Support) << "Quality::Bad, setting to red" << ENDM;
      h->SetFillColor(kRed);
      h->SetLineColor(kRed);
    } else if (checkResult == Quality::Medium) {
      ILOG(Info, Support) << "Quality::medium, setting to orange" << ENDM;
      h->SetFillColor(kOrange);
      h->SetLineColor(kOrange);
    }
  }
}

} // namespace o2::quality_control_modules::trd
