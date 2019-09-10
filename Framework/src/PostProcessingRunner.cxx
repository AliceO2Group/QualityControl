// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#include "QualityControl/PostProcessingRunner.h"

#include "QualityControl/PostProcessingFactory.h"
#include "QualityControl/PostProcessingInterface.h"

#include <Configuration/ConfigurationFactory.h>

using namespace o2::configuration;

namespace o2::quality_control::postprocessing
{

PostProcessingRunner::PostProcessingRunner(std::string name, std::string configPath) //
  : mConfigFile(ConfigurationFactory::getConfiguration(configPath))
{
}

PostProcessingRunner::~PostProcessingRunner()
{
}

void PostProcessingRunner::init()
{

  PostProcessingConfig config{};

  // setup user's task
  PostProcessingFactory f;
  mTask.reset(f.create(config));

  // init user's task
  //  mTask->loadCcdb(mTaskConfig.conditionUrl);
  mTask->initialize(Trigger::Once);
}

void PostProcessingRunner::run()
{
}

void PostProcessingRunner::stop()
{
}

void PostProcessingRunner::reset()
{
}

} // namespace o2::quality_control::postprocessing