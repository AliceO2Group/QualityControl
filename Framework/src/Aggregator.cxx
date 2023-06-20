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
/// \file   Aggregator.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/Aggregator.h"
#include "QualityControl/AggregatorSpec.h"
#include "QualityControl/RootClassFactory.h"
#include "QualityControl/AggregatorInterface.h"
#include "QualityControl/UpdatePolicyType.h"
#include "QualityControl/ActivityHelpers.h"
#include "QualityControl/Activity.h"
#include <Common/Exceptions.h>

#include <utility>

using namespace o2::quality_control::checker;
using namespace o2::quality_control::core;
using namespace std;

namespace o2::quality_control::checker
{

Aggregator::Aggregator(AggregatorConfig configuration) : mAggregatorConfig(std::move(configuration))
{

}

void Aggregator::init()
{
  try {
    ILOG(Info, Devel) << "Instantiating the user code for aggregator " << mAggregatorConfig.name
                      << " (" << mAggregatorConfig.moduleName << ", " << mAggregatorConfig.className << ")" << ENDM;
    mAggregatorInterface =
      root_class_factory::create<AggregatorInterface>(mAggregatorConfig.moduleName, mAggregatorConfig.className);
    mAggregatorInterface->setName(mAggregatorConfig.name);
    mAggregatorInterface->setCustomParameters(mAggregatorConfig.customParameters);
    mAggregatorInterface->configure();
  } catch (...) {
    std::string diagnostic = boost::current_exception_diagnostic_information();
    ILOG(Fatal, Ops) << "Unexpected exception, diagnostic information follows: "
                     << diagnostic << ENDM;
    throw;
  }

  // Print setting
  ILOG(Info, Support) << mAggregatorConfig.name << ": Module " << mAggregatorConfig.moduleName << ENDM;
  ILOG(Info, Support) << mAggregatorConfig.name << ": Class " << mAggregatorConfig.className << ENDM;
  ILOG(Info, Support) << mAggregatorConfig.name << ": Detector " << mAggregatorConfig.detectorName << ENDM;
  ILOG(Info, Support) << mAggregatorConfig.name << ": Policy " << UpdatePolicyTypeUtils::ToString(mAggregatorConfig.policyType) << ENDM;
  ILOG(Info, Support) << mAggregatorConfig.name << ": QualityObjects : " << ENDM;
  for (const auto& moname : mAggregatorConfig.objectNames) {
    ILOG(Info, Support) << mAggregatorConfig.name << "   - " << moname << ENDM;
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
    auto it = std::find_if(mAggregatorConfig.sources.begin(), mAggregatorConfig.sources.end(),
                           [&local](const AggregatorSource source) {
                             std::string token = local->getCheckName().substr(0, local->getCheckName().find("/"));
                             return token == source.name;
                           });

    // if no source found, it is not here
    if (it == mAggregatorConfig.sources.end()) {
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

QualityObjectsType Aggregator::aggregate(QualityObjectsMapType& qoMap, const Activity& defaultActivity)
{
  auto filtered = filter(qoMap);

  Activity resultActivity;
  if (filtered.empty()) {
    resultActivity = defaultActivity;
  } else {
    // Aggregated Quality validity is a union of all Qualities used to produce it.
    // This is to allow to trigger postprocessing on an update of the aggregated QualityObject
    // and get a validFrom timestamp which allows to access all the input QualityObjects as well.
    // Not sure if this is "correct", but I do not see a better solution at the moment...
    resultActivity = activity_helpers::overlappingActivity(
      filtered.begin(),
      filtered.end(),
      [](const std::pair<std::string, std::shared_ptr<const QualityObject>>& item) -> const Activity& {
        return item.second->getActivity();
      });
    if (resultActivity.mValidity.isInvalid()) {
      ILOG(Warning, Support) << "Overlapping validity of inputs QOs to aggregator " << mAggregatorConfig.name << " is invalid (disjoint validities of input objects). Default activity will be used instead." << ENDM;
      resultActivity = defaultActivity;
    }
  }

  auto results = mAggregatorInterface->aggregate(filtered);
  QualityObjectsType qualityObjects;
  for (auto const& [qualityName, quality] : results) {
    qualityObjects.emplace_back(std::make_shared<QualityObject>(
      quality,
      mAggregatorConfig.name + "/" + qualityName,
      mAggregatorConfig.detectorName,
      UpdatePolicyTypeUtils::ToString(mAggregatorConfig.policyType)));
    qualityObjects.back()->setActivity(resultActivity);
  }
  return qualityObjects;
}

const std::string& Aggregator::getName() const
{
  return mAggregatorConfig.name;
}

UpdatePolicyType Aggregator::getUpdatePolicyType() const
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

std::vector<AggregatorSource> Aggregator::getSources() const
{
  return mAggregatorConfig.sources;
}

std::vector<AggregatorSource> Aggregator::getSources(core::DataSourceType type)
{
  std::vector<AggregatorSource> matches;
  std::copy_if(mAggregatorConfig.sources.begin(), mAggregatorConfig.sources.end(), std::back_inserter(matches), [&](const AggregatorSource& source) {
    return source.type == type;
  });
  return matches;
}

AggregatorConfig Aggregator::extractConfig(const core::CommonSpec&, const AggregatorSpec& aggregatorSpec)
{
  framework::Inputs inputs;
  std::vector<std::string> objectNames;
  UpdatePolicyType updatePolicy = aggregatorSpec.updatePolicy;
  bool checkAllObjects = false;
  std::vector<AggregatorSource> sources;
  ILOG(Info, Devel) << "Extracting configuration of a new aggregator " << aggregatorSpec.aggregatorName << ENDM;
  for (const auto& dataSource : aggregatorSpec.dataSources) {
    if (!dataSource.isOneOf(DataSourceType::Check, DataSourceType::Aggregator)) {
      throw std::runtime_error(
        "Unsupported dataSource '" + dataSource.name + "' for an Aggregator '" + aggregatorSpec.aggregatorName + "'");
    }
    ILOG(Info, Devel) << "   Found a source : " << dataSource.name << ENDM;
    AggregatorSource source(dataSource.type, dataSource.name);

    if (dataSource.type == DataSourceType::Check) { // Aggregator results do not come from DPL inputs
      inputs.insert(inputs.end(), dataSource.inputs.begin(), dataSource.inputs.end());
    }

    // Subscribe on predefined MOs.
    // If "MOs" are not set, the check function will be triggered whenever a new MO appears.
    if (dataSource.subInputs.empty()) {
      ILOG(Info, Devel) << "      (no QOs specified, we take all)" << ENDM;
      checkAllObjects = true;
      updatePolicy = UpdatePolicyType::OnGlobalAny;
    } else {
      for (const auto& qoName : dataSource.subInputs) {
        auto name = dataSource.name + "/" + qoName;
        ILOG(Info, Devel) << "      - " << name << ENDM;
        objectNames.push_back(name);
        source.objects.push_back(name);
      }
    }
    sources.emplace_back(source);
  }

  return {
    aggregatorSpec.aggregatorName,
    aggregatorSpec.moduleName,
    aggregatorSpec.className,
    aggregatorSpec.detectorName,
    aggregatorSpec.customParameters,
    updatePolicy,
    std::move(objectNames),
    checkAllObjects,
    std::move(inputs),
    sources
  };
}

} // namespace o2::quality_control::checker
