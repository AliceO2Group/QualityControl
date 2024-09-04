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
/// \file    testQualitiesToFlagCollectionConverter.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/QualitiesToFlagCollectionConverter.h"
#include "QualityControl/QualityObject.h"
#include <DataFormatsQualityControl/QualityControlFlagCollection.h>
#include <DataFormatsQualityControl/FlagTypeFactory.h>
#include <ranges>
#include <catch_amalgamated.hpp>

using namespace o2::quality_control;
using namespace o2::quality_control::core;

TEST_CASE("Default QO conversions", "[QualitiesToFlagCollectionConverter]")
{
  SECTION("Good QO with no Flags is not converted to any Flag")
  {
    std::unique_ptr<QualityControlFlagCollection> qcfc{ new QualityControlFlagCollection("test1", "DET", { 5, 100 }) };
    QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");

    QualityObject qo{ Quality::Good, "xyzCheck", "DET" };
    qo.setValidity({ 5, 100 });
    converter(qo);

    qcfc = converter.getResult();

    REQUIRE(qcfc->size() == 0);
  }
  SECTION("Bad and Medium QOs with no Flags are converted to Unknown Flag")
  {
    std::vector<QualityObject> qos{
      { Quality::Medium, "xyzCheck", "DET" },
      { Quality::Bad, "xyzCheck", "DET" }
    };

    qos[0].setValidity({ 5, 150 });
    qos[1].setValidity({ 10, 100 });

    std::unique_ptr<QualityControlFlagCollection> qcfc{ new QualityControlFlagCollection("test1", "DET", { 5, 100 }) };
    QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");
    for (const auto& qo : qos) {
      converter(qo);
    }
    qcfc = converter.getResult();

    REQUIRE(qcfc->size() == 2);

    auto& flag1 = *qcfc->begin();
    CHECK(flag1.getStart() == 5);
    CHECK(flag1.getEnd() == 100);
    CHECK(flag1.getFlag() == FlagTypeFactory::Unknown());
    CHECK(flag1.getSource() == "qc/DET/QO/xyzCheck");

    auto& flag2 = *(++qcfc->begin());
    CHECK(flag2.getStart() == 10);
    CHECK(flag2.getEnd() == 100);
    CHECK(flag2.getFlag() == FlagTypeFactory::Unknown());
    CHECK(flag2.getSource() == "qc/DET/QO/xyzCheck");
  }

  SECTION("Null QO with no Flags is converted to UnknownQuality Flag")
  {
    std::unique_ptr<QualityControlFlagCollection> qcfc{ new QualityControlFlagCollection("test1", "DET", { 5, 100 }) };
    QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");

    QualityObject qo{ Quality::Null, "xyzCheck", "DET" };
    qo.setValidity({ 5, 100 });
    converter(qo);

    qcfc = converter.getResult();

    REQUIRE(qcfc->size() == 1);
    auto& flag = *qcfc->begin();
    CHECK(flag.getStart() == 5);
    CHECK(flag.getEnd() == 100);
    CHECK(flag.getFlag() == FlagTypeFactory::UnknownQuality());
    CHECK(flag.getSource() == "qc/DET/QO/xyzCheck");
  }
}

