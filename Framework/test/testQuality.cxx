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
/// \file   testQuality.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include <catch_amalgamated.hpp>
#include <DataFormatsQualityControl/FlagTypeFactory.h>

using namespace std;

namespace o2::quality_control::core
{

TEST_CASE("quality_test")
{
  Quality asdf(123, "asdf");
  Quality myQuality = Quality::Bad;
  CHECK(myQuality.getLevel() == 3);
  CHECK(myQuality.getName() == "Bad");
  myQuality = Quality::Good;
  CHECK(myQuality.getLevel() == 1);
  CHECK(myQuality.getName() == "Good");
  myQuality = Quality::Medium;
  CHECK(myQuality.getLevel() == 2);
  CHECK(myQuality.getName() == "Medium");
  myQuality = Quality::Null;
  CHECK(myQuality.getLevel() == Quality::NullLevel);
  CHECK(myQuality.getName() == "Null");

  ILOG(Info, Support) << "test quality output : " << myQuality << ENDM;

  CHECK(Quality::Bad.isWorseThan(Quality::Medium));
  CHECK(Quality::Bad.isWorseThan(Quality::Good));
  CHECK(!Quality::Bad.isWorseThan(Quality::Null));
  CHECK(!Quality::Bad.isWorseThan(Quality::Bad));

  CHECK(Quality::Good.isBetterThan(Quality::Medium));
  CHECK(Quality::Good.isBetterThan(Quality::Bad));
  CHECK(Quality::Good.isBetterThan(Quality::Null));
  CHECK(!Quality::Good.isBetterThan(Quality::Good));
}

TEST_CASE("quality_flags")
{
  Quality myQuality = Quality::Bad;
  myQuality.addFlag(FlagTypeFactory::BadTracking(), "exception in x");
  myQuality.addFlag(FlagTypeFactory::BadTracking(), "exception in y");
  myQuality.addFlag(FlagTypeFactory::BadPID(), "Bethe and Bloch had a bad day");

  auto myFlags = myQuality.getFlags();
  CHECK(myFlags[0].first == FlagTypeFactory::BadTracking());
  CHECK(myFlags[0].second == "exception in x");
  CHECK(myFlags[1].first == FlagTypeFactory::BadTracking());
  CHECK(myFlags[1].second == "exception in y");
  CHECK(myFlags[2].first == FlagTypeFactory::BadPID());
  CHECK(myFlags[2].second == "Bethe and Bloch had a bad day");

  auto copyQuality = myQuality;
  auto copyFlags = copyQuality.getFlags();
  CHECK(copyFlags[0].first == FlagTypeFactory::BadTracking());
  CHECK(copyFlags[0].second == "exception in x");
  CHECK(copyFlags[1].first == FlagTypeFactory::BadTracking());
  CHECK(copyFlags[1].second == "exception in y");
  CHECK(copyFlags[2].first == FlagTypeFactory::BadPID());
  CHECK(copyFlags[2].second == "Bethe and Bloch had a bad day");
}

} // namespace o2::quality_control::core
