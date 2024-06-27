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
/// \file    testKafkaPoller.cxx
/// \author  Michal Tichak
///

#include <catch_amalgamated.hpp>
#include <chrono>
#include <thread>
#include <QualityControl/KafkaPoller.h>

TEST_CASE("test_kafka_poller", "[.manual]")
{
  using namespace o2::quality_control::core;
  KafkaPoller kafkaPoller("mtichak-flp-1-27.cern.ch:9092", "unitTestID");
  kafkaPoller.subscribe("aliecs.run");

  bool receivedSOR = false;
  bool receivedEOR = false;

  // while (runningSOR || runningEOR) {
  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds{ 5 });
    const auto records = kafkaPoller.poll();
    std::cout << "parsing: " << records.size() << "\n";

    for (const auto& record : records) {
      const auto event = proto::recordToEvent(record.value());
      if (proto::start_of_run::check(event.value(), "", 0)) {
        std::cout << "receivedSOR!\n";
        receivedSOR = true;
      }
      if (proto::end_of_run::check(event.value(), "", 0)) {
        std::cout << "receivedEOR!\n";
        receivedEOR = true;
      }
    }

    if (receivedSOR && receivedEOR) {
      break;
    }
  }
}
