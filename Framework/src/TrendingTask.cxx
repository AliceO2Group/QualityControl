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
/// \file    TrendingTask.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/TrendingTask.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/MonitorObject.h"
#include <Configuration/ConfigurationInterface.h>
#include <TH1.h>
//#include <TGraph.h>
#include <TCanvas.h>
#include <ctime>
using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;

const Int_t TrendSizeBase = 10;

void TrendingTask::configure(std::string name, o2::configuration::ConfigurationInterface& config)
{
  mConfig = TrendingTaskConfig(name, config);
}

void TrendingTask::initialize(Trigger, framework::ServiceRegistry& services)
{
  // todo: react to the configuration: TGraph, TTree, ApacheArrow stuff
  mTrend = std::make_unique<TGraph>();
  if (!mTrend) {
    throw std::runtime_error("Could not create TGraph");
  }
  mPoints = 0;
  mTrend->SetName("MeanTrend");
  mTrend->Expand(TrendSizeBase);

  mDatabase = &services.get<repository::DatabaseInterface>();

  trend();
}

void TrendingTask::update(Trigger, framework::ServiceRegistry&)
{
  trend();
  store();
}

void TrendingTask::finalize(Trigger, framework::ServiceRegistry&)
{

  store();
}

void TrendingTask::store()
{
  QcInfoLogger::GetInstance() << "Storing TGraph" << AliceO2::InfoLogger::InfoLogger::endm;
  auto mo = std::make_shared<core::MonitorObject>(mTrend.get(), getName(), "TST");
  mo->setIsOwner(false);
  mDatabase->store(mo);
  TCanvas* c = new TCanvas();
  mTrend->Draw();
  c->SaveAs("trend.png");
  delete c;
}

void TrendingTask::trend()
{
  core::MonitorObject* mo = mDatabase->retrieve("qc/TST/QcTask", "example"); // fixme: use config values
  TObject* obj = mo ? mo->getObject() : nullptr;

  if (obj && strncmp(obj->ClassName(), "TH1", 3) == 0) {
    Double_t mean = reinterpret_cast<TH1*>(obj)->GetMean();
    Double_t x = static_cast<Double_t>(time(nullptr));
    QcInfoLogger::GetInstance() << "New trend value (" << x << ", " << mean  << ")" << AliceO2::InfoLogger::InfoLogger::endm;

    mTrend->SetPoint(mPoints++, x, mean);
    if (mPoints >= mTrend->GetN()) {
      mTrend->Expand(mTrend->GetN() + TrendSizeBase);
    }
  }
}