TEST_CASE("Filling empty intervals with UnknownQuality", "[QualitiesToFlagCollectionConverter]")
{
  SECTION("test_NoQOs")
  {
    std::unique_ptr<QualityControlFlagCollection> qcfc{ new QualityControlFlagCollection("test1", "DET", { 5, 100 }) };
    QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");
    qcfc = converter.getResult();

    REQUIRE(qcfc->size() == 1);
    auto& flag = *qcfc->begin();
    CHECK(flag.getStart() == 5);
    CHECK(flag.getEnd() == 100);
    CHECK(flag.getFlag() == FlagTypeFactory::UnknownQuality());
    CHECK(flag.getSource() == "qc/DET/QO/xyzCheck");
  }

  SECTION("test_NoBeginning")
  {
    std::vector<QualityObject> qos{
      { Quality::Bad, "xyzCheck", "DET" },
      { Quality::Good, "xyzCheck", "DET" }
    };

    qos[0].setValidity({ 10, 50 });
    qos[1].setValidity({ 50, 120 });

    std::unique_ptr<QualityControlFlagCollection> qcfc{ new QualityControlFlagCollection("test1", "DET", { 5, 100 }) };
    QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");
    for (const auto& qo : qos) {
      converter(qo);
    }
    qcfc = converter.getResult();

    REQUIRE(qcfc->size() == 2);

    auto& flag1 = *qcfc->begin();
    CHECK(flag1.getStart() == 5);
    CHECK(flag1.getEnd() == 10);
    CHECK(flag1.getFlag() == FlagTypeFactory::UnknownQuality());
    CHECK(flag1.getSource() == "qc/DET/QO/xyzCheck");

    auto& flag2 = *(++qcfc->begin());
    CHECK(flag2.getStart() == 10);
    CHECK(flag2.getEnd() == 50);
    CHECK(flag2.getFlag() == FlagTypeFactory::Unknown());
    CHECK(flag2.getSource() == "qc/DET/QO/xyzCheck");
  }

  SECTION("test_NoEnd")
  {
    std::vector<QualityObject> qos{
      { Quality::Good, "xyzCheck", "DET" }
    };

    qos[0].setValidity({ 5, 80 });

    std::unique_ptr<QualityControlFlagCollection> qcfc{ new QualityControlFlagCollection("test1", "DET", { 5, 100 }) };
    QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");
    for (const auto& qo : qos) {
      converter(qo);
    }
    qcfc = converter.getResult();

    REQUIRE(qcfc->size() == 1);
    auto& flag = *qcfc->begin();
    CHECK(flag.getStart() == 80);
    CHECK(flag.getEnd() == 100);
    CHECK(flag.getFlag() == FlagTypeFactory::UnknownQuality());
    CHECK(flag.getSource() == "qc/DET/QO/xyzCheck");
  }
}

TEST_CASE("UnknownQuality is overwritten by other Flags", "[QualitiesToFlagCollectionConverter]")
{
  // Automatically generated UnknownQuality is overwritten by anything
  // User-provided UnknownQuality and default Flag for Null are overwritten by other user-provided flags which are not UnknownQuality

  std::vector<QualityObject> qos{
    { Quality::Null, "xyzCheck", "DET" }, // Null with default UnknownQuality Flag, to be trimmed
    { Quality::Null, "xyzCheck", "DET" }, // Null with default UnknownQuality Flag, to be removed
    { Quality::Null, "xyzCheck", "DET" }, // Null with a user-provided UnknownQuality Flag, to be trimmed
    { Quality::Null, "xyzCheck", "DET" }, // Null with a user-provided UnknownQuality Flag, to be removed
    { Quality::Good, "xyzCheck", "DET" }  // "known" Flag which should trim/remove all of the above
  };

  qos[0].setValidity({ 5, 30 });
  qos[1].setValidity({ 40, 50 });
  qos[2].setValidity({ 50, 100 });
  qos[2].addFlag(FlagTypeFactory::UnknownQuality(), "custom comment 1");
  qos[3].setValidity({ 50, 60 });
  qos[3].addFlag(FlagTypeFactory::UnknownQuality(), "custom comment 2");
  qos[4].setValidity({ 20, 60 });

  auto check = [](const QualityControlFlagCollection& qcfc) {
    REQUIRE(qcfc.size() == 2);

    auto it = qcfc.begin();
    auto& flag1 = *it;
    CHECK(flag1.getStart() == 5);
    CHECK(flag1.getEnd() == 20);
    CHECK(flag1.getFlag() == FlagTypeFactory::UnknownQuality());
    CHECK(flag1.getSource() == "qc/DET/QO/xyzCheck");

    auto& flag2 = *(++it);
    CHECK(flag2.getStart() == 60);
    CHECK(flag2.getEnd() == 100);
    CHECK(flag2.getFlag() == FlagTypeFactory::UnknownQuality());
    CHECK(flag2.getComment() == "custom comment 1");
    CHECK(flag2.getSource() == "qc/DET/QO/xyzCheck");
  };

  // we perform the same test twice by feeding the converter in the normal and reverse order
  // to make sure it does not affect the result
  SECTION("flags provided from the first to last in the vector")
  {
    std::unique_ptr<QualityControlFlagCollection> qcfc{ new QualityControlFlagCollection("test1", "DET", { 5, 100 }) };
    QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");
    for (const auto& qo : qos) {
      converter(qo);
    }
    qcfc = converter.getResult();
    check(*qcfc);
  }
  SECTION("flags provided in the reverse order")
  {
    std::unique_ptr<QualityControlFlagCollection> qcfc{ new QualityControlFlagCollection("test1", "DET", { 5, 100 }) };
    QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");
    for (const auto& qo : qos | std::views::reverse) {
      converter(qo);
    }
    qcfc = converter.getResult();
    check(*qcfc);
  }
}

