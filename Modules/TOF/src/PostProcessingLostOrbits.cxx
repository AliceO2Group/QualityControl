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
/// \file   PostProcessingLostOrbits.cxx
/// \author Francesca Ercolessi francesca.ercolessi@cern.ch
///

#include "TOF/PostProcessingLostOrbits.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/DatabaseInterface.h"

#include "TOF/Utils.h"

#include <TH1F.h>
#include <TH2F.h>
#include <boost/property_tree/ptree.hpp>

using namespace o2::quality_control::postprocessing;

namespace o2::quality_control_modules::tof
{

PostProcessingLostOrbits::~PostProcessingLostOrbits()
{
}

void PostProcessingLostOrbits::configure(const boost::property_tree::ptree& config)
{
  if (const auto& customConfigs = config.get_child_optional("qc.postprocessing." + getID() + ".customization"); customConfigs.has_value()) {
    for (const auto& customConfig : customConfigs.value()) { // Plot configuration
      if (const auto& customNames = customConfig.second.get_child_optional("name"); customNames.has_value()) {
        if (customConfig.second.get<std::string>("name") == "Nbins") {
          mBins = customConfig.second.get<int>("value");
          ILOG(Info, Support) << "Setting Nbins to " << mBins << ENDM;
        } else if (customConfig.second.get<std::string>("name") == "MaxValue") {
          mMaxRange = customConfig.second.get<float>("value");
          ILOG(Info, Support) << "Setting Nbins to " << mMaxRange << ENDM;
        } else if (customConfig.second.get<std::string>("name") == "CCDBPath") {
          mCCDBPath = customConfig.second.get<std::string>("value");
          ILOG(Info, Support) << "Setting CCDBPath to " << mCCDBPath << ENDM;
        } else if (customConfig.second.get<std::string>("name") == "MOName") {
          mMOName = customConfig.second.get<std::string>("value");
          ILOG(Info, Support) << "Setting MOName to " << mMOName << ENDM;
        }
      }
    }
  }
}

void PostProcessingLostOrbits::initialize(Trigger, framework::ServiceRegistryRef services)
{
  float bin_width = mMaxRange / mBins;
  mHistoOrbitsInTFEfficiency = std::make_shared<TH1F>("OrbitsInTFEfficiency", "Fraction of orbits in TF;Fraction of orbits in TF; Counts x n_{crates} ", mBins, bin_width / 2, mMaxRange + bin_width / 2);
  getObjectsManager()->startPublishing(mHistoOrbitsInTFEfficiency.get());

  mDatabase = &services.get<o2::quality_control::repository::DatabaseInterface>();
}

void PostProcessingLostOrbits::update(Trigger t, framework::ServiceRegistryRef)
{
  ILOG(Info, Support) << "Trigger type is: " << t.triggerType << ", the timestamp is " << t.timestamp << ENDM;

  TH1F* tempPerCrateHisto[72];   // 72 crates
  for (int i = 0; i < 72; i++) { // loop over crates
    tempPerCrateHisto[i] = nullptr;
  }

  auto mo = mDatabase->retrieveMO(mCCDBPath, mMOName, t.timestamp, t.activity);
  TH2F* moH = static_cast<TH2F*>(mo ? mo->getObject() : nullptr);
  if (moH) {
    ILOG(Info, Support) << "Found MO " << mMOName << " in path " << mCCDBPath << ENDM;
    for (int i = 0; i < 72; i++) { // loop over crates
      tempPerCrateHisto[i] = (TH1F*)moH->ProjectionY(Form("hPerCrate%i",i), i + 1, i + 1);
      mHistoOrbitsInTFEfficiency->Fill(tempPerCrateHisto[i]->Integral() / (32.f * 3.f)); // 32 orbits in Time Frame, 3 readout windows per orbit
    }
  } else {
    ILOG(Warning) << "Did not find MO " << mMOName << " in path " << mCCDBPath << ENDM;
  }
}

void PostProcessingLostOrbits::finalize(Trigger, framework::ServiceRegistryRef)
{
  // Only if you don't want it to be published after finalisation.
  getObjectsManager()->stopPublishing(mHistoOrbitsInTFEfficiency.get());
}

} // namespace o2::quality_control_modules::tof
