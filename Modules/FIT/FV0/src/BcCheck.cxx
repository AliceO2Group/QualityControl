// Copyright 2023 CERN and copyright holders of ALICE O2.
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
/// \file   BcCheck.cxx
/// \author Dawid Skora dawid.mateusz.skora@cern.ch
///

#include "QualityControl/MonitorObject.h"
#include "FV0/BcCheck.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/Quality.h"
#include <TList.h>
#include <TH2.h>
#include <TPaveText.h>

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::fv0
{

void BcCheck::configure()
{

}

Quality BcCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
    Quality result = Quality::Null;
    for (auto& [moName, mo] : *moMap) {
        (void)moName;
        if (mo->getName() == "BCvsFEEmodules" || mo->getName() == "BCvsTriggers") {
            auto* histogram = dynamic_cast<TH2*>(mo->getObject());

            if (!histogram) {
                ILOG(Error, Support) << "check(): MO " << mo->getName() << " not found" << ENDM;
                result.addReason(FlagReasonFactory::Unknown(), "MO " + mo->getName() + " not found");
                result.set(Quality::Null);
                return result;
            }

            for (int channel = 1; channel < histogram->GetNbinsX(); ++channel) {
                
            }
        }
    }
}

std::string BcCheck::getAcceptedType() { return "TH2"; }

void BcCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
    if (mo->getName() == "BCvsFEEmodules" || mo->getName() == "BCvsTriggers") {

        auto* histogram = dynamic_cast<TH2*>(mo->getObject());

        if (!histogram) {
            ILOG(Error, Support) << "beautify(): MO " << mo->getName() << " not found" << ENDM;
            return;
        }

        TPaveText* msg = new TPaveText(0.15, 0.2, 0.85, 0.45, "NDC");
        histogram->GetListOfFunctions()->Add(msg);
        msg->SetName(Form("%s_msg", mo->GetName()));
        msg->Clear();

        if (checkResult == Quality::Good) {
            msg->AddText(">> Quality::Good <<");
            msg->SetFillColor(kGreen);
        } else if (checkResult == Quality::Bad) {
            auto reasons = checkResult.getReasons();
            msg->SetFillColor(kRed);
            msg->AddText(">> Quality::Bad <<");
        } else if (checkResult == Quality::Medium) {
            auto reasons = checkResult.getReasons();
            msg->SetFillColor(kOrange);
            msg->AddText(">> Quality::Medium <<");
        } else if (checkResult == Quality::Null) {
            msg->AddText(">> Quality::Null <<");
            msg->SetFillColor(kGray);
        }
    }
}
} // namespace o2::quality_control_modules::fv0
