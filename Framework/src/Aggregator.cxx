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
#include <Common/Exceptions.h>

using namespace o2::quality_control::checker;
using namespace o2::quality_control::core;
using namespace std;

namespace o2::quality_control::checker
{

Aggregator::Aggregator(const std::string& aggregatorName, const boost::property_tree::ptree& configuration)
{
  mAggregatorConfig.name = aggregatorName;
  mAggregatorConfig.moduleName = configuration.get<std::string>("moduleName", "");
  mAggregatorConfig.policyType = configuration.get<std::string>("policy", "");
  mAggregatorConfig.className = configuration.get<std::string>("className", "");
  mAggregatorConfig.detectorName = configuration.get<std::string>("detectorName", "");

  // Params
  if (configuration.count("aggregatorParameters")) {
    for (const auto& [key, value] : configuration.get_child("aggregatorParameters")) {
      mAggregatorConfig.customParameters[key] = value.data();
    }
  }

  ILOG(Info, Devel) << "Creation of a new aggregator " << aggregatorName << ENDM;
  for (const auto& [_key, dataSource] : configuration.get_child("dataSource")) { // loop through data sources
    (void)_key;

    if (auto sourceType = dataSource.get<std::string>("type"); sourceType == "Aggregator" || sourceType == "Check") {
      auto sourceName = dataSource.get<std::string>("name");
      ILOG(Info, Devel) << "   Found a source : " << sourceName << ENDM;
      AggregatorSource source(sourceType, sourceName);

      // Get the QOs for this source (if any)
      vector<string> qos;
      if (dataSource.count("QOs") == 0) {
        ILOG(Info, Devel) << "      (no QOs specified, we take all)" << ENDM;
        mAggregatorConfig.allObjects = true;
        mAggregatorConfig.policyType = "_OnGlobalAny";
      } else {
        for (const auto& qoName : dataSource.get_child("QOs")) {
          auto name = std::string(sourceName + "/" + qoName.second.get_value<std::string>());
          ILOG(Info, Devel) << "      - " << name << ENDM;
          mAggregatorConfig.objectNames.push_back(name);
          source.objects.push_back(name);
        }
      }
      mSources.emplace_back(source); // keep track of the sources
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
  for (const auto& moname : mAggregatorConfig.objectNames) {
    ILOG(Info, Ops) << mAggregatorConfig.name << "   - " << moname << AliceO2::InfoLogger::InfoLogger::endm;
  }
}

QualityObjectsMapType Aggregator::filter(QualityObjectsMapType& qoMap)
{
  // This is a basic implementation, if it needs to be more efficient it will have to be rethought.
  // for each qo in the list we receive, check if a source of this aggregator contains it (or rather
  // contains the first part of its checkName before `/`).

  QualityObjectsMapType result;
  for (auto const& [name, qo] : qoMap) {

    // find the source for this qo
    shared_ptr<const QualityObject> local = qo;
    auto it = std::find_if(mSources.begin(), mSources.end(),
                           [&local](const AggregatorSource source) {
                             std::string token = local->getCheckName().substr(0, local->getCheckName().find("/"));
                             return token == source.name;
                           });

    // if no source found, it is not here
    if (it == mSources.end()) {
      continue;
    }

    // search the qo in the objects of the source, if found we accept it.
    // if the source has no qos specified we accept it.
    auto source = *it;
    if (source.objects.empty() ||
        find(source.objects.begin(), source.objects.end(), name) != source.objects.end()) { // no qo specified, we accept all
      result[name] = qo;
    }
  }

  return result;
}

QualityObjectsType Aggregator::aggregate(QualityObjectsMapType& qoMap)
{
  auto filtered = filter(qoMap);
  auto results = mAggregatorInterface->aggregate(filtered);
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
  return mAggregatorConfig.objectNames;
}

bool Aggregator::getAllObjectsOption() const
{
  return mAggregatorConfig.allObjects;
}

std::vector<AggregatorSource> Aggregator::getSources()
{
  return mSources;
}

std::vector<AggregatorSource> Aggregator::getSources(AggregatorSourceType type)
{
  std::vector<AggregatorSource> matches;
  std::copy_if(mSources.begin(), mSources.end(), std::back_inserter(matches), [&](const AggregatorSource& source) {
    return source.type == type;
  });
  return matches;
}

AggregatorSource::AggregatorSource(const std::string& t, const std::string& n)
{
  if (t == "Aggregator") {
    type = aggregator;
  } else if (t == "Check") {
    type = check;
  } else {
    BOOST_THROW_EXCEPTION(AliceO2::Common::Exception() << AliceO2::Common::errinfo_details("Unknown type of Aggregator: " + t));
  }
  name = n;
}

}