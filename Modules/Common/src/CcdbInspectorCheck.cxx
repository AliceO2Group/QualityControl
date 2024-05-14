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
/// \file   CcdbInspectorCheck.cxx
/// \author Andrea Ferrero
///

#include "Common/CcdbInspectorCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"

#include <DataFormatsQualityControl/FlagType.h>
#include <DataFormatsQualityControl/FlagTypeFactory.h>

// ROOT
#include <TH2.h>

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::common
{

void CcdbInspectorCheck::configure() {}

Quality CcdbInspectorCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;
    if (mo->getName() == "ObjectsStatus") {
      auto* h = dynamic_cast<TH2F*>(mo->getObject());

      // initialize the overall quality when processing the first QO
      if (result == Quality::Null) {
        result.set(Quality::Good);
      }

      // loop over objects (X-axis bins)
      for (int i = 1; i <= h->GetNbinsX(); i++) {
        std::string objectName = h->GetXaxis()->GetBinLabel(i);

        // check that the first Y bin is non-zero, whch corresponds to VALID object status
        bool isGood = h->GetBinContent(i, 1) > 0;
        if (!isGood) {
          result.set(Quality::Bad);
        }
        // if any of the other Y bins is different from zero, it means that there was n issue accessing the object
        for (int j = 2; j <= h->GetNbinsY(); j++) {
          std::string flag = h->GetYaxis()->GetBinLabel(j);
          if (h->GetBinContent(i, j) > 0) {
            result.set(Quality::Bad);
            result.addFlag(FlagTypeFactory::Unknown(), objectName + " is " + flag);
          }
        }
      }
    }
  }
  return result;
}

std::string CcdbInspectorCheck::getAcceptedType() { return "TH1"; }

void CcdbInspectorCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
}

void CcdbInspectorCheck::reset()
{
}

void CcdbInspectorCheck::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "CcdbInspectorCheck::start : " << activity.mId << ENDM;
  mActivity = make_shared<Activity>(activity);
}

void CcdbInspectorCheck::endOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "CcdbInspectorCheck::end : " << activity.mId << ENDM;
}

} // namespace o2::quality_control_modules::common
