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
/// \file   BenchmarkTask.cxx
/// \author Barthelemy von Haller
///

#include "Example/BenchmarkTask.h"
#include <Configuration/ConfigurationFactory.h>
#include "QualityControl/QcInfoLogger.h"
#include <TCanvas.h>
#include <TH1.h>
#include <thread>

using namespace std;
using namespace o2::configuration;

namespace o2::quality_control_modules::example
{

// These two cannot be in the header, because ctors and dtors need to know how to construct and destruct
// ConfigurationInterface, which is only forward-declared in the header.
BenchmarkTask::BenchmarkTask() = default;

BenchmarkTask::~BenchmarkTask() = default;

void BenchmarkTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info) << "initialize benchmarktask \"" << getName() << "\""
             << ENDM;

  mConfigFile = ConfigurationFactory::getConfiguration("file:./example.ini");
  string prefix = "qc.tasks_config." + getName();
  string taskDefinitionName = mConfigFile->get<std::string>(prefix + ".taskDefinition");
  auto taskConfigTree = mConfigFile->getRecursive(taskDefinitionName);
  mNumberHistos = taskConfigTree.get<int>("numberHistos");
  mNumberChecks = taskConfigTree.get<int>("numberChecks");
  mTypeOfChecks = taskConfigTree.get<std::string>("typeOfChecks");
  mModuleOfChecks = taskConfigTree.get<std::string>("moduleOfChecks");

  mHistos.reserve(mNumberHistos);

  // Create and publish the histos
  for (size_t i = 0; i < mNumberHistos; i++) {
    assert(mHistos.empty()); // because otherwise the code below makes no sense.
    stringstream name;
    name << "histogram_" << getName() << "_" << i;
    mHistos.push_back(new TH1F(name.str().c_str(), name.str().c_str(), 1000, -5, 5));
    getObjectsManager()->startPublishing(mHistos[i]);
  }
}

void BenchmarkTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "startOfActivity" << ENDM;
}

void BenchmarkTask::startOfCycle()
{
  ILOG(Info) << "startOfCycle" << ENDM;
}

void BenchmarkTask::monitorData(o2::framework::ProcessingContext& /*ctx*/)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(100) /*100ms*/);
}

void BenchmarkTask::endOfCycle()
{
  for (auto histo : mHistos) {
    histo->Reset();
    histo->FillRandom("gaus", 1000);
  }
  ILOG(Info) << "endOfCycle" << ENDM;
}

void BenchmarkTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "endOfActivity" << ENDM;
}

void BenchmarkTask::reset() { ILOG(Info) << "Reset" << ENDM; }

} // namespace o2::quality_control_modules::example
