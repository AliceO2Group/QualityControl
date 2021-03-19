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
/// \file   SkeletonCheck.cxx
/// \author My Name
///

#include "Skeleton/SkeletonCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TH1.h>

using namespace std;

namespace o2::quality_control_modules::skeleton
{

void SkeletonCheck::configure(std::string) {}

Quality SkeletonCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  std::cout << "asdf" << std::endl;
  ILOG(Info, Support) << "Info" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  ILOG(Debug, Support) << "Debug" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  ILOG(Error, Support) << "Error" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  ILOG(Info, Ops) << "Ops" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  ILOG(Info, Support) << "Support" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  ILOG(Info, Devel) << "Devel" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  ILOG(Info, Trace) << "Trace" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.


  for (auto& [moName, mo] : *moMap) {

    (void)moName;
    if (mo->getName() == "example") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());

      result = Quality::Good;

      for (int i = 0; i < h->GetNbinsX(); i++) {
        if (i > 0 && i < 8 && h->GetBinContent(i) == 0) {
          result = Quality::Bad;
          break;
        } else if ((i == 0 || i > 7) && h->GetBinContent(i) > 0) {
          result = Quality::Medium;
        }
      }
    }
  }
  return result;
}

std::string SkeletonCheck::getAcceptedType() { return "TH1"; }

void SkeletonCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "example") {
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
}

} // namespace o2::quality_control_modules::skeleton
