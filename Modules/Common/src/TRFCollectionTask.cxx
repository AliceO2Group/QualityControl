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
/// \file   PostProcessingInterface.cxx
/// \author Piotr Konopka
///

#include "Common/TRFCollectionTask.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/CcdbDatabase.h"
#include "QualityControl/RepoPathUtils.h"
#include "QualityControl/QualitiesToTRFCollectionConverter.h"

#include <DataFormatsQualityControl/TimeRangeFlagCollection.h>
#include <DataFormatsQualityControl/TimeRangeFlag.h>
#include <DataFormatsQualityControl/FlagReasons.h>

#include <optional>

using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control::core;
using namespace o2::quality_control;

namespace o2::quality_control_modules::common
{

TRFCollectionTask::~TRFCollectionTask()
{
}

void TRFCollectionTask::configure(std::string name, const boost::property_tree::ptree& config)
{
  mConfig = TRFCollectionTaskConfig(name, config);
}

void TRFCollectionTask::initialize(Trigger t, framework::ServiceRegistry&)
{
  mLastTimestampLimitStart = t.timestamp;
}

TimeRangeFlagCollection TRFCollectionTask::transformQualities(repository::DatabaseInterface& qcdb, const uint64_t timestampLimitStart, const uint64_t timestampLimitEnd)
{
  // ------ HELPERS ------
  std::function<std::vector<uint64_t>(const std::string& /*QO*/)> fetchAvailableTimestamps;
  try {
    fetchAvailableTimestamps = [&qcdbAsCcdb = dynamic_cast<repository::CcdbDatabase&>(qcdb), &detector = mConfig.detector](const std::string& qo) {
      std::string path = RepoPathUtils::getQoPath(detector, qo);
      return qcdbAsCcdb.getTimestampsForObject(path);
    };
  } catch (std::bad_cast& ex) {
    ILOG(Error) << "Could not cast the database interface to CcdbDatabase, this task supports only the CCDB backend" << ENDM
  }

  // ------ IMPLEMENTATION ------
  // stats
  size_t totalQOsIncluded = 0;
  size_t totalWorseThanGoodQOs = 0;

  TimeRangeFlagCollection mainTrfCollection{ mConfig.name, mConfig.detector };

  for (const auto& qoName : mConfig.qualityObjects) {

    std::string qoPath = RepoPathUtils::getQoPath(mConfig.detector, qoName);
    QualitiesToTRFCollectionConverter converter(mConfig.name, mConfig.detector, timestampLimitStart, timestampLimitEnd, qoPath);

    auto availableTimestamps = fetchAvailableTimestamps(qoName);
    auto firstMatchingTimestamp = std::upper_bound(availableTimestamps.begin(), availableTimestamps.end(), timestampLimitStart);

    if (firstMatchingTimestamp == availableTimestamps.end()) {
      ILOG(Warning) << "No object under the path '" << qoPath << "' available after timestamp '" << timestampLimitStart << "'" << ENDM;
      continue;
    }

    // if available, we move one timestamp back, because 'validUntil' might cover our period.
    if (firstMatchingTimestamp != availableTimestamps.begin()) {

      auto currentObjTimestamp = firstMatchingTimestamp - 1;
      auto qo = qcdb.retrieveQO(qoPath, *currentObjTimestamp);
      if (qo == nullptr) {
        throw std::runtime_error("Could not retrieve a QO for timestamp '" + std::to_string(*currentObjTimestamp) + "'");
      }
      converter(*qo);
    }

    // the main loop over QOs
    for (auto currentStartTime = firstMatchingTimestamp;
         currentStartTime != availableTimestamps.end() && *currentStartTime < timestampLimitEnd;
         currentStartTime++) {

      auto newQO = qcdb.retrieveQO(qoPath, *currentStartTime);
      if (newQO == nullptr) {
        throw std::runtime_error("Could not retrieve a QO for timestamp '" + std::to_string(*currentStartTime) + "'");
      }

      converter(*newQO);
    }

    totalQOsIncluded += converter.getQOsIncluded();
    totalWorseThanGoodQOs += converter.getWorseThanGoodQOs();
    mainTrfCollection.merge(*converter.getResult());
  }

  ILOG(Info) << "Total number of QOs included in TRFCollection: " << totalQOsIncluded << ENDM;
  ILOG(Info) << "Total number of worse than good QOs: " << totalWorseThanGoodQOs << ENDM;
  ILOG(Info) << "Number of TRFs: " << mainTrfCollection.size() << ENDM;
  // TODO: now we print it, but it should be stored in the QCDB after we have QC-547.
  ILOG(Info) << mainTrfCollection << ENDM;

  return mainTrfCollection;
}

void TRFCollectionTask::update(Trigger, framework::ServiceRegistry&)
{
  throw std::runtime_error("Only two timestamps should be given to the task");
}

void TRFCollectionTask::finalize(Trigger t, framework::ServiceRegistry& services)
{
  auto timestampLimitEnd = t.timestamp;

  auto& qcdb = services.get<repository::DatabaseInterface>();
  auto trfCollection = transformQualities(qcdb, mLastTimestampLimitStart, timestampLimitEnd);

  mLastTimestampLimitStart = timestampLimitEnd;
}

} // namespace o2::quality_control_modules::common
