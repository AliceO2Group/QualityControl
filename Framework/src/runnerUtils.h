//
// Created by bvonhall on 16/04/19.
//

#ifndef QUALITYCONTROL_RUNNERUTILS_H
#define QUALITYCONTROL_RUNNERUTILS_H

#include <Configuration/ConfigurationFactory.h>

namespace o2::quality_control::core
{

/**
 * Returns the name of the first task encountered in the config file.
 * Ad-hoc solution to avoid hard-coding the task when we create the printer (he needs it to know the data description
 * of the data coming out of the checker).
 * @param config
 * @return The name of the first task in the config file.
 */
std::string getFirstTaskName(std::string configurationSource)
{
  auto config = o2::configuration::ConfigurationFactory::getConfiguration(configurationSource);

  for (const auto&[taskName, taskConfig] : config->getRecursive("qc.tasks")) {
    return taskName;
  }

  throw;
}

} // o2::quality_control::core

#endif //QUALITYCONTROL_RUNNERUTILS_H
