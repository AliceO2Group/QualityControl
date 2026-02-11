// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
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
/// \file    testUserInputOutput.cxx
/// \author  Piotr Konopka
///

#include <catch_amalgamated.hpp>

#include <Headers/DataHeader.h>
#include <Framework/DataSpecUtils.h>

#include "QualityControl/UserInputOutput.h"

using namespace o2::header;

namespace o2::quality_control::core
{

TEST_CASE("ConcreteDataMatcher")
{
  auto dataMatcher = createUserDataMatcher(DataSourceType::Task, "TST", "mytask");
  CHECK(dataMatcher.origin == DataOrigin{ "QTST" });
  CHECK(dataMatcher.description == DataDescription{ "mytask" });
  CHECK(dataMatcher.subSpec == 0);
}

TEST_CASE("InputSpec")
{
  auto inputSpec = createUserInputSpec(DataSourceType::Task, "TST", "mytask");
  CHECK(inputSpec.binding == "mytask");
  CHECK(inputSpec.lifetime == framework::Lifetime::Sporadic);
  CHECK(framework::DataSpecUtils::match(inputSpec, framework::ConcreteDataMatcher{ DataOrigin{ "QTST" }, DataDescription{ "mytask" }, 0 }));
}

TEST_CASE("OutputSpec")
{
  auto outputSpec = createUserOutputSpec(DataSourceType::Task, "TST", "mytask");
  CHECK(outputSpec.binding.value == "mytask");
  CHECK(outputSpec.lifetime == framework::Lifetime::Sporadic);
  CHECK(framework::DataSpecUtils::match(outputSpec, framework::ConcreteDataMatcher{ DataOrigin{ "QTST" }, DataDescription{ "mytask" }, 0 }));
}

} // namespace o2::quality_control::core
