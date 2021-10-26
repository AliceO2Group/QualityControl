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
/// \file   OutOfBunchCollTask.cxx
/// \author Sebastian Bysiak sbysiak@cern.ch
/// PostProcessing task which finds collisions not compatible with BC pattern

#include "CommonDataFormat/BunchFilling.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/DatabaseInterface.h"
#include "FT0/OutOfBunchCollTask.h"

#include <TH1F.h>
#include <TH2.h>
#include <typeinfo>
#include "Rtypes.h"

using namespace o2::quality_control::postprocessing;

namespace o2::quality_control_modules::ft0
{

OutOfBunchCollTask::~OutOfBunchCollTask()
{
  delete mListHistGarbage;
}

void OutOfBunchCollTask::configure(std::string, const boost::property_tree::ptree& config)
{
  mCcdbUrl = config.get_child("qc.config.conditionDB.url").get_value<std::string>();

  const char* configPath = Form("qc.postprocessing.%s", getName().c_str());
  ILOG(Info, Support) << "configPath = " << configPath << ENDM;
  auto node = config.get_child_optional(Form("%s.custom.pathDigitQcTask", configPath));
  if (node) {
    mPathDigitQcTask = node.get_ptr()->get_child("").get_value<std::string>();
    ILOG(Info, Support) << "configure() : using pathDigitQcTask = \"" << mPathDigitQcTask << "\"" << ENDM;
  } else {
    mPathDigitQcTask = "qc/FT0/MO/DigitQcTask/";
    ILOG(Info, Support) << "configure() : using default pathDigitQcTask = \"" << mPathDigitQcTask << "\"" << ENDM;
  }

  node = config.get_child_optional(Form("%s.custom.pathBunchFilling", configPath));
  if (node) {
    mPathBunchFilling = node.get_ptr()->get_child("").get_value<std::string>();
    ILOG(Info, Support) << "configure() : using pathBunchFilling = \"" << mPathBunchFilling << "\"" << ENDM;
  } else {
    mPathBunchFilling = "GLO/GRP/BunchFilling";
    ILOG(Info, Support) << "configure() : using default pathBunchFilling = \"" << mPathBunchFilling << "\"" << ENDM;
  }
}

void OutOfBunchCollTask::initialize(Trigger, framework::ServiceRegistry& services)
{
  mDatabase = &services.get<o2::quality_control::repository::DatabaseInterface>();
  mCcdbApi.init(mCcdbUrl);

  mMapDigitTrgNames.insert({ o2::ft0::Triggers::bitA, "OrA" });
  mMapDigitTrgNames.insert({ o2::ft0::Triggers::bitC, "OrC" });
  mMapDigitTrgNames.insert({ o2::ft0::Triggers::bitVertex, "Vertex" });
  mMapDigitTrgNames.insert({ o2::ft0::Triggers::bitCen, "Central" });
  mMapDigitTrgNames.insert({ o2::ft0::Triggers::bitSCen, "SemiCentral" });

  mListHistGarbage = new TList();
  mListHistGarbage->SetOwner(kTRUE);
  for (const auto& entry : mMapDigitTrgNames) {
    auto pairOutOfBunchColl = mMapOutOfBunchColl.insert({ entry.first, new TH2F(Form("OutOfBunchColl_Trg%s", entry.second.c_str()), Form("BC-orbit map for out-of-bunch collisions: %s fired;Orbit;BC", entry.second.c_str()), 256, 0, 256, 3564, 0, 3564) });
    if (pairOutOfBunchColl.second) {
      getObjectsManager()->startPublishing(pairOutOfBunchColl.first->second);
      mListHistGarbage->Add(pairOutOfBunchColl.first->second);
    }
  }
  for (auto& entry : mMapOutOfBunchColl) {
    entry.second->SetOption("colz");
  }
  mHistBcPattern = std::make_unique<TH2F>("bcPattern", "BC pattern", 256, 0, 256, 3564, 0, 3564);
  mHistBcPattern->SetOption("colz");
  getObjectsManager()->startPublishing(mHistBcPattern.get());
}

void OutOfBunchCollTask::update(Trigger, framework::ServiceRegistry&)
{
  std::map<std::string, std::string> metadata;
  std::map<std::string, std::string> headers;
  const auto* bcPattern = mCcdbApi.retrieveFromTFileAny<o2::BunchFilling>(mPathBunchFilling, metadata, -1, &headers);
  if (!bcPattern) {
    ILOG(Error, Support) << "object \"" << mPathBunchFilling << "\" NOT retrieved!!!"
                         << ENDM;
    return;
  }
  const int nBc = 3564;
  const int nOrbits = 256;
  mHistBcPattern->Reset();
  for (int j = 0; j < nOrbits + 1; j++)
    for (int i = 0; i < nBc + 1; i++)
      mHistBcPattern->SetBinContent(j + 1, i + 1, bcPattern->testBC(i));

  for (auto& entry : mMapOutOfBunchColl) {
    auto moName = Form("BcOrbitMap_Trg%s", mMapDigitTrgNames.at(entry.first).c_str());
    auto mo = mDatabase->retrieveMO(mPathDigitQcTask, moName);
    auto hBcOrbitMapTrg = mo ? (TH2F*)mo->getObject() : nullptr;
    if (!hBcOrbitMapTrg) {
      ILOG(Error, Support) << "MO \"" << moName << "\" NOT retrieved!!!"
                           << ENDM;
      continue;
    }
    entry.second->Reset();
    // scale bc pattern by vmax to make sure the difference is non positive
    float vmax = hBcOrbitMapTrg->GetBinContent(hBcOrbitMapTrg->GetMaximumBin());
    entry.second->Add(hBcOrbitMapTrg, mHistBcPattern.get(), 1, -1 * vmax);
    for (int j = 0; j < nOrbits + 1; j++)
      for (int i = 0; i < nBc + 1; i++)
        if (entry.second->GetBinContent(j + 1, i + 1) < 0)
          entry.second->SetBinContent(j + 1, i + 1, 0); // is it too slow?
    entry.second->SetEntries(entry.second->Integral());
    getObjectsManager()->addMetadata(entry.second->GetName(), "BcOrbitMapIntegral", std::to_string(hBcOrbitMapTrg->Integral()));
  }
}

void OutOfBunchCollTask::finalize(Trigger, framework::ServiceRegistry&)
{
}

} // namespace o2::quality_control_modules::ft0