TEST_CASE("All QOs with Flags are converted to Flags, while Quality is ignored", "[QualitiesToFlagCollectionConverter]")
{
  std::vector<QualityObject> qos{
    { Quality::Null, "xyzCheck", "DET" },
    { Quality::Bad, "xyzCheck", "DET" },
    { Quality::Medium, "xyzCheck", "DET" },
    { Quality::Good, "xyzCheck", "DET" },
  };

  qos[0].setValidity({ 5, 20 });
  qos[0].addFlag(FlagTypeFactory::Good(), "null");

  qos[1].setValidity({ 20, 40 });
  qos[1].addFlag(FlagTypeFactory::Good(), "bad");

  qos[2].setValidity({ 40, 60 });
  qos[2].addFlag(FlagTypeFactory::Good(), "medium");

  qos[3].setValidity({ 60, 100 });
  qos[3].addFlag(FlagTypeFactory::Unknown(), "good");

  std::unique_ptr<QualityControlFlagCollection> qcfc{
    new QualityControlFlagCollection("test1", "DET", { 5, 100 })
  };
  QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");
  for (const auto& qo : qos) {
    converter(qo);
  }
  qcfc = converter.getResult();

  REQUIRE(qcfc->size() == 4);

  auto it = qcfc->begin();
  auto& flag1 = *it;
  CHECK(flag1.getStart() == 5);
  CHECK(flag1.getEnd() == 20);
  CHECK(flag1.getFlag() == FlagTypeFactory::Good());
  CHECK(flag1.getComment() == "null");
  CHECK(flag1.getSource() == "qc/DET/QO/xyzCheck");

  auto& flag2 = *(++it);
  CHECK(flag2.getStart() == 20);
  CHECK(flag2.getEnd() == 40);
  CHECK(flag2.getFlag() == FlagTypeFactory::Good());
  CHECK(flag2.getComment() == "bad");
  CHECK(flag2.getSource() == "qc/DET/QO/xyzCheck");

  auto& flag3 = *(++it);
  CHECK(flag3.getStart() == 40);
  CHECK(flag3.getEnd() == 60);
  CHECK(flag3.getFlag() == FlagTypeFactory::Good());
  CHECK(flag3.getComment() == "medium");
  CHECK(flag3.getSource() == "qc/DET/QO/xyzCheck");

  auto& flag4 = *(++it);
  CHECK(flag4.getStart() == 60);
  CHECK(flag4.getEnd() == 100);
  CHECK(flag4.getFlag() == FlagTypeFactory::Unknown());
  CHECK(flag4.getComment() == "good");
  CHECK(flag4.getSource() == "qc/DET/QO/xyzCheck");
}

