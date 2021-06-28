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
/// \file   ClusterCheck.cxx
/// \author Cristina Terrevoli
///

#include "PHOS/ClusterCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include <fairlogger/Logger.h>
// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TPaveText.h>
#include <TList.h>
#include <iostream>

using namespace std;

namespace o2::quality_control_modules::phos
{

void ClusterCheck::configure(std::string)
{

  //TODO: configure reading bad map from CCDB
  mBadMap.reset(new o2::phos::BadChannelsMap());
}

Quality ClusterCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  auto mo = moMap->begin()->second;

  //check occupancy
  if (mo->getName().find("ClusterOccupancyM") != std::string::npos) {
    auto* h = dynamic_cast<TH2*>(mo->getObject());
    if (h->GetEntries() == 0) {
      return Quality::Bad;
    }
    //get module number:
    int m = mo->getName()[mo->getName().find_first_of("1234")] - '0';
    //Compare with current bad map: check for new bad regions
    float mean = 0;
    int entries = 0, nDead = 0, nNoisy = 0;
    for (int ix = 1; ix <= 64; ix++) {
      for (int iz = 1; iz <= 56; iz++) {
        float cont = h->GetBinContent(ix, iz);
        char relid[3] = { char(m), char(ix), char(iz) };
        short absId;
        o2::phos::Geometry::relToAbsNumbering(relid, absId);

        if (cont > 0) {
          mean += cont;
          entries++;
        } else {
          if (mBadMap->isChannelGood(absId)) { //new bad channel
            nDead++;
          }
        }
      }
    }
    if (entries) {
      mean /= entries;
    }
    //scan noisy channels
    for (int ix = 1; ix <= 64; ix++) {
      for (int iz = 1; iz <= 56; iz++) {
        float cont = h->GetBinContent(ix, iz);
        if (cont > kMaxUccupancyCut * mean) {
          nNoisy++;
        }
      }
    }
    if (nDead > kDeadThreshold) {
      return Quality::Bad;
    }
    if (nNoisy > kNoisyThreshold) {
      return Quality::Medium;
    }
    return Quality::Good;
  }
  return Quality::Good;
}

std::string ClusterCheck::getAcceptedType() { return "TH1"; }

void ClusterCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName().find("ClusterOccupancyM") != std::string::npos) {
    auto* h = dynamic_cast<TH2*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.5, 0.5, 0.9, 0.75, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));

    if (checkResult == Quality::Good) {
      //
      msg->Clear();
      msg->AddText("Occupancy OK!!!");
      msg->SetFillColor(kGreen);
      //
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      LOG(INFO) << "Quality::Bad, setting to red";
      msg->Clear();
      msg->AddText("Too many dead channels");
      msg->AddText("If NOT a technical run,");
      msg->AddText("call PHOS on-call.");
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      LOG(INFO) << "Quality::medium, setting to orange";
      msg->Clear();
      msg->AddText("Too many noisy channels");
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
}
} // namespace o2::quality_control_modules::phos
