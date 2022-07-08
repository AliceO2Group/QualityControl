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
#include "DataFormatsFT0/Digit.h"

#include <TH1F.h>
#include <TH2.h>
#include <typeinfo>
#include "Rtypes.h"

using namespace o2::quality_control::postprocessing;

namespace o2::quality_control_modules::ft0
{

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
    mPathDigitQcTask = "FT0/MO/DigitQcTask/";
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
  mMapDigitTrgNames.insert({ o2::ft0::Triggers::bitLaser, "Laser" });
  mMapDigitTrgNames.insert({ o2::ft0::Triggers::bitOutputsAreBlocked, "OutputsAreBlocked" });
  mMapDigitTrgNames.insert({ o2::ft0::Triggers::bitDataIsValid, "DataIsValid" });

  mHistBcTrgOutOfBunchColl = std::make_unique<TH2F>("OutOfBunchColl_BCvsTrg", "BC vs Triggers for out-of-bunch collisions;BC;Triggers", 3564, 0, 3564, mMapDigitTrgNames.size(), 0, mMapDigitTrgNames.size());
  mHistBcPattern = std::make_unique<TH2F>("bcPattern", "BC pattern", 3564, 0, 3564, mMapDigitTrgNames.size(), 0, mMapDigitTrgNames.size());
  for (const auto& entry : mMapDigitTrgNames) {
    mHistBcTrgOutOfBunchColl->GetYaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());
    mHistBcPattern->GetYaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());
  }
  getObjectsManager()->startPublishing(mHistBcTrgOutOfBunchColl.get());
  getObjectsManager()->startPublishing(mHistBcPattern.get());
  getObjectsManager()->setDefaultDrawOptions(mHistBcTrgOutOfBunchColl.get(), "COLZ");
  getObjectsManager()->setDefaultDrawOptions(mHistBcPattern.get(), "COLZ");
}

void OutOfBunchCollTask::update(Trigger t, framework::ServiceRegistry&)
{
  std::map<std::string, std::string> metadata;
  std::map<std::string, std::string> headers;
  const auto* bcPattern = mCcdbApi.retrieveFromTFileAny<o2::BunchFilling>(mPathBunchFilling, metadata, -1, &headers);
  if (!bcPattern) {
    ILOG(Error, Support) << "object \"" << mPathBunchFilling << "\" NOT retrieved!!!" << ENDM;
    return;
  }
  const int nBc = 3564;
  mHistBcPattern->Reset();
  for (int i = 0; i < nBc + 1; i++) {
    for (int j = 0; j < mMapDigitTrgNames.size() + 1; j++) {
      mHistBcPattern->SetBinContent(i + 1, j + 1, bcPattern->testBC(i));
    }
  }
  auto mo = mDatabase->retrieveMO(mPathDigitQcTask, "BCvsTriggers", t.timestamp, t.activity);
  auto hBcVsTrg = mo ? (TH2F*)mo->getObject() : nullptr;
  if (!hBcVsTrg) {
    ILOG(Error, Support) << "MO \"BCvsTriggers\" NOT retrieved!!!" << ENDM;
    return;
  }

  mHistBcTrgOutOfBunchColl->Reset();
  float vmax = hBcVsTrg->GetBinContent(hBcVsTrg->GetMaximumBin());
  mHistBcTrgOutOfBunchColl->Add(hBcVsTrg, mHistBcPattern.get(), 1, -1 * vmax);
  for (int i = 0; i < nBc + 1; i++) {
    for (int j = 0; j < mMapDigitTrgNames.size() + 1; j++) {
      if (mHistBcTrgOutOfBunchColl->GetBinContent(i + 1, j + 1) < 0) {
        mHistBcTrgOutOfBunchColl->SetBinContent(i + 1, j + 1, 0); // is it too slow?
      }
    }
  }
  mHistBcTrgOutOfBunchColl->SetEntries(mHistBcTrgOutOfBunchColl->Integral(1, nBc, 1, mMapDigitTrgNames.size()));
  for (int iBin = 1; iBin < mMapDigitTrgNames.size() + 1; iBin++) {
    const std::string metadataKey = "BcVsTrgIntegralBin" + std::to_string(iBin);
    const std::string metadataValue = std::to_string(mHistBcTrgOutOfBunchColl->Integral(1, nBc, iBin, iBin));
    getObjectsManager()->getMonitorObject(mHistBcTrgOutOfBunchColl->GetName())->addOrUpdateMetadata(metadataKey, metadataValue);
    ILOG(Info, Support) << metadataKey << ":" << metadataValue << ENDM;
  }
}

void OutOfBunchCollTask::finalize(Trigger t, framework::ServiceRegistry&)
{
}

} // namespace o2::quality_control_modules::ft0