TEST_CASE("Input parameter validation", "[QualitiesToFlagCollectionConverter]")
{
  std::vector<QualityObject> qos{
    { Quality::Bad, "xyzCheck", "TPC" },
    { Quality::Bad, "xyzCheck", "DET" },
    { Quality::Bad, "xyzCheck", "DET" }
  };

  qos[0].setValidity({ 10, 120 });
  qos[1].setValidity({ 1000, 10000 });
  qos[2].setValidity({ 40, 30 });

  SECTION("Different detector")
  {
    // different detector
    std::unique_ptr<QualityControlFlagCollection> qcfc{
      new QualityControlFlagCollection("test1", "DET", { 5, 100 })
    };
    QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");
    CHECK_THROWS_AS(converter(qos[0]), std::runtime_error);
  }
  SECTION("QO start after the QCFC end limit")
  {
    std::unique_ptr<QualityControlFlagCollection> qcfc{
      new QualityControlFlagCollection("test1", "DET", { 5, 100 })
    };
    QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");
    converter(qos[1]);

    qcfc = converter.getResult();
    REQUIRE(qcfc->size() == 1);
    auto& flag = *qcfc->begin();
    CHECK(flag.getStart() == 5);
    CHECK(flag.getEnd() == 100);
    CHECK(flag.getFlag() == FlagTypeFactory::UnknownQuality());
    CHECK(flag.getSource() == "qc/DET/QO/xyzCheck");
  }
  SECTION("QO validity starts after it finishes")
  {
    std::unique_ptr<QualityControlFlagCollection> qcfc{
      new QualityControlFlagCollection("test1", "DET", { 5, 100 })
    };
    QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");

    qcfc = converter.getResult();
    REQUIRE(qcfc->size() == 1);
    auto& flag = *qcfc->begin();
    CHECK(flag.getStart() == 5);
    CHECK(flag.getEnd() == 100);
    CHECK(flag.getFlag() == FlagTypeFactory::UnknownQuality());
    CHECK(flag.getSource() == "qc/DET/QO/xyzCheck");
  }
}

