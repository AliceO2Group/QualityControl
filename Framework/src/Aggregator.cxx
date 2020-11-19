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

Aggregator::Aggregator(const std::string& aggregatorName, const boost::property_tree::ptree& configuration)
{
  mAggregatorConfig.name = aggregatorName;
  mAggregatorConfig.moduleName = configuration.get<std::string>("moduleName", "");
  mAggregatorConfig.policyType = configuration.get<std::string>("policy", "");
  mAggregatorConfig.className = configuration.get<std::string>("className", "");
  mAggregatorConfig.detectorName = configuration.get<std::string>("detectorName", "");

  // Params
  if (configuration.count("aggregatorParameters")) {
    for (const auto& [_key, _value] : configuration.get_child("aggregatorParameters")) {
      mAggregatorConfig.customParameters[_key] = _value.data();
    }
  }

  ILOG(Info, Devel) << "Creation of a new aggregator " << aggregatorName << ENDM;
  for (const auto& [_key, dataSource] : configuration.get_child("dataSource")) { // loop through data sources
    (void)_key;

    if (auto sourceType = dataSource.get<std::string>("type"); sourceType == "Aggregator" || sourceType == "Check") {
      auto sourceName = dataSource.get<std::string>("name");
      ILOG(Info, Devel) << "   Found a source : " << sourceName << ENDM;

      if (dataSource.count("QOs") == 0 || dataSource.get<std::string>("QOs") == "all") {
        ILOG(Info, Devel) << "      (no QOs specified or specified as `all`)" << ENDM;
        mAggregatorConfig.allMOs = true;
        // fixme: this is a dirty fix. Policies should be refactored, so this check won't be needed.
        if (mAggregatorConfig.policyType != "OnEachSeparately") {
          mAggregatorConfig.policyType = "_OnGlobalAny";
        }
      } else {
        for (const auto& qoName : dataSource.get_child("QOs")) {
          auto name = std::string(sourceName + "/" + qoName.second.get_value<std::string>());
          ILOG(Info, Devel) << "      - " << name << ENDM;
          mAggregatorConfig.moNames.push_back(name);
        }
      }
    }
  }
}

void Aggregator::init()
{
  try {
    ILOG(Info, Devel) << "Instantiating the user code for aggregator " << mAggregatorConfig.name
                      << " (" << mAggregatorConfig.moduleName << ", " << mAggregatorConfig.className << ")" << ENDM;
    mAggregatorInterface =
      root_class_factory::create<AggregatorInterface>(mAggregatorConfig.moduleName, mAggregatorConfig.className);
    mAggregatorInterface->setCustomParameters(mAggregatorConfig.customParameters);
    mAggregatorInterface->configure(mAggregatorConfig.name);
  } catch (...) {
    std::string diagnostic = boost::current_exception_diagnostic_information();
    ILOG(Fatal, Ops) << "Unexpected exception, diagnostic information follows:\n"
                     << diagnostic << ENDM;
    throw;
  }

  // Print setting
  ILOG(Info, Ops) << mAggregatorConfig.name << ": Module " << mAggregatorConfig.moduleName << AliceO2::InfoLogger::InfoLogger::endm;
  ILOG(Info, Ops) << mAggregatorConfig.name << ": Class " << mAggregatorConfig.className << AliceO2::InfoLogger::InfoLogger::endm;
  ILOG(Info, Ops) << mAggregatorConfig.name << ": Detector " << mAggregatorConfig.detectorName << AliceO2::InfoLogger::InfoLogger::endm;
  ILOG(Info, Ops) << mAggregatorConfig.name << ": Policy " << mAggregatorConfig.policyType << AliceO2::InfoLogger::InfoLogger::endm;
  ILOG(Info, Ops) << mAggregatorConfig.name << ": QualityObjects : " << AliceO2::InfoLogger::InfoLogger::endm;
  for (const auto& moname : mAggregatorConfig.moNames) {
    ILOG(Info, Ops) << mAggregatorConfig.name << "   - " << moname << AliceO2::InfoLogger::InfoLogger::endm;
  }
}

QualityObjectsType Aggregator::aggregate(QualityObjectsMapType& qoMap)
{
  auto results = mAggregatorInterface->aggregate(qoMap);
  QualityObjectsType qualityObjects;
  for (auto const& [qualityName, quality] : results) {
    qualityObjects.emplace_back(std::make_shared<QualityObject>(
      quality,
      mAggregatorConfig.name + "/" + qualityName,
      mAggregatorConfig.detectorName,
      mAggregatorConfig.policyType));
  }
  return qualityObjects;
}

const std::string& Aggregator::getName() const
{
  return mAggregatorConfig.name;
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