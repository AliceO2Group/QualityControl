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
/// \file   PostProcessDiagnosticPerCrate.cxx
/// \brief  Post processing to rearrange TOF information at the level of the crate (maybe we should do the opposite..)
/// \author Nicolo' Jacazio and Francesca Ercolessi
/// \since  11/09/2020
///

// QC includes
#include "TOF/PostProcessDiagnosticPerCrate.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QcInfoLogger.h"

// ROOT includes
#include <TH2F.h>
#include <TCanvas.h>

using namespace o2::quality_control::postprocessing;

namespace o2::quality_control_modules::tof
{

const int PostProcessDiagnosticPerCrate::mNWords = 32;
const int PostProcessDiagnosticPerCrate::mNSlots = 14;

PostProcessDiagnosticPerCrate::~PostProcessDiagnosticPerCrate()
{
}

void PostProcessDiagnosticPerCrate::configure(const boost::property_tree::ptree& config)
{
  const std::string baseJsonPath = "qc.postprocessing." + getID() + ".customization.";
  mCCDBPath = config.get<std::string>(baseJsonPath + "CCDBPath", "TOF/MO/TaskRaw/");
  ILOG(Info, Support) << "Setting CCDBPath to " << mCCDBPath << ENDM;
  mCCDBPathObjectDRM = config.get<std::string>(baseJsonPath + "CCDBPathObjectDRM", "DRMCounter");
  ILOG(Info, Support) << "Setting mCCDBPathObjectDRM to " << mCCDBPathObjectDRM << ENDM;
  mCCDBPathObjectLTM = config.get<std::string>(baseJsonPath + "CCDBPathObjectLTM", "LTMCounter");
  ILOG(Info, Support) << "Setting mCCDBPathObjectLTM to " << mCCDBPathObjectLTM << ENDM;
  mCCDBPathObjectTRM = config.get<std::string>(baseJsonPath + "CCDBPathObjectTRM", "TRMCounter");
  ILOG(Info, Support) << "Setting mCCDBPathObjectTRM to " << mCCDBPathObjectTRM << ENDM;
}

void PostProcessDiagnosticPerCrate::initialize(Trigger, framework::ServiceRegistryRef services)
{
  int counter = 0;
  for (auto& i : mCrates) {
    i.reset(new TH2F(Form("hCrate%02i", counter),
                     Form("Crate%02i;Word;Slot", counter),
                     mNWords, 0, mNWords, mNSlots, 0, mNSlots));
    counter++;
  }

  // Setting up services
  mDatabase = &services.get<o2::quality_control::repository::DatabaseInterface>();
}

void PostProcessDiagnosticPerCrate::update(Trigger t, framework::ServiceRegistryRef)
{
  ILOG(Debug) << "UPDATING !" << ENDM;
  for (int slot = 0; slot < mNSlots; slot++) { // Loop over slots
    std::string moName = mCCDBPathObjectDRM;
    if (slot == 1) {
      moName = mCCDBPathObjectLTM;
    } else if (slot > 1) {
      moName = Form("%s%i", mCCDBPathObjectTRM.c_str(), slot);
    }
    ILOG(Debug) << "Processing slot " << slot << " from " << moName << ENDM;
    auto mo = mDatabase->retrieveMO(mCCDBPath, moName, t.timestamp, t.activity);
    TH2F* moH = static_cast<TH2F*>(mo ? mo->getObject() : nullptr);
    if (moH) {
      for (int crate = 0; crate < moH->GetNbinsY(); crate++) { // Loop over crates
        ILOG(Debug) << "Processing crate " << crate << ENDM;
        if (static_cast<unsigned int>(crate) > mCrates.size()) {
          ILOG(Fatal) << "Crate counter is too large " << ENDM;
        }
        for (int word = 0; word < moH->GetNbinsX(); word++) { // Loop over words
          if (word > mNWords) {
            ILOG(Fatal) << "Word counter is too large " << ENDM;
          }
          mCrates[crate]->SetBinContent(word + 1, slot + 1, moH->GetBinContent(word + 1, crate + 1));
        }
      }
    } else {
      ILOG(Warning) << "Did not find MO " << moName << " in path " << mCCDBPath << ENDM;
    }
  }
  ILOG(Debug) << "DONE UPDATING !" << ENDM;
}

void PostProcessDiagnosticPerCrate::finalize(Trigger t, framework::ServiceRegistryRef)
{
  ILOG(Info, Support) << "FINALIZING !" << ENDM;

  for (auto& i : mCrates) {
    TCanvas* c = new TCanvas(i->GetName(), i->GetName());
    i->Draw();
    auto mo = std::make_shared<o2::quality_control::core::MonitorObject>(c, "PostProcessDiagnosticPerCrate", "o2::quality_control_modules::tof::PostProcessDiagnosticPerCreate", "TOF");
    mo->setIsOwner(false);
    mDatabase->storeMO(mo);

    // It should delete everything inside. Confirmed by trying to delete histo after and getting a segfault.
    delete c;
  }
  ILOG(Info, Support) << "DONE FINALIZING !" << ENDM;
}

} // namespace o2::quality_control_modules::tof
