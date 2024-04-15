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

#include "MUONCommon/HistPlotter.h"

#include <TH1F.h>
#include <TCanvas.h>

namespace o2::quality_control_modules::muon
{

void HistPlotter::publish(std::shared_ptr<o2::quality_control::core::ObjectsManager> objectsManager, HistInfo& hinfo, o2::quality_control::core::PublicationPolicy policy)
{
  objectsManager->startPublishing(hinfo.object, policy);
  objectsManager->setDefaultDrawOptions(hinfo.object, hinfo.drawOptions);
  objectsManager->setDisplayHint(hinfo.object, hinfo.displayHints);
  mPublishedHistograms.push_back(hinfo);
}

void HistPlotter::publish(std::shared_ptr<o2::quality_control::core::ObjectsManager> objectsManager, o2::quality_control::core::PublicationPolicy policy)
{
  for (auto hinfo : mHistograms) {
    publish(objectsManager, hinfo, policy);
  }
}

void HistPlotter::unpublish(std::shared_ptr<o2::quality_control::core::ObjectsManager> objectsManager)
{
  for (auto hinfo : mPublishedHistograms) {
    objectsManager->stopPublishing(hinfo.object);
  }
}

void HistPlotter::reset()
{
  for (auto hinfo : mHistograms) {
    TH1* histo = dynamic_cast<TH1*>(hinfo.object);
    if (histo) {
      histo->Reset();
    } else {
      TCanvas* c = dynamic_cast<TCanvas*>(hinfo.object);
      if (c) {
        TObject* obj;
        TIter next(c->GetListOfPrimitives());
        while ((obj = next())) {
          if (!obj->InheritsFrom("TH1")) {
            continue;
          }
          histo = dynamic_cast<TH1*>(obj);
          if (histo) {
            histo->Reset();
          }
        }
      }
    }
  }
}

} // namespace o2::quality_control_modules::muon