TEST_CASE("Merging Flags", "[QualitiesToFlagCollectionConverter]")
{
  SECTION("test_OverlappingQOs")
  {
    std::vector<QualityObject> qos{
      { Quality::Good, "xyzCheck", "DET" },
      { Quality::Good, "xyzCheck", "DET" },
      { Quality::Good, "xyzCheck", "DET" },
      { Quality::Bad, "xyzCheck", "DET" },
      { Quality::Bad, "xyzCheck", "DET" },
      { Quality::Bad, "xyzCheck", "DET" }
    };

    qos[0].setValidity({ 5, 50 });
    qos[1].setValidity({ 10, 50 });
    qos[2].setValidity({ 15, 60 });
    qos[3].setValidity({ 55, 120 });
    qos[4].setValidity({ 60, 120 });
    qos[5].setValidity({ 70, 120 });

    std::unique_ptr<QualityControlFlagCollection> qcfc{ new QualityControlFlagCollection("test1", "DET", { 5, 100 }) };
    QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");
    for (const auto& qo : qos) {
      converter(qo);
    }
    qcfc = converter.getResult();

    REQUIRE(qcfc->size() == 1);

    auto& flag1 = *qcfc->begin();
    CHECK(flag1.getStart() == 55);
    CHECK(flag1.getEnd() == 100);
    CHECK(flag1.getFlag() == FlagTypeFactory::Unknown());
    CHECK(flag1.getSource() == "qc/DET/QO/xyzCheck");
  }

  SECTION("test_AdjacentQOs")
  {
    std::vector<QualityObject> qos{
      { Quality::Good, "xyzCheck", "DET" },
      { Quality::Good, "xyzCheck", "DET" },
      { Quality::Bad, "xyzCheck", "DET" },
      { Quality::Bad, "xyzCheck", "DET" }
    };

    qos[0].setValidity({ 5, 10 });
    qos[1].setValidity({ 10, 50 });
    qos[2].setValidity({ 50, 80 });
    qos[3].setValidity({ 80, 120 });

    std::unique_ptr<QualityControlFlagCollection> qcfc{ new QualityControlFlagCollection("test1", "DET", { 5, 100 }) };
    QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");

    for (const auto& qo : qos) {
      converter(qo);
    }
    qcfc = converter.getResult();

    REQUIRE(qcfc->size() == 1);

    auto& flag1 = *qcfc->begin();
    CHECK(flag1.getStart() == 50);
    CHECK(flag1.getEnd() == 100);
    CHECK(flag1.getFlag() == FlagTypeFactory::Unknown());
    CHECK(flag1.getSource() == "qc/DET/QO/xyzCheck");
  }

  SECTION("test_NonDefaultFlags")
  {
    std::vector<QualityObject> qos{
      { Quality::Good, "xyzCheck", "DET" },
      { Quality::Bad, "xyzCheck", "DET" },
      { Quality::Bad, "xyzCheck", "DET" },
      { Quality::Bad, "xyzCheck", "DET" }
    };

    qos[0].setValidity({ 5, 10 });
    qos[1].setValidity({ 10, 40 });
    qos[2].setValidity({ 30, 80 });
    qos[3].setValidity({ 50, 100 });

    qos[1].addFlag(FlagTypeFactory::BadTracking(), "Bug in reco");
    qos[2].addFlag(FlagTypeFactory::BadTracking(), "Bug in reco");
    qos[2].addFlag(FlagTypeFactory::BadHadronPID(), "evil CERN scientists changed the proton mass");
    qos[3].addFlag(FlagTypeFactory::BadTracking(), "Bug in reco");

    std::unique_ptr<QualityControlFlagCollection> qcfc{ new QualityControlFlagCollection("test1", "DET", { 5, 100 }) };
    QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");
    for (const auto& qo : qos) {
      converter(qo);
    }
    qcfc = converter.getResult();

    REQUIRE(qcfc->size() == 2);

    auto& flag1 = *qcfc->begin();
    CHECK(flag1.getStart() == 10);
    CHECK(flag1.getEnd() == 100);
    CHECK(flag1.getFlag() == FlagTypeFactory::BadTracking());
    CHECK(flag1.getComment() == "Bug in reco");
    CHECK(flag1.getSource() == "qc/DET/QO/xyzCheck");

    auto& flag2 = *(++qcfc->begin());
    CHECK(flag2.getStart() == 30);
    CHECK(flag2.getEnd() == 80);
    CHECK(flag2.getFlag() == FlagTypeFactory::BadHadronPID());
    CHECK(flag2.getComment() == "evil CERN scientists changed the proton mass");
    CHECK(flag2.getSource() == "qc/DET/QO/xyzCheck");
  }

  SECTION("test_TheSameFlagsButSeparated")
  {
    std::vector<QualityObject> qos{
      { Quality::Bad, "xyzCheck", "DET" },
      { Quality::Good, "xyzCheck", "DET" },
      { Quality::Bad, "xyzCheck", "DET" },
      { Quality::Bad, "xyzCheck", "DET" }
    };

    qos[0].setValidity({ 5, 25 });
    qos[1].setValidity({ 10, 40 });
    qos[2].setValidity({ 30, 50 });
    qos[3].setValidity({ 80, 100 });

    qos[0].addFlag(FlagTypeFactory::BadTracking(), "Bug in reco");

    qos[2].addFlag(FlagTypeFactory::BadTracking(), "Bug in reco");
    qos[3].addFlag(FlagTypeFactory::BadTracking(), "Bug in reco");

    std::unique_ptr<QualityControlFlagCollection> qcfc{ new QualityControlFlagCollection("test1", "DET", { 5, 100 }) };
    QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");

    for (const auto& qo : qos) {
      converter(qo);
    }
    qcfc = converter.getResult();

    REQUIRE(qcfc->size() == 4);

    auto it = qcfc->begin();
    auto& flag1 = *it;
    CHECK(flag1.getStart() == 5);
    CHECK(flag1.getEnd() == 25);
    CHECK(flag1.getFlag() == FlagTypeFactory::BadTracking());
    CHECK(flag1.getComment() == "Bug in reco");
    CHECK(flag1.getSource() == "qc/DET/QO/xyzCheck");

    auto& flag2 = *(++it);
    CHECK(flag2.getStart() == 30);
    CHECK(flag2.getEnd() == 50);
    CHECK(flag2.getFlag() == FlagTypeFactory::BadTracking());
    CHECK(flag2.getComment() == "Bug in reco");
    CHECK(flag2.getSource() == "qc/DET/QO/xyzCheck");

    auto& flag3 = *(++it);
    CHECK(flag3.getStart() == 50);
    CHECK(flag3.getEnd() == 80);
    CHECK(flag3.getFlag() == FlagTypeFactory::UnknownQuality());
    CHECK(flag3.getSource() == "qc/DET/QO/xyzCheck");

    auto& flag4 = *(++it);
    CHECK(flag4.getStart() == 80);
    CHECK(flag4.getEnd() == 100);
    CHECK(flag4.getFlag() == FlagTypeFactory::BadTracking());
    CHECK(flag4.getComment() == "Bug in reco");
    CHECK(flag4.getSource() == "qc/DET/QO/xyzCheck");
  }
}

