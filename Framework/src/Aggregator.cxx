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
/// \file   Aggregator.cxx
/// \author Barthelemy von Haller
///

#include <Configuration/ConfigurationInterface.h>
#include "QualityControl/Aggregator.h"
#include "QualityControl/RootClassFactory.h"
#include "QualityControl/AggregatorInterface.h"

using namespace o2::quality_control::checker;
using namespace o2::quality_control::core;
using namespace std;

Aggregator::Aggregator(const std::string& aggregatorName, boost::property_tree::ptree configuration)
{
  mAggregatorConfig.checkName = aggregatorName;
  mAggregatorConfig.moduleName = configuration.get<std::string>("moduleName", "");
  mAggregatorConfig.policyType = configuration.get<std::string>("policy", "");
  mAggregatorConfig.className = configuration.get<std::string>("className", "");
  mAggregatorConfig.detectorName = configuration.get<std::string>("detectorName", "");

  // Params
  if(configuration.count("aggregatorParameters")) {
    for (const auto& [_key, _value] : configuration.get_child("aggregatorParameters")) {
      mAggregatorConfig.customParameters[_key] = _value.data();
    }
  }

  // Inputs
  for (const auto& [_key, dataSource] : configuration.get_child("dataSource")) {
    (void)_key;

    if (auto sourceType = dataSource.get<std::string>("type");
        sourceType == "Checks" || sourceType == "Aggregators") {

      for (const auto& sourceName : dataSource.get_child("names")) {
        auto name = sourceName.second.get_value<std::string>();
        if (std::find(mAggregatorConfig.moNames.begin(), mAggregatorConfig.moNames.end(), name) == mAggregatorConfig.moNames.end()) {
          mAggregatorConfig.moNames.push_back(name);
        }
      }
    }
  }
}

void Aggregator::init()
{
  try {
    ILOG(Info, Devel) << "Insantiating the user code for aggregator " << mAggregatorConfig.checkName
                      << " (" << mAggregatorConfig.moduleName << ", " << mAggregatorConfig.className << ")" << ENDM;
     mAggregatorInterface = root_class_factory::create<AggregatorInterface>(mAggregatorConfig.moduleName, mAggregatorConfig.className);
     mAggregatorInterface->setCustomParameters(mAggregatorConfig.customParameters);
     mAggregatorInterface->configure(mAggregatorConfig.checkName);
  } catch (...) {
    std::string diagnostic = boost::current_exception_diagnostic_information();
    ILOG(Fatal, Ops) << "Unexpected exception, diagnostic information follows:\n"
                     << diagnostic << ENDM;
    throw;
  }

  // Print setting
  ILOG(Info, Ops) << mAggregatorConfig.checkName << ": Module " << mAggregatorConfig.moduleName << AliceO2::InfoLogger::InfoLogger::endm;
  ILOG(Info, Ops) << mAggregatorConfig.checkName << ": Class " << mAggregatorConfig.className << AliceO2::InfoLogger::InfoLogger::endm;
  ILOG(Info, Ops) << mAggregatorConfig.checkName << ": Detector " << mAggregatorConfig.detectorName << AliceO2::InfoLogger::InfoLogger::endm;
  ILOG(Info, Ops) << mAggregatorConfig.checkName << ": Policy " << mAggregatorConfig.policyType << AliceO2::InfoLogger::InfoLogger::endm;
  ILOG(Info, Ops) << mAggregatorConfig.checkName << ": QualityObjects : " << AliceO2::InfoLogger::InfoLogger::endm;
  for (const auto& moname : mAggregatorConfig.moNames) {
    ILOG(Info, Ops) << mAggregatorConfig.checkName << "   - " << moname << AliceO2::InfoLogger::InfoLogger::endm;
  }
}

QualityObjectsType Aggregator::aggregate(QualityObjectsMapType& qoMap)
{
  std::vector<Quality> results = mAggregatorInterface->aggregate(qoMap);
  QualityObjectsType qualityObjects;
  for (auto quality : results) {
    qualityObjects.emplace_back(std::make_shared<QualityObject>(
      quality,
      mAggregatorConfig.checkName,
      mAggregatorConfig.detectorName,
      mAggregatorConfig.policyType));
  }
  return qualityObjects;
}

std::string Aggregator::getPolicyName() const
{
  return mAggregatorConfig.policyType;
}

std::vector<std::string> Aggregator::getObjectsNames() const
{
  return mAggregatorConfig.moNames;
}

bool Aggregator::getAllObjectsOption() const
{
  return mAggregatorConfig.allMOs;
}