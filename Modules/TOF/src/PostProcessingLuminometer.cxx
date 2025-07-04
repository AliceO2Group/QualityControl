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
/// \file   PostProcessingLuminometer.cxx
/// \author Francesca Ercolessi francesca.ercolessi@cern.ch
///

#include "TOF/PostProcessingLuminometer.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/DatabaseInterface.h"

#include "TOF/Utils.h"

#include <TH1F.h>
#include <TH2F.h>
#include <boost/property_tree/ptree.hpp>

using namespace o2::quality_control::postprocessing;

namespace o2::quality_control_modules::tof
{

PostProcessingLuminometer::~PostProcessingLuminometer()
{
}

void PostProcessingLuminometer::configure(const boost::property_tree::ptree& config)
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
        } else if (customConfig.second.get<std::string>("name") == "ActiveThr") {
          mActiveThr = customConfig.second.get<float>("value");
          ILOG(Info, Support) << "Setting mActiveThr to " << mActiveThr << ENDM;
        }
      }
    }
  }
}

void PostProcessingLuminometer::initialize(Trigger, framework::ServiceRegistryRef services)
{
  float bin_width = mMaxRange / mBins;
  mHistoOrbitsInTFEfficiency.reset();
  mHistoOrbitsInTFEfficiency = std::make_shared<TH1F>("OrbitsInTFEfficiency", "Fraction of orbits in TF;Fraction of orbits in TF; Counts x n_{crates} ", mBins, bin_width / 2, mMaxRange + bin_width / 2);
  getObjectsManager()->startPublishing(mHistoOrbitsInTFEfficiency.get());

  mHistoLuminometer.reset();
  mHistoLuminometer = std::make_shared<TH1F>("Luminometer", "Luminometer; ; hit_{TOF}/(eff_{RO}f_{active}) ", 1000, 0., 10000);
  getObjectsManager()->startPublishing(mHistoLuminometer.get());

  mDatabase = &services.get<o2::quality_control::repository::DatabaseInterface>();
}

void PostProcessingLuminometer::update(Trigger t, framework::ServiceRegistryRef)
{
  ILOG(Info, Support) << "Trigger type is: " << t.triggerType << ", the timestamp is " << t.timestamp << ENDM;

  if (mHistoOrbitsInTFEfficiency) {
    mHistoOrbitsInTFEfficiency->Reset();
  }
  if (mHistoLuminometer){
    mHistoLuminometer->Reset();
  }

  TH1F* tempPerCrateHisto[72];   // 72 crates
  for (int i = 0; i < 72; i++) { // loop over crates
    tempPerCrateHisto[i] = nullptr;
  }

  auto moEfficiency = mDatabase->retrieveMO(mCCDBPath, mMOEfficiency, t.timestamp, t.activity);
  auto moActiveChannels = mDatabase->retrieveMO(mCCDBPath, mMOActiveChannels, t.timestamp, t.activity);
  auto moMultiplicity = mDatabase->retrieveMO(mCCDBPath, mMOMultiplicity, t.timestamp, t.activity);

  float ROeff = 1.;
  float hitMult = 0.;
  float activeCh = 1.;

  // Readout efficiency
  TH2F* moHEfficiency = static_cast<TH2F*>(moEfficiency ? moEfficiency->getObject() : nullptr);
  if (moHEfficiency) {
    ILOG(Info, Support) << "Found MO " << mMOEfficiency << " in path " << mCCDBPath << ENDM;
    for (int i = 0; i < 72; i++) { // loop over crates
      tempPerCrateHisto[i] = (TH1F*)moHEfficiency->ProjectionY(Form("hPerCrate%i", i), i + 1, i + 1);
      mHistoOrbitsInTFEfficiency->Fill(tempPerCrateHisto[i]->Integral() / (32.f * 3.f)); // 32 orbits in Time Frame, 3 readout windows per orbit
    }
  } else {
    ILOG(Warning) << "Did not find MO " << mMOEfficiency << " in path " << mCCDBPath << ENDM;
  }

  if (mHistoOrbitsInTFEfficiency) {
    ROeff = mHistoOrbitsInTFEfficiency->GetMean();
  }

  // Active channels
  TH2F* moHActiveChannels = static_cast<TH2F*>(moActiveChannels ? moActiveChannels->getObject() : nullptr);
  if (moHActiveChannels) {
    ILOG(Info, Support) << "Found MO " << mMOActiveChannels << " in path " << mCCDBPath << ENDM;

    int counterAll = 0;
    int counterActive = 0;

    for (int i = 1; i <= moHActiveChannels->GetNbinsX(); i++) {
      for (int j = 1; j <= moHActiveChannels->GetNbinsY(); j++) {
        counterAll++;

        if (moHActiveChannels->GetBinContent(i,j)>mActiveThr){
          counterActive++;
        }
      }
    }

    activeCh = (float)counterActive/counterAll;
  } else {
    ILOG(Warning) << "Did not find MO " << mMOActiveChannels << " in path " << mCCDBPath << ENDM;
  }

  // Multiplicity
  TH1F* moHMultiplicity = static_cast<TH1F*>(moMultiplicity ? moMultiplicity->getObject() : nullptr);
  if (moHMultiplicity) {
    ILOG(Info, Support) << "Found MO " << mMOMultiplicity << " in path " << mCCDBPath << ENDM;
    hitMult = moHMultiplicity->GetMean();
  } else {
    ILOG(Warning) << "Did not find MO " << mMOMultiplicity << " in path " << mCCDBPath << ENDM;
  }

  mHistoLuminometer->Fill(hitMult/(activeCh*ROeff));
}

void PostProcessingLuminometer::finalize(Trigger, framework::ServiceRegistryRef)
{
  // Only if you don't want it to be published after finalisation.
  getObjectsManager()->stopPublishing(mHistoLuminometer.get());
}

} // namespace o2::quality_control_modules::tof
