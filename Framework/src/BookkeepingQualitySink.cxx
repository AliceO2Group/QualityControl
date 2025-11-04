// Copyright 2024 CERN and copyright holders of ALICE O2.
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
/// \file   BookkeepingQualitySink.cxx
/// \author Michal Tichak
///

#include "QualityControl/BookkeepingQualitySink.h"
#include <Framework/DataRefUtils.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/CompletionPolicyHelpers.h>
#include <Framework/DeviceSpec.h>
#include <DataFormatsQualityControl/QualityControlFlagCollection.h>
#include "QualityControl/QualitiesToFlagCollectionConverter.h"
#include "QualityControl/QualityObject.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/runnerUtils.h"

#include <BookkeepingApi/QcFlagServiceClient.h>
#include <CCDB/BasicCCDBManager.h>
#include <stdexcept>
#include <utility>
#include <QualityControl/Bookkeeping.h>

namespace o2::quality_control::core
{

void BookkeepingQualitySink::customizeInfrastructure(std::vector<framework::CompletionPolicy>& policies)
{
  using namespace o2::framework;
  auto matcher = [label = BookkeepingQualitySink::getLabel()](auto const& device) {
    return std::find(device.labels.begin(), device.labels.end(), label) != device.labels.end();
  };
  policies.emplace_back(CompletionPolicyHelpers::consumeWhenAny("BookkeepingQualitySinkCompletionPolicy", matcher));
}

void BookkeepingQualitySink::init(framework::InitContext& iCtx)
{
  Bookkeeping::getInstance().init(mGrpcUri);
  initInfologger(iCtx, {}, "bkqsink/", "");

  try { // registering state machine callbacks
    iCtx.services().get<framework::CallbackService>().set<framework::CallbackService::Id::Start>([this, services = iCtx.services()]() mutable { start(services); });
  } catch (o2::framework::RuntimeErrorRef& ref) {
    ILOG(Error) << "Error during initialization: " << o2::framework::error_from_ref(ref).what << ENDM;
  }

  ILOG(Info, Devel) << "Initialized BookkeepingQualitySink" << ENDM;
}

void BookkeepingQualitySink::start(framework::ServiceRegistryRef services)
{
  Activity fallback; // no proper fallback as we don't have the config in this device
  auto currentActivity = computeActivity(services, fallback);
  QcInfoLogger::setRun(currentActivity.mId);
}

void BookkeepingQualitySink::send(const std::string& grpcUri, const BookkeepingQualitySink::FlagsMap& flags, Provenance provenance)
{
  auto& bkpClient = o2::quality_control::core::Bookkeeping::getInstance();

  std::optional<int> runNumber;
  std::optional<std::string> passName;
  std::optional<std::string> periodName;

  for (auto& [detector, qoMap] : flags) {
    ILOG(Info, Support) << "Processing flags for detector: " << detector << ENDM;

    std::vector<QcFlag> bkpQcFlags{};
    for (auto& [qoName, converter] : qoMap) {
      if (converter == nullptr) {
        continue;
      }
      if (provenance == Provenance::AsyncQC || provenance == Provenance::MCQC) {
        auto runDuration = ccdb::BasicCCDBManager::instance().getRunDuration(converter->getRunNumber(), false);
        converter->updateValidityInterval({ static_cast<uint64_t>(runDuration.first), static_cast<uint64_t>(runDuration.second) });
      }

      auto flagCollection = converter->getResult();
      if (flagCollection == nullptr) {
        continue;
      }
      if (!runNumber.has_value()) {
        runNumber = flagCollection->getRunNumber();
      }
      if (!passName.has_value()) {
        passName = flagCollection->getPassName();
      }
      if (!periodName.has_value()) {
        periodName = flagCollection->getPeriodName();
      }

      for (const auto& flag : *flagCollection) {
        // BKP uses start/end of run for missing time values, so we are using this functionality in order to avoid
        // determining these values by ourselves (see TaskRunner::start() for details). mtichak checked with mboulais that
        // it is okay to do so.
        bkpQcFlags.emplace_back(QcFlag{
          .flagTypeId = flag.getFlag().getID(),
          .from = flag.getStart() == gFullValidityInterval.getMin() ? std::nullopt : std::optional<uint64_t>{ flag.getStart() },
          .to = flag.getEnd() == gFullValidityInterval.getMax() ? std::nullopt : std::optional<uint64_t>{ flag.getEnd() },
          .origin = flag.getSource(),
          .comment = flag.getComment() });
      }
    }

    if (bkpQcFlags.empty()) {
      ILOG(Info, Support) << "No flags for detector '" << detector << "', skipping" << ENDM;
      continue;
    }
    try {
      switch (provenance) {
        case Provenance::SyncQC:
          bkpClient.sendFlagsForSynchronous(runNumber.value(), detector, bkpQcFlags);
          break;
        case Provenance::AsyncQC:
          bkpClient.sendFlagsForDataPass(runNumber.value(), passName.value(), detector, bkpQcFlags);
          break;
        case Provenance::MCQC:
          bkpClient.sendFlagsForSimulationPass(runNumber.value(), periodName.value(), detector, bkpQcFlags);
          break;
      }
      ILOG(Info, Support) << "Sent " << bkpQcFlags.size() << " flags for detector '" << detector << "'" << ENDM;
    } catch (const std::runtime_error& err) {
      ILOG(Error, Support) << "Encountered errors while sending flags for detector '" << detector << "', details: " << err.what() << ENDM;
    }
  }
}

BookkeepingQualitySink::BookkeepingQualitySink(const std::string& grpcUri, Provenance provenance, SendCallback sendCallback)
  : mGrpcUri{ grpcUri }, mProvenance{ provenance }, mSendCallback{ std::move(sendCallback) } {}

auto collectionForQualityObject(const QualityObject& qualityObject) -> std::unique_ptr<QualityControlFlagCollection>
{
  return std::make_unique<QualityControlFlagCollection>(
    qualityObject.getName(),
    qualityObject.getDetectorName(),
    gFullValidityInterval,
    qualityObject.getActivity().mId,
    qualityObject.getActivity().mPeriodName,
    qualityObject.getActivity().mPassName,
    qualityObject.getActivity().mProvenance);
}

void BookkeepingQualitySink::run(framework::ProcessingContext& context)
{
  for (auto const& ref : framework::InputRecordWalker(context.inputs())) {
    std::unique_ptr<QualityObject> qualityObject;
    try {
      qualityObject = framework::DataRefUtils::as<QualityObject>(ref);
    } catch (...) {
      ILOG(Warning, Support) << "Unexpected message received, QualityObject expected" << ENDM;
      continue;
    }
    auto& converter = mFlagsMap[qualityObject->getDetectorName()][qualityObject->getName()];
    if (converter == nullptr) {
      converter = std::make_unique<QualitiesToFlagCollectionConverter>(collectionForQualityObject(*qualityObject), qualityObject->getPath());
    }
    (*converter)(*qualityObject);
  }
}

void BookkeepingQualitySink::endOfStream(framework::EndOfStreamContext&)
{
  sendAndClear();
}

void BookkeepingQualitySink::stop()
{
  sendAndClear();
}

void BookkeepingQualitySink::sendAndClear()
{
  if (!mFlagsMap.empty()) {
    mSendCallback(mGrpcUri, mFlagsMap, mProvenance);
  }
  mFlagsMap.clear();
}

} // namespace o2::quality_control::core
