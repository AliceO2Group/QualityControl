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
/// \file    testKafkaConsumer.cxx
/// \author  Michal Tichak
///

#include <catch_amalgamated.hpp>
#include <QualityControl/KafkaConsumer.h>

TEST_CASE("test_kafka_consumer")
{
  o2::quality_control::core::KafkaConsumer consumer("mtichak-flp-1-27.cern.ch:9092");

  consumer.consume("test");
}
