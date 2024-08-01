// Copyright 2019-2024 CERN and copyright holders of ALICE O2.
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
/// @file     KafkaPoller.h
/// @author   Michal Tichak

#ifndef QC_CORE_KAFKA_CONSUMER_H
#define QC_CORE_KAFKA_CONSUMER_H

#include <kafka/KafkaConsumer.h>
#include <proto/events.pb.h>
#include "Activity.h"

namespace o2::quality_control::core
{

namespace proto
{

auto recordToEvent(const kafka::Value&) -> std::optional<events::Event>;

namespace start_of_run
{
void fillActivity(const events::Event& event, Activity& activity);
bool isValid(const events::Event& event, const std::string& environmentID = "", int runNumber = 0);
} // namespace start_of_run

namespace end_of_run
{
void fillActivity(const events::Event& event, Activity& activity);
bool isValid(const events::Event& event, const std::string& environmentID = "", int runNumber = 0);
} // namespace end_of_run

} // namespace proto

class KafkaPoller
{
 public:
  using KafkaRecords = std::vector<kafka::clients::consumer::ConsumerRecord>;

  explicit KafkaPoller(const std::string& brokers, const std::string& groupId);

  void subscribe(const std::string& topic, size_t numberOfRetries = 5);
  // timeout is used to wait if there are not messages.
  auto poll(std::chrono::milliseconds timeout = std::chrono::milliseconds{ 10 }) -> KafkaRecords;

 private:
  kafka::clients::consumer::KafkaConsumer mConsumer;
};

} // namespace o2::quality_control::core

#endif
