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
#include "QualityControl/Bookkeeping.h"
#include "QualityControl/QualitiesToFlagCollectionConverter.h"
#include "QualityControl/QualityObject.h"
#include "QualityControl/QcInfoLogger.h"
#include <BookkeepingApi/QcFlagServiceClient.h>
#include <BookkeepingApi/BkpClientFactory.h>
#include <stdexcept>

namespace o2::quality_control::core
{

void BookkeepingQualitySink::send(const std::string& grpcUri, const BookkeepingQualitySink::FlagsMap& flags, Provenance type)
{
  auto bkpClient = o2::bkp::api::BkpClientFactory::create(grpcUri);
  auto& qcClient = bkpClient->qcFlag();

  ILOG(Info) << "Sending " << flags.size() << " flag collections" << ENDM;

  for (const auto& [_, flagCollection] : flags) {

    std::vector<QcFlag> bkpQcFlags{};
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

    ILOG(Info) << "Flag collection has " << bkpQcFlags.size() << " flags" << ENDM;

    if (bkpQcFlags.empty()) {
      continue;
    }

    try {
      switch (type) {
        case Provenance::SyncQc:
          qcClient->createForDataPass(flagCollection->getRunNumber(), flagCollection->getPassName(), flagCollection->getDetector(), bkpQcFlags);
          break;
        case Provenance::AsyncQc:
          // NOTE: There is productionName in Bookkeaping API instead of passName, but I don't seen anythin like production around here.
          qcClient->createForSimulationPass(flagCollection->getRunNumber(), flagCollection->getPassName(), flagCollection->getDetector(), bkpQcFlags);
          break;
      }
    } catch (const std::runtime_error& err) {
      ILOG(Error) << "Failed to send flags for detector: " << flagCollection->getDetector()
                  << " and pass: " << flagCollection->getPassName()
                  << " with error: " << err.what() << ENDM;
    }
  }
}

BookkeepingQualitySink::BookkeepingQualitySink(const std::string& grpcUri, Provenance type, SendCallback sendCallback)
  : mGrpcUri{ grpcUri }, mType{ type }, mSendCallback{ sendCallback } {}

auto merge(std::unique_ptr<QualityControlFlagCollection>&& collection, const std::unique_ptr<QualityObject>& qualityObject) -> std::unique_ptr<QualityControlFlagCollection>
{
  QualitiesToFlagCollectionConverter converter(std::move(collection), qualityObject->getPath());
  converter(*qualityObject);
  return converter.getResult();
}

auto collectionFromQualityObject(const QualityObject& qualityObject) -> std::unique_ptr<QualityControlFlagCollection>
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

auto to_key(const QualityObject& qualityObject) -> std::string
{
  std::string key;
  key.reserve(qualityObject.getDetectorName().size() + std::size("||") + qualityObject.getCheckName().size());
  key.append(qualityObject.getDetectorName());
  key.append("||");
  key.append(qualityObject.getCheckName());
  return key;
}

template <typename t>
struct tt;

void BookkeepingQualitySink::run(framework::ProcessingContext& context)
{
  for (auto const& ref : framework::InputRecordWalker(context.inputs())) {
    try {
      // mQualityObjects.push_back(std::move(framework::DataRefUtils::as<QualityObject>(ref)));
      auto qualityObject = framework::DataRefUtils::as<QualityObject>(ref);
      auto [emplacedIt, _] = mQualityObjectsMap.emplace(to_key(*qualityObject), collectionFromQualityObject(*qualityObject));
      emplacedIt->second = merge(std::move(emplacedIt->second), qualityObject);
    } catch (...) {
      ILOG(Warning, Support) << "Unexpected message received, QualityObject expected" << ENDM;
    }
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
  mSendCallback(mGrpcUri, mQualityObjectsMap, mType);
  mQualityObjectsMap.clear();
}

} // namespace o2::quality_control::core
