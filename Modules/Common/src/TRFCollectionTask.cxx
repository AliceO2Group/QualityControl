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

  const char* noQualityObjectsComment = "No Quality Objects found within the specified time range";

  // ------ IMPLEMENTATION ------
  // stats
  size_t totalQOsIncluded = 0;
  size_t totalWorseThanGoodQOs = 0;

  TimeRangeFlagCollection trfCollection{ mConfig.name, mConfig.detector };

  for (const auto& qoName : mConfig.qualityObjects) {
    std::string qoPath = RepoPathUtils::getQoPath(mConfig.detector, qoName);

    auto availableTimestamps = fetchAvailableTimestamps(qoName);
    auto firstMatchingTimestamp = std::upper_bound(availableTimestamps.begin(), availableTimestamps.end(), timestampLimitStart);

    if (firstMatchingTimestamp == availableTimestamps.end()) {
      ILOG(Warning) << "No object under the path '" << qoPath << "' available after timestamp '" << timestampLimitStart << "'" << ENDM;
      trfCollection.insert({ timestampLimitStart, timestampLimitEnd, FlagReasonFactory::MissingQualityObject(), noQualityObjectsComment, qoName });
      continue;
    }

    std::optional<TimeRangeFlag> currentTRF;
    auto currentEndTime = *firstMatchingTimestamp;
    // if available, we move one timestamp back, because 'validUntil' might cover our period.
    if (firstMatchingTimestamp != availableTimestamps.begin()) {

      auto currentObjTimestamp = firstMatchingTimestamp - 1;
      auto qo = qcdb.retrieveQO(qoPath, *currentObjTimestamp);
      if (qo == nullptr) {
        throw std::runtime_error("Could not retrieve a QO for timestamp '" + std::to_string(*currentObjTimestamp) + "'");
      }
      uint64_t validUntil = strtoull(qo->getMetadata("Valid-Until").c_str(), nullptr, 10);
      currentEndTime = validUntil > timestampLimitEnd ? timestampLimitEnd : validUntil;

      if (currentEndTime > timestampLimitStart && qo->getQuality().isWorseThan(core::Quality::Good)) {
        // todo use reasons from QOs when they are available
        currentTRF.emplace(timestampLimitStart, currentEndTime, FlagReasonFactory::Unknown(), qo->getMetadata("comment", ""), qo->getName());
        totalQOsIncluded++;
        totalWorseThanGoodQOs++;
      }
    } // otherwise, let's see if we have QO coverage at the beginning of the time range
    else if (*firstMatchingTimestamp > timestampLimitStart && !currentTRF.has_value()) {
      trfCollection.insert({ timestampLimitStart, *firstMatchingTimestamp, FlagReasonFactory::MissingQualityObject(), noQualityObjectsComment, qoPath });
    }

    // the main loop over QOs
    for (auto currentStartTime = firstMatchingTimestamp;
         currentStartTime != availableTimestamps.end() && *currentStartTime < timestampLimitEnd;
         currentStartTime++) {

      auto newQO = qcdb.retrieveQO(qoPath, *currentStartTime);
      if (newQO == nullptr) {
        throw std::runtime_error("Could not retrieve a QO for timestamp '" + std::to_string(*currentStartTime) + "'");
      }
      totalQOsIncluded++;
      if (newQO->getQuality().isWorseThan(Quality::Good)) {
        totalWorseThanGoodQOs++;
      }

      uint64_t validUntil = strtoull(newQO->getMetadata("Valid-Until").c_str(), nullptr, 10);
      currentEndTime = validUntil > timestampLimitEnd ? timestampLimitEnd : validUntil;

      if (!currentTRF.has_value() && newQO->getQuality().isWorseThan(Quality::Good)) {
        // There was no TRF in the previous step and the data quality is bad now.
        // We create a new TRF and we will work on it in next loop iterations.
        currentTRF.emplace(*currentStartTime, currentEndTime, FlagReasonFactory::Unknown(), newQO->getMetadata("comment", ""), newQO->getName());

      } else if (currentTRF.has_value()) {
        // There is already a TRF. We will check if it can be merged with the new QO.
        auto newFlag = FlagReasonFactory::Unknown(); // todo: use reasons from QOs
        auto newComment = newQO->getMetadata("comment", "");

        if (newQO->getQuality() == Quality::Good) {
          // The data quality is not bad anymore.
          // We trim the current TRF's time range if necessary and deposit it to the collection.
          if (currentTRF->getEnd() > *currentStartTime) {
            currentTRF->setEnd(*currentStartTime);
          }
          trfCollection.insert(currentTRF.value());
          currentTRF.reset();
        } else if (currentTRF->getFlag() != newFlag || currentTRF->getComment() != newComment) {
          // The data quality is still bad, but in a different way.
          // We trim the current TRF's time range if necessary and deposit it to the collection.
          // Then, we create a new TRF.
          if (currentTRF->getEnd() > *currentStartTime) {
            currentTRF->setEnd(*currentStartTime);
          }
          trfCollection.insert(currentTRF.value());

          currentTRF.emplace(*currentStartTime, currentEndTime, newFlag, newComment, newQO->getName());
        } else if (currentTRF->getEnd() < currentEndTime) {
          // The data quality is still bad and in the same way.
          // We extend the duration of the current TRF.
          currentTRF->setEnd(currentEndTime);
        }
        // If none of the above conditions was fulfilled,
        // then currentTRF covers larger time range than the new one, keep the old.
      }
    }

    // handling the end of the time range
    if (currentTRF.has_value()) {
      trfCollection.insert(currentTRF.value());
      if (currentTRF->getEnd() < timestampLimitEnd) {
        trfCollection.insert({ currentTRF->getEnd(), timestampLimitEnd, FlagReasonFactory::MissingQualityObject(), noQualityObjectsComment, qoPath });
      }
    } else if (currentEndTime < timestampLimitEnd) {
      trfCollection.insert({ currentEndTime, timestampLimitEnd, FlagReasonFactory::MissingQualityObject(), noQualityObjectsComment, qoPath });
    }
  }

  ILOG(Info) << "Total number of QOs included in TRFCollection: " << totalQOsIncluded << ENDM;
  ILOG(Info) << "Total number of worse than good QOs: " << totalWorseThanGoodQOs << ENDM;
  ILOG(Info) << "Number of TRFs: " << trfCollection.size() << ENDM;
  // TODO: now we print it, but it should be stored in the QCDB after we have QC-547.
  ILOG(Info) << trfCollection << ENDM;

  return trfCollection;
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
