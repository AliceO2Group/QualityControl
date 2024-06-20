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
/// @file     KafkaConsumer.h
/// @author   Michal Tichak

#ifndef QC_CORE_KAFKA_CONSUMER_H
#define QC_CORE_KAFKA_CONSUMER_H

#include <kafka/KafkaConsumer.h>
#include <atomic>

namespace o2::quality_control::core
{

class KafkaConsumer
{
 public:
  /*
   *
   */
  explicit KafkaConsumer(const std::string& brokers);

  void consume(const std::string& topic);
  void stop() noexcept;

 private:
  kafka::clients::consumer::KafkaConsumer mConsumer;
  std::atomic_bool mRunning;
};

} // namespace o2::quality_control::core

#endif
