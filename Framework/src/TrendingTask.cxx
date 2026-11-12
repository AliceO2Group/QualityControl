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
/// \file    TrendingTask.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/TrendingTask.h"
#include "QualityControl/TrendingMachine.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/Reductor.h"
#include "QualityControl/ReductorHelpers.h"
#include "QualityControl/RootClassFactory.h"
#include "QualityControl/RepoPathUtils.h"
#include "QualityControl/ActivityHelpers.h"

#include <TH1.h>
#include <TCanvas.h>
#include <TPaveText.h>
#include <TGraphErrors.h>
#include <TPoint.h>
#include <TStyle.h>
#include <TLegend.h>
#include <TTree.h>

#include <boost/algorithm/string.hpp>
#include <set>

using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;

TrendingTask::TrendingTask() = default;
TrendingTask::~TrendingTask() = default;

void TrendingTask::configure(const boost::property_tree::ptree& config)
{
  // we clear any existing objects, which would be there only in case of reconfiguration
  // at the time of writing, this not even supported by ECS
  mTrendingMachine.reset();

  // configuration
  mConfig = TrendingTaskConfig(getID(), config);
  mTrendingMachine = std::make_unique<TrendingMachine>(mConfig.machine);
}

void TrendingTask::initializeTrend(o2::quality_control::repository::DatabaseInterface& qcdb)
{
  std::unique_ptr<TTree> existingTree;

  // Try to retrieve existing tree from QCDB if resuming
  if (mConfig.resumeTrend) {
    ILOG(Info, Support) << "Trying to retrieve an existing TTree for this task to continue the trend." << ENDM;
    auto path = RepoPathUtils::getMoPath(mConfig.detectorName, PostProcessingInterface::getName(), "", "", false);
    auto mo = qcdb.retrieveMO(path, PostProcessingInterface::getName(), repository::DatabaseInterface::Timestamp::Latest);
    if (mo && mo->getObject()) {
      auto tree = dynamic_cast<TTree*>(mo->getObject());
      if (tree) {
        mo->setIsOwner(false); // TrendingMachine will take ownership
        existingTree.reset(tree);
      }
    } else {
      ILOG(Warning, Support) << "Could not retrieve an existing TTree for this task" << ENDM;
    }
  }

  mTrendingMachine->initializeTrend(PostProcessingInterface::getName(), std::move(existingTree));
}

void TrendingTask::initialize(Trigger, framework::ServiceRegistryRef services)
{
  // removing leftovers from any previous runs
  mPlots.clear();

  initializeTrend(services.get<repository::DatabaseInterface>());

  if (mConfig.producePlotsOnUpdate) {
    getObjectsManager()->startPublishing(mTrendingMachine->getTrend(), PublicationPolicy::ThroughStop);
  }
}

// todo: see if OptimizeBaskets() indeed helps after some time
void TrendingTask::update(Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();

  const auto allSourcesInvoked = trendValues(t, qcdb);
  if (mConfig.producePlotsOnUpdate && (!mConfig.trendIfAllInputs || allSourcesInvoked)) {
    generatePlots();
  }
}

void TrendingTask::finalize(Trigger, framework::ServiceRegistryRef)
{
  if (!mConfig.producePlotsOnUpdate) {
    getObjectsManager()->startPublishing(mTrendingMachine->getTrend());
  }
  generatePlots();
}

bool TrendingTask::trendValues(const Trigger& t, repository::DatabaseInterface& qcdb)
{
  UInt_t timestamp;
  if (mConfig.trendingTimestamp == "trigger") {
    // ROOT expects seconds since epoch.
    timestamp = t.timestamp / 1000;
  } else if (mConfig.trendingTimestamp == "validFrom") {
    timestamp = t.activity.mValidity.getMin() / 1000;
  } else { // validUntil
    timestamp = t.activity.mValidity.getMax() / 1000;
  }

  mTrendingMachine->setMetadata(timestamp, t.activity.mId);

  const bool wereAllSourcesInvoked = mTrendingMachine->updateReductors([&](const auto& dataSource, Reductor& reductor) {
    return reductor_helpers::updateReductor(&reductor, t, dataSource, qcdb, (core::ConditionAccess&)*this);
  });

  if (!mConfig.trendIfAllInputs || wereAllSourcesInvoked) {
    mTrendingMachine->fillTrend();
  }

  return wereAllSourcesInvoked;
}

void TrendingTask::generatePlots()
{
  // Before we generate any new plots, we have to delete existing under the same names.
  // It seems that ROOT cannot handle an existence of two canvases with a common name in the same process.
  mPlots.clear();

  auto plots = mTrendingMachine->generatePlots();
  for (auto& plot : plots) {
    // Store plots to keep them alive
    auto* published = plot.get();
    mPlots[published->GetName()] = std::move(plot);
    getObjectsManager()->startPublishing(published, PublicationPolicy::Once);
  }
}
