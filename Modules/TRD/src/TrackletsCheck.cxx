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
/// \author My Name
///

#include "TRD/TrackletsCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TH1.h>
#include <TH2.h>

#include <DataFormatsQualityControl/FlagReasons.h>

#include "CCDB/BasicCCDBManager.h"
#include "TRDQC/StatusHelper.h"

using namespace o2::quality_control;

namespace o2::quality_control_modules::trd
{

void TrackletsCheck::retrieveCCDBSettings()
{
  if (auto param = mCustomParameters.find("ccdbtimestamp"); param != mCustomParameters.end()) {
    mTimestamp = std::stol(mCustomParameters["ccdbtimestamp"]);
    ILOG(Info, Support) << "configure() : using ccdbtimestamp = " << mTimestamp << ENDM;
  } else {
    mTimestamp = o2::ccdb::getCurrentTimestamp();
    ILOG(Info, Support) << "configure() : using default timestam of now = " << mTimestamp << ENDM;
  }
  auto& mgr = o2::ccdb::BasicCCDBManager::instance();
  mgr.setTimestamp(mTimestamp);
  if (auto param = mCustomParameters.find("integralthreshold"); param != mCustomParameters.end()) {
    mIntegralThreshold = std::stol(mCustomParameters["integralthreshold"]);
    ILOG(Info, Support) << "configure() : using integral threshold = " << mIntegralThreshold << ENDM;
  } else {
    mIntegralThreshold = 100;
    ILOG(Info, Support) << "configure() : using default integral threshold of = " << mIntegralThreshold << ENDM;
  }
  if (auto param = mCustomParameters.find("ratiothreshold"); param != mCustomParameters.end()) {
    mRatioThreshold = std::stol(mCustomParameters["ratiothreshold"]);
    ILOG(Info, Support) << "configure() : using ratio threshold = " << mRatioThreshold << ENDM;
  } else {
    mRatioThreshold = 0.9; // 90% of counts must be above the threshold
    ILOG(Info, Support) << "configure() : using default ratio threshold of = " << mRatioThreshold << ENDM;
  }
  if (auto param = mCustomParameters.find("zerobinratiotheshold"); param != mCustomParameters.end()) {
    mZeroBinRatioThreshold = std::stol(mCustomParameters["zerobinratiothreshold"]);
    ILOG(Info, Support) << "configure() : using ratio threshold = " << mZeroBinRatioThreshold << ENDM;
  } else {
    mZeroBinRatioThreshold = 0.9; // 90% of counts must be above the threshold
    ILOG(Info, Support) << "configure() : using default ratio threshold of = " << mZeroBinRatioThreshold << ENDM;
  }
}

void TrackletsCheck::configure()
{
  //get ccdb values
  //fill mask spectra
  retrieveCCDBSettings();
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
      if (h->Integral(0, mIntegralThreshold) > 0 && h->Integral(mIntegralThreshold, 1000) > 0) {
        ratioabove = h->Integral(0, 100) / h->Integral(100, 1000);
      }
      if (ratioabove > mRatioThreshold) {
        result = Quality::Good;
      } else {
        result = Quality::Bad;
        result.addReason(FlagReasonFactory::Unknown(),
                         "The Ratio of tracklet counts above " + std::to_string(mIntegralThreshold) + " is too low " + std::to_string(mRatioThreshold) + " %");
      }

      for (int i = 0; i < h->GetNbinsX(); i++) {
        if (i > 0 && i < 8 && h->GetBinContent(i) == 0) {
          result = Quality::Bad;
          result.addReason(FlagReasonFactory::Unknown(),
                           "It is bad because there is nothing in bin " + std::to_string(i));
          break;
        } else if ((i == 0 || i > 7) && h->GetBinContent(i) > 0) {
          result = Quality::Medium;
          result.addReason(FlagReasonFactory::Unknown(),
                           "It is medium because bin " + std::to_string(i) + " is not empty");
          result.addReason(FlagReasonFactory::BadTracking(),
                           "This is to demonstrate that we can assign more than one Reason to a Quality");
        }
      }
    }
  }
  return result;
}

std::string TrackletsCheck::getAcceptedType() { return "TH1"; }

std::vector<TH2F*> TrackletsCheck::createTrdMaskHistsPerLayer()
{
  std::vector<TH2F*> hMask;
  for (int iLayer = 0; iLayer < 6; ++iLayer) {
    hMask.push_back(new TH2F(Form("layer%i_mask", iLayer), "", 76, -0.5, 75.5, 144, -0.5, 143.5));
    hMask.back()->SetMarkerColor(kRed);
    hMask.back()->SetMarkerSize(0.9);
  }
  return hMask;
}

void TrackletsCheck::fillTrdMaskHistsPerLayer()
{
  for (int iSec = 0; iSec < 18; ++iSec) {
    for (int iStack = 0; iStack < 5; ++iStack) {
      int rowMax = (iStack == 2) ? 12 : 16;
      for (int iLayer = 0; iLayer < 6; ++iLayer) {
        for (int iCol = 0; iCol < 8; ++iCol) {
          int side = (iCol < 4) ? 0 : 1;
          int det = iSec * 30 + iStack * 6 + iLayer;
          int hcid = (side == 0) ? det * 2 : det * 2 + 1;
          for (int iRow = 0; iRow < rowMax; ++iRow) {
            int rowGlb = iStack < 3 ? iRow + iStack * 16 : iRow + 44 + (iStack - 3) * 16; // pad row within whole sector
            int colGlb = iCol + iSec * 8;
            // bin number 0 is underflow
            rowGlb += 1;
            colGlb += 1;
            if (mChamberStatus->isMasked(hcid)) {
              mLayersMask[iLayer]->SetBinContent(rowGlb, colGlb, 1);
            }
          }
        }
      }
    }
  }
}

void TrackletsCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "trackletspertimeframe") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());

    if (checkResult == Quality::Good) {
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      ILOG(Info, Support) << "Quality::Bad, setting to red" << ENDM;
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      ILOG(Info, Support) << "Quality::medium, setting to orange" << ENDM;
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
  if (mo->getName() == "trackletspertimeframecycled") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());

    if (checkResult == Quality::Good) {
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      ILOG(Info, Support) << "Quality::Bad, call on call" << ENDM;
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      ILOG(Info, Support) << "Quality::medium, something might be off" << ENDM;
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
}

} // namespace o2::quality_control_modules::trd
