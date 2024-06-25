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

namespace o2::quality_control::core
{

class KafkaPoller
{
 public:
  using KafkaRecords = std::vector<kafka::clients::consumer::ConsumerRecord>;

  explicit KafkaPoller(const std::string& brokers);

  void subscribe(const std::string& topic);
  auto poll() -> KafkaRecords;

 private:
  kafka::clients::consumer::KafkaConsumer mConsumer;
};

} // namespace o2::quality_control::core

#endif
