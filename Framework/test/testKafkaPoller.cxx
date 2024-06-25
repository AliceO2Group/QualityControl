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

TEST_CASE("test_kafka_poller")
{
  using namespace o2::quality_control::core;
  KafkaPoller kafkaPoller("mtichak-flp-1-27.cern.ch:9092");
  kafkaPoller.subscribe("test");

  while (true) {
    std::ranges::for_each(kafkaPoller.poll(), [](const auto& record) {
      std::cout << record.toString() << "\n";
    });
    std::this_thread::sleep_for(std::chrono::milliseconds{ 100 });
  }
}
