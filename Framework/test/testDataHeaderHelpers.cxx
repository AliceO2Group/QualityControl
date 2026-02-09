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
/// \file   testDataHeaderHelpers.h
/// \author Piotr Konopka
///

#include <catch_amalgamated.hpp>

#include <Headers/DataHeader.h>

#include "QualityControl/DataHeaderHelpers.h"
#include "QualityControl/DataSourceType.h"

using namespace o2::quality_control::core;
using namespace o2::header;

TEST_CASE("DataOrigin")
{
  CHECK_THROWS(createDataOrigin(DataSourceType::Direct, "TST")); // non-QC data source
  CHECK_THROWS(createDataOrigin(DataSourceType::Task, ""));      // empty detector is wrong

  CHECK(createDataOrigin(DataSourceType::Task, "TST") == DataOrigin{ "QTST" });
  CHECK(createDataOrigin(DataSourceType::TaskMovingWindow, "TST") == DataOrigin{ "WTST" });

  CHECK(createDataOrigin(DataSourceType::Task, "TOO_LONG") == DataOrigin{ "QTOO" });
  CHECK(createDataOrigin(DataSourceType::Task, "X") == DataOrigin{ "QX" });
}

TEST_CASE("DataDescription")
{
  CHECK(createDataDescription("", 10) == DataDescription(""));
  CHECK(createDataDescription("ABC", 10) == DataDescription("ABC"));
  CHECK(createDataDescription("ABCDEABCDEABCDEA", 10) == DataDescription("ABCDEABCDEABCDEA"));
  CHECK(createDataDescription("LOOOOOOOOOOOOOOONG", 4) != DataDescription("LOOOOOOOOOOOOOON"));

  CHECK_THROWS(createDataDescription("LOOOOOOOOOOOOOOONG", DataDescription::size + 50));
}