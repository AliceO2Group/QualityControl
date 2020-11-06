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

const std::string PostProcessDiagnosticPerCrate::mCCDBPath = "qc/TOF/MO/TaskDiagnostics/";
const int PostProcessDiagnosticPerCrate::mNWords = 32;
const int PostProcessDiagnosticPerCrate::mNSlots = 14;

PostProcessDiagnosticPerCrate::~PostProcessDiagnosticPerCrate()
{
}

void PostProcessDiagnosticPerCrate::initialize(Trigger, framework::ServiceRegistry& services)
{
  int counter = 0;
  for (auto& i : mCrates) {
    i.reset(new TH2F(Form("hCrate%i", counter),
                     Form("Crate%i;Word;Slot", counter),
                     mNWords, 0, mNWords, mNSlots, 0, mNSlots));
    counter++;
  }

  //    Setting up services
  mDatabase = &services.get<o2::quality_control::repository::DatabaseInterface>();
}

void PostProcessDiagnosticPerCrate::update(Trigger, framework::ServiceRegistry&)
{
  ILOG(Debug) << "PostProcessDiagnosticPerCrate" << ENDM;
  for (int slot = 0; slot < mNSlots; slot++) { // Loop over slots
    std::string moName = "DRMCounter";
    if (slot == 1) {
      moName = "LTMCounter";
    } else if (slot > 1) {
      moName = Form("TRMCounterSlot%i", slot);
    }
    ILOG(Debug) << "Processing slot " << slot << " from " << moName << ENDM;
    auto mo = mDatabase->retrieveMO(mCCDBPath, moName);
    TH2F* moH = static_cast<TH2F*>(mo ? mo->getObject() : nullptr);
    if (moH) {
      for (int crate = 0; crate < moH->GetNbinsY(); crate++) { // Loop over crates
        ILOG(Debug) << "Processing crate " << crate << ENDM;
        if (static_cast<unsigned int>(crate) > mCrates.size()) {
          ILOG(Fatal) << "Crate counter is too large " << ENDM;
        }
        for (int word = 0; word < moH->GetNbinsX(); word++) { // Loop over words
          ILOG(Debug) << "Processing word " << word << ENDM;
          if (word > mNWords) {
            ILOG(Fatal) << "Word counter is too large " << ENDM;
          }
          mCrates[crate]->SetBinContent(word + 1, slot + 1, moH->GetBinContent(word + 1, crate + 1));
        }
      }
    }
  }
  ILOG(Debug) << "DONE PostProcessDiagnosticPerCrate" << ENDM;
}

void PostProcessDiagnosticPerCrate::finalize(Trigger, framework::ServiceRegistry&)
{
  ILOG(Info) << "FINALIZING !" << ENDM;

  for (auto& i : mCrates) {
    TCanvas* c = new TCanvas(i->GetName(), i->GetName());
    i->Draw();
    auto mo = std::make_shared<o2::quality_control::core::MonitorObject>(c, "PostProcessDiagnosticPerCrate", "TOF");
    mo->setIsOwner(false);
    mDatabase->storeMO(mo);

    // It should delete everything inside. Confirmed by trying to delete histo after and getting a segfault.
    delete c;
  }
  ILOG(Info) << "DONE FINALIZING !" << ENDM;
}

} // namespace o2::quality_control_modules::tof
