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
/// \file   CheckOnHc2d.cxx
/// \author Deependra Sharma
///

#include "TRD/CheckOnHc2d.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TH2.h>

#include "TRD/TRDHelpers.h"
#include <DataFormatsQualityControl/FlagReasons.h>
#include "DataFormatsTRD/HelperMethods.h"

using namespace std;
using namespace o2::quality_control;
using Helper = o2::trd::HelperMethods;

namespace o2::quality_control_modules::trd
{

void CheckOnHc2d::configure()
{
  if (auto param = mCustomParameters.find("ccdbtimestamp"); param != mCustomParameters.end()) {
    mTimestamp = std::stol(mCustomParameters["ccdbtimestamp"]);
    ILOG(Debug, Support) << "configure() : using ccdbtimestamp = " << mTimestamp << ENDM;
  } else {
    mTimestamp = o2::ccdb::getCurrentTimestamp();
    ILOG(Debug, Support) << "configure() : using default timestam of now = " << mTimestamp << ENDM;
  }

  if (auto param = mCustomParameters.find("ccdbPath"); param != mCustomParameters.end()) {
    mCcdbPath = mCustomParameters["ccdbPath"];
    ILOG(Debug, Support) << "configure() : using ccdb path = " << mCcdbPath << ENDM;
  } else {

    ILOG(Warning, Trace) << "provided ccdb path in config not found" << ENDM;
    mCcdbPath = "TRD/Calib/DCSDPsFedChamberStatus";
    ILOG(Warning, Trace) << "using ccdb path =" << mCcdbPath << ENDM;
  }

  // auto& mgr = o2::ccdb::BasicCCDBManager::instance();
  // mgr.setTimestamp(mTimestamp);
  // mChamberStatus = mgr.get<std::array<int, MAXCHAMBER>>("TRD/Calib/DCSDPsFedChamberStatus");

  mChamberStatus = retrieveConditionAny<std::array<int, MAXCHAMBER>>(mCcdbPath, {}, mTimestamp);

  if (mChamberStatus == nullptr) {
    ILOG(Info, Support) << "mChamberStatus is null, no chamber status to display" << ENDM;
  }
}

Quality CheckOnHc2d::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result1 = Quality::Null;
  Quality result2 = Quality::Null;
  Quality overAllQuality = Quality::Null;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;
    if (mo->getName() == "trackletsperHC2D") {
      auto* h = dynamic_cast<TH2F*>(mo->getObject());
      if (!h) {
        ILOG(Debug, Trace) << "Requested Object Not Found" << ENDM;
        return overAllQuality;
        // return result1;
      }

      result1 = Quality::Good;
      // int nMaskedHC = 0;
      // int nMaskedChInASec;
      for (int i = 0; i < h->GetNbinsX(); i++) { // loop along sector_side
        // nMaskedChInASec = 0;
        for (int j = 0; j < h->GetNbinsY(); j++) { // loop along stack_layer
          double content = h->GetBinContent(i, j);

          if (content == 0) { // if half-chamber is empty
            int sector = i / 2;
            int side = i % 2;
            int stack = j / NLAYER;
            int layer = j % NLAYER;
            int det = Helper::getDetector(sector, stack, layer);
            int hcid = det * 2 + side;

            if (!TRDHelpers::isHalfChamberMasked(hcid, mChamberStatus)) { // if half-chamber is not masked
              result1 = Quality::Bad;
              result1.addReason(FlagReasonFactory::Unknown(), "some half chambers are empty while they are not masked!");
              return result1;
            } // endif chamber is not masked

            // else{
            //   nMaskedHC++;
            //   nMaskedChInASec++;
            //   if(nMaskedHC >  int(NCHAMBER*2*0.4) ){ // if nMaskedHC is greator than 40% of all chambers
            //     result1 = Quality::Bad;
            //     result1.addReason(FlagReasonFactory::Unknown(), "more than 40 percent chambers are masked!");
            //     return result1;
            //   }
            //   if(nMaskedChInASec == NCHAMBERPERSEC){ //if entire sector is masked
            //     result1 = Quality::Bad;
            //     result1.addReason(FlagReasonFactory::Unknown(), "some sectors are entirely masked");
            //     return result1;
            //   }
            // }

          } // endif empty half-chamber
        }   // stack_layer loop ends
      }     // sector_side loop ends

      result2 = Quality::Good;
      int nMaskedHC = 0;        // total number of masked half-chamber
      int nMaskedHChInASec = 1; // total number of masked half-chamber in a sector
      int currentSector = -1, oldSector = -1;
      for (int hcid = 0; hcid < MAXCHAMBER * 2; ++hcid) {
        currentSector = Helper::getSector(hcid / 2);
        if (TRDHelpers::isHalfChamberMasked(hcid, mChamberStatus)) {
          ILOG(Debug, Trace) << "Masked half Chamber id =" << hcid << ENDM;
          nMaskedHC++;
          if (nMaskedHC > (NCHAMBER * 2 * 0.4)) { // if nMaskedHC is greator than 40% of all chambers
            result2 = Quality::Bad;
            result2.addReason(FlagReasonFactory::Unknown(), "more than 40 percent chambers are masked!");
            return result2;
          }
          if (oldSector == currentSector) { // if chambers are masked within same sector
            nMaskedHChInASec++;
            if (nMaskedHChInASec == NCHAMBERPERSEC * 2) { // if entire sector is masked
              result2 = Quality::Bad;
              result2.addReason(FlagReasonFactory::Unknown(), "some sectors are entirely masked");
              return result2;
            }
          } else {
            nMaskedHChInASec = 1;
          }
        } // endif masked chamber
        oldSector = currentSector;
      } // hcid loop ends

      overAllQuality = Quality::Good;
      if (result1 == Quality::Good && result2 == Quality::Good) {
        overAllQuality = Quality::Good;
        overAllQuality.addReason(FlagReasonFactory::Unknown(), "All unmasked chambers working well!");
        return overAllQuality;
      }
    }
  }
  return overAllQuality;
  // return result1;
}

std::string CheckOnHc2d::getAcceptedType() { return "TH2"; }

void CheckOnHc2d::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "trackletsperHC2D") {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());

    if (checkResult == Quality::Good) {
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      ILOG(Debug, Devel) << "Quality::Bad, setting to red" << ENDM;
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      ILOG(Debug, Devel) << "Quality::medium, setting to orange" << ENDM;
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
}

void CheckOnHc2d::reset()
{
  ILOG(Debug, Devel) << "CheckOnHc2d::reset" << ENDM;
}

void CheckOnHc2d::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "CheckOnHc2d::start : " << activity.mId << ENDM;
  mActivity = make_shared<Activity>(activity);
}

void CheckOnHc2d::endOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "CheckOnHc2d::end : " << activity.mId << ENDM;
}

} // namespace o2::quality_control_modules::trd