TEST_CASE("Trimming the validity interval", "[QualitiesToFlagCollectionConverter]")
{
  std::vector<QualityObject> qos{
    { Quality::Bad, "xyzCheck", "DET" },
    { Quality::Good, "xyzCheck", "DET" },
    { Quality::Bad, "xyzCheck", "DET" }
  };

  qos[0].setValidity({ 5, 100 });
  qos[1].setValidity({ 50, 100 });
  qos[1].addFlag(FlagTypeFactory::Good(), "hello");
  qos[2].setValidity({ 30, 70 });
  qos[2].addFlag(FlagTypeFactory::BadTracking(), "comment");

  std::unique_ptr<QualityControlFlagCollection> qcfc{ new QualityControlFlagCollection("test1", "DET", { 5, 100 }) };
  QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");
  converter(qos[0]);
  converter(qos[1]);
  converter.updateValidityInterval({ 10, 40 });
  converter(qos[2]);
  qcfc = converter.getResult();

  REQUIRE(qcfc->size() == 2);

  auto& flag1 = *qcfc->begin();
  CHECK(flag1.getStart() == 10);
  CHECK(flag1.getEnd() == 40);
  CHECK(flag1.getFlag() == FlagTypeFactory::Unknown());
  CHECK(flag1.getSource() == "qc/DET/QO/xyzCheck");

  auto& flag2 = *(++qcfc->begin());
  CHECK(flag2.getStart() == 30);
  CHECK(flag2.getEnd() == 40);
  CHECK(flag2.getFlag() == FlagTypeFactory::BadTracking());
  CHECK(flag2.getComment() == "comment");
}

TEST_CASE("Extending the validity interval", "[QualitiesToFlagCollectionConverter]")
{
  std::vector<QualityObject> qos{
    { Quality::Bad, "xyzCheck", "DET" },
    { Quality::Good, "xyzCheck", "DET" }
  };

  qos[0].setValidity({ 5, 100 });
  qos[1].setValidity({ 50, 100 });
  qos[1].addFlag(FlagTypeFactory::Good(), "hello");

  std::unique_ptr<QualityControlFlagCollection> qcfc{ new QualityControlFlagCollection("test1", "DET", { 5, 100 }) };
  QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");
  for (const auto& qo : qos) {
    converter(qo);
  }
  converter.updateValidityInterval({ 1, 120 });
  qcfc = converter.getResult();

  CHECK(qcfc->size() == 4);

  auto it = qcfc->begin();
  auto& flag1 = *it;
  CHECK(flag1.getStart() == 1);
  CHECK(flag1.getEnd() == 5);
  CHECK(flag1.getFlag() == FlagTypeFactory::UnknownQuality());
  CHECK(flag1.getSource() == "qc/DET/QO/xyzCheck");

  auto& flag2 = *(++it);
  CHECK(flag2.getStart() == 5);
  CHECK(flag2.getEnd() == 100);
  CHECK(flag2.getFlag() == FlagTypeFactory::Unknown());
  CHECK(flag2.getSource() == "qc/DET/QO/xyzCheck");

  auto& flag3 = *(++it);
  CHECK(flag3.getStart() == 50);
  CHECK(flag3.getEnd() == 100);
  CHECK(flag3.getFlag() == FlagTypeFactory::Good());
  CHECK(flag3.getComment() == "hello");
  CHECK(flag3.getSource() == "qc/DET/QO/xyzCheck");

  auto& flag4 = *(++it);
  CHECK(flag4.getStart() == 100);
  CHECK(flag4.getEnd() == 120);
  CHECK(flag4.getFlag() == FlagTypeFactory::UnknownQuality());
  CHECK(flag4.getSource() == "qc/DET/QO/xyzCheck");
}