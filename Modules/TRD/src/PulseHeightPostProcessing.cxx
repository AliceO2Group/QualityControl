
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
/// \file TRDTrending.cxx
/// \author      based on Piotr Konopka
///

#include "TRD/PulseHeightPostProcessing.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Reductor.h"
#include "QualityControl/ObjectMetadataKeys.h"
#include "QualityControl/RootClassFactory.h"
#include "QualityControl/RepoPathUtils.h"
#include <TDatime.h>
#include <TH1.h>
#include <TProfile2D.h>
#include <TProfile.h>

#include <TCanvas.h>
#include <TPaveText.h>
#include <TGraphErrors.h>
#include <TPoint.h>
#include <map>
#include <string>
#include <memory>
#include <vector>
using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::repository;
using namespace o2::quality_control::postprocessing;

void PulseHeightPostProcessing::configure(const boost::property_tree::ptree& config)
{

  mHost = config.get<std::string>("qc.config.conditionDB.url");

  mPath = config.get<std::string>("qc.postprocessing.PulseHeightPerChamber2D.path");
  mTimestamps = config.get<long>("qc.postprocessing.PulseHeightPerChamber2D.timestamps");
}

void PulseHeightPostProcessing::initialize(Trigger, framework::ServiceRegistryRef services)
{

  mCdbph.init(mHost);

  for (Int_t sm = 0; sm < 18; sm++) {

    cc[sm] = std::make_shared<TCanvas>(Form("cc%.2i", sm), Form("sm-%.2i", sm));

    cc[sm].get()->Divide(5, 6);

    getObjectsManager()->startPublishing(cc[sm].get());
  }
}

// todo: see if OptimizeBaskets() indeed helps after some time
void PulseHeightPostProcessing::update(Trigger t, framework::ServiceRegistryRef)
{
  PlotPulseHeightPerChamber();
}

void PulseHeightPostProcessing::finalize(Trigger, framework::ServiceRegistryRef services)
{
  for (Int_t sm = 0; sm < 18; sm++)
    getObjectsManager()->stopPublishing(cc[sm].get());
}

void PulseHeightPostProcessing::PlotPulseHeightPerChamber()
{
  std::map<std::string, std::string> meta;
  auto hPulseHeight2D = mCdbph.retrieveFromTFileAny<TProfile2D>(mPath, meta, mTimestamps);

  Int_t cn = 0;
  Int_t sm = 0;
  Int_t cm = 0;
  Int_t c = 1;

  for (Int_t i = 0; i < 540; i++) {

    sm = i / 30;

    h2[i] = (TProfile*)hPulseHeight2D->ProfileX(Form("%i_pfx", i), i, i + 1);
    h2[i]->SetName(Form("phpersm_%.2i", i));
    h2[i]->SetTitle(Form("%.2i_%i_%i", sm, cn / 6, cn % 6)); ////SM_Stack_Layer
    h2[i]->SetXTitle("timebin");
    h2[i]->SetYTitle("pulseheight");
    TProfile* h3 = (TProfile*)h2[i]->Clone();

    cc[sm].get()->cd(cm * 5 + c);

    h3->Draw("p");
    cc[sm].get()->Update();
    cn++;
    //  cout << sm << endl;
    if (cn > 29) {
      cn = 0;
      c = 0;
    }
    cm++;
    if (cm > 5) {
      cm = 0;
      c++;
    }
  }
}
