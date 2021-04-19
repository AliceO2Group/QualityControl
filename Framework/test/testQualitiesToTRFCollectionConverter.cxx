// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file    testQualitiesToTRFCollectionConverter.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/QualitiesToTRFCollectionConverter.h"
#include "QualityControl/QualityObject.h"
#include <DataFormatsQualityControl/TimeRangeFlagCollection.h>

#define BOOST_TEST_MODULE QO2TRFCConversion test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

using namespace o2::quality_control;
using namespace o2::quality_control::core;

BOOST_AUTO_TEST_CASE(test_NoQOs)
{
  QualitiesToTRFCollectionConverter converter("test1", "DET", 5, 100, "qc/DET/QO/xyzCheck");
  auto trfc = converter.getResult();

  BOOST_REQUIRE_EQUAL(trfc->size(), 1);
  auto& trf = *trfc->begin();
  BOOST_CHECK_EQUAL(trf.getStart(), 5);
  BOOST_CHECK_EQUAL(trf.getEnd(), 100);
  BOOST_CHECK_EQUAL(trf.getFlag(), FlagReasonFactory::MissingQualityObject());
  BOOST_CHECK_EQUAL(trf.getSource(), "qc/DET/QO/xyzCheck");
}

BOOST_AUTO_TEST_CASE(test_NoBeginning)
{
  std::vector<QualityObject> qos{
    { Quality::Bad, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "10" }, { "Valid-Until", "50" } } },
    { Quality::Good, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "50" }, { "Valid-Until", "120" } } }
  };
  QualitiesToTRFCollectionConverter converter("test1", "DET", 5, 100, "qc/DET/QO/xyzCheck");
  for (const auto& qo : qos) {
    converter(qo);
  }
  auto trfc = converter.getResult();

  BOOST_REQUIRE_EQUAL(trfc->size(), 2);

  auto& trf1 = *trfc->begin();
  BOOST_CHECK_EQUAL(trf1.getStart(), 5);
  BOOST_CHECK_EQUAL(trf1.getEnd(), 9);
  BOOST_CHECK_EQUAL(trf1.getFlag(), FlagReasonFactory::MissingQualityObject());
  BOOST_CHECK_EQUAL(trf1.getSource(), "qc/DET/QO/xyzCheck");

  auto& trf2 = *(++trfc->begin());
  BOOST_CHECK_EQUAL(trf2.getStart(), 10);
  BOOST_CHECK_EQUAL(trf2.getEnd(), 50);
  BOOST_CHECK_EQUAL(trf2.getFlag(), FlagReasonFactory::Unknown());
  BOOST_CHECK_EQUAL(trf2.getSource(), "qc/DET/QO/xyzCheck");
}

BOOST_AUTO_TEST_CASE(test_NoEnd)
{
  std::vector<QualityObject> qos{
    { Quality::Good, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "5" }, { "Valid-Until", "80" } } }
  };
  QualitiesToTRFCollectionConverter converter("test1", "DET", 5, 100, "qc/DET/QO/xyzCheck");
  for (const auto& qo : qos) {
    converter(qo);
  }
  auto trfc = converter.getResult();

  BOOST_REQUIRE_EQUAL(trfc->size(), 1);
  auto& trf = *trfc->begin();
  BOOST_CHECK_EQUAL(trf.getStart(), 80);
  BOOST_CHECK_EQUAL(trf.getEnd(), 100);
  BOOST_CHECK_EQUAL(trf.getFlag(), FlagReasonFactory::MissingQualityObject());
  BOOST_CHECK_EQUAL(trf.getSource(), "qc/DET/QO/xyzCheck");
}

BOOST_AUTO_TEST_CASE(test_WrongOrder)
{
  std::vector<QualityObject> qos{
    { Quality::Good, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "70" }, { "Valid-Until", "90" } } },
    { Quality::Good, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "60" }, { "Valid-Until", "90" } } },
    { Quality::Bad, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "50" }, { "Valid-Until", "120" } } },
    { Quality::Bad, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "40" }, { "Valid-Until", "120" } } },
    { Quality::Good, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "30" }, { "Valid-Until", "120" } } }
  };

  {
    // good to good
    QualitiesToTRFCollectionConverter converter("test1", "DET", 5, 100, "qc/DET/QO/xyzCheck");
    converter(qos[0]);
    BOOST_CHECK_THROW(converter(qos[1]), std::runtime_error);
  }
  {
    // good to bad
    QualitiesToTRFCollectionConverter converter("test1", "DET", 5, 100, "qc/DET/QO/xyzCheck");
    converter(qos[1]);
    BOOST_CHECK_THROW(converter(qos[2]), std::runtime_error);
  }
  {
    // bad to bad
    QualitiesToTRFCollectionConverter converter("test1", "DET", 5, 100, "qc/DET/QO/xyzCheck");
    converter(qos[2]);
    BOOST_CHECK_THROW(converter(qos[3]), std::runtime_error);
  }
  {
    // bad to good
    QualitiesToTRFCollectionConverter converter("test1", "DET", 5, 100, "qc/DET/QO/xyzCheck");
    converter(qos[3]);
    BOOST_CHECK_THROW(converter(qos[4]), std::runtime_error);
  }
}

BOOST_AUTO_TEST_CASE(test_MismatchingParameters)
{
  std::vector<QualityObject> qos{
    { Quality::Bad, "xyzCheck", "TPC", {}, {}, {}, { { "Valid-From", "10" }, { "Valid-Until", "120" } } },
    { Quality::Bad, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "1000" }, { "Valid-Until", "10000" } } },
    { Quality::Bad, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "40" }, { "Valid-Until", "30" } } }
  };

  {
    // different detector
    QualitiesToTRFCollectionConverter converter("test1", "DET", 5, 100, "qc/DET/QO/xyzCheck");
    BOOST_CHECK_THROW(converter(qos[0]), std::runtime_error);
  }
  {
    // QO start after the TRFC end limit
    QualitiesToTRFCollectionConverter converter("test1", "DET", 5, 100, "qc/DET/QO/xyzCheck");
    BOOST_CHECK_THROW(converter(qos[1]), std::runtime_error);
  }
  {
    // QO validity starts after it finishes
    QualitiesToTRFCollectionConverter converter("test1", "DET", 5, 100, "qc/DET/QO/xyzCheck");
    BOOST_CHECK_THROW(converter(qos[2]), std::runtime_error);
  }
}

BOOST_AUTO_TEST_CASE(test_OverlappingQOs)
{
  std::vector<QualityObject> qos{
    { Quality::Good, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "5" }, { "Valid-Until", "50" } } },
    { Quality::Good, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "10" }, { "Valid-Until", "50" } } },
    { Quality::Good, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "15" }, { "Valid-Until", "60" } } },
    { Quality::Bad, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "55" }, { "Valid-Until", "120" } } },
    { Quality::Bad, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "60" }, { "Valid-Until", "120" } } },
    { Quality::Bad, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "70" }, { "Valid-Until", "120" } } }
  };
  QualitiesToTRFCollectionConverter converter("test1", "DET", 5, 100, "qc/DET/QO/xyzCheck");
  for (const auto& qo : qos) {
    converter(qo);
  }
  auto trfc = converter.getResult();

  BOOST_REQUIRE_EQUAL(trfc->size(), 1);

  auto& trf1 = *trfc->begin();
  BOOST_CHECK_EQUAL(trf1.getStart(), 55);
  BOOST_CHECK_EQUAL(trf1.getEnd(), 100);
  BOOST_CHECK_EQUAL(trf1.getFlag(), FlagReasonFactory::Unknown());
  BOOST_CHECK_EQUAL(trf1.getSource(), "qc/DET/QO/xyzCheck");
}

BOOST_AUTO_TEST_CASE(test_AdjacentQOs)
{
  std::vector<QualityObject> qos{
    { Quality::Good, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "5" }, { "Valid-Until", "10" } } },
    { Quality::Good, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "10" }, { "Valid-Until", "14" } } },
    { Quality::Good, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "15" }, { "Valid-Until", "49" } } },
    { Quality::Bad, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "50" }, { "Valid-Until", "80" } } },
    { Quality::Bad, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "80" }, { "Valid-Until", "95" } } },
    { Quality::Bad, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "96" }, { "Valid-Until", "120" } } }
  };
  QualitiesToTRFCollectionConverter converter("test1", "DET", 5, 100, "qc/DET/QO/xyzCheck");
  for (const auto& qo : qos) {
    converter(qo);
  }
  auto trfc = converter.getResult();

  BOOST_REQUIRE_EQUAL(trfc->size(), 1);

  auto& trf1 = *trfc->begin();
  BOOST_CHECK_EQUAL(trf1.getStart(), 50);
  BOOST_CHECK_EQUAL(trf1.getEnd(), 100);
  BOOST_CHECK_EQUAL(trf1.getFlag(), FlagReasonFactory::Unknown());
  BOOST_CHECK_EQUAL(trf1.getSource(), "qc/DET/QO/xyzCheck");
}

BOOST_AUTO_TEST_CASE(test_UnexplainedMediumIsBad)
{
  std::vector<QualityObject> qos{
    { Quality::Medium, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "5" }, { "Valid-Until", "150" } } },
    { Quality::Bad, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "10" }, { "Valid-Until", "100" } } }
  };
  QualitiesToTRFCollectionConverter converter("test1", "DET", 5, 100, "qc/DET/QO/xyzCheck");
  for (const auto& qo : qos) {
    converter(qo);
  }
  auto trfc = converter.getResult();

  BOOST_REQUIRE_EQUAL(trfc->size(), 1);

  auto& trf1 = *trfc->begin();
  BOOST_CHECK_EQUAL(trf1.getStart(), 5);
  BOOST_CHECK_EQUAL(trf1.getEnd(), 100);
  BOOST_CHECK_EQUAL(trf1.getFlag(), FlagReasonFactory::Unknown());
  BOOST_CHECK_EQUAL(trf1.getSource(), "qc/DET/QO/xyzCheck");
}

BOOST_AUTO_TEST_CASE(test_KnownReasons)
{
  std::vector<QualityObject> qos{
    { Quality::Good, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "5" }, { "Valid-Until", "10" } } },
    { Quality::Bad, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "10" }, { "Valid-Until", "40" } } },
    { Quality::Bad, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "30" }, { "Valid-Until", "80" } } },
    { Quality::Bad, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "50" }, { "Valid-Until", "100" } } }
  };
  qos[1].addReason(FlagReasonFactory::ProcessingError(), "Bug in reco");
  qos[2].addReason(FlagReasonFactory::ProcessingError(), "Bug in reco");
  qos[2].addReason(FlagReasonFactory::LimitedAcceptance(), "Sector C was off");
  qos[3].addReason(FlagReasonFactory::ProcessingError(), "Bug in reco");

  QualitiesToTRFCollectionConverter converter("test1", "DET", 5, 100, "qc/DET/QO/xyzCheck");
  for (const auto& qo : qos) {
    converter(qo);
  }
  auto trfc = converter.getResult();

  BOOST_REQUIRE_EQUAL(trfc->size(), 2);

  auto& trf1 = *trfc->begin();
  BOOST_CHECK_EQUAL(trf1.getStart(), 10);
  BOOST_CHECK_EQUAL(trf1.getEnd(), 100);
  BOOST_CHECK_EQUAL(trf1.getFlag(), FlagReasonFactory::ProcessingError());
  BOOST_CHECK_EQUAL(trf1.getComment(), "Bug in reco");
  BOOST_CHECK_EQUAL(trf1.getSource(), "qc/DET/QO/xyzCheck");

  auto& trf2 = *(++trfc->begin());
  BOOST_CHECK_EQUAL(trf2.getStart(), 30);
  BOOST_CHECK_EQUAL(trf2.getEnd(), 50);
  BOOST_CHECK_EQUAL(trf2.getFlag(), FlagReasonFactory::LimitedAcceptance());
  BOOST_CHECK_EQUAL(trf2.getComment(), "Sector C was off");
  BOOST_CHECK_EQUAL(trf2.getSource(), "qc/DET/QO/xyzCheck");
}

BOOST_AUTO_TEST_CASE(test_TheSameReasonsButSeparated)
{
  std::vector<QualityObject> qos{
    { Quality::Bad, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "5" }, { "Valid-Until", "25" } } },
    { Quality::Good, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "10" }, { "Valid-Until", "40" } } },
    { Quality::Bad, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "30" }, { "Valid-Until", "50" } } },
    { Quality::Bad, "xyzCheck", "DET", {}, {}, {}, { { "Valid-From", "80" }, { "Valid-Until", "100" } } }
  };
  qos[0].addReason(FlagReasonFactory::ProcessingError(), "Bug in reco");
  qos[2].addReason(FlagReasonFactory::ProcessingError(), "Bug in reco");
  qos[3].addReason(FlagReasonFactory::ProcessingError(), "Bug in reco");

  QualitiesToTRFCollectionConverter converter("test1", "DET", 5, 100, "qc/DET/QO/xyzCheck");
  for (const auto& qo : qos) {
    converter(qo);
  }
  auto trfc = converter.getResult();

  BOOST_REQUIRE_EQUAL(trfc->size(), 3);

  auto it = trfc->begin();
  auto& trf1 = *it;
  BOOST_CHECK_EQUAL(trf1.getStart(), 5);
  BOOST_CHECK_EQUAL(trf1.getEnd(), 10);
  BOOST_CHECK_EQUAL(trf1.getFlag(), FlagReasonFactory::ProcessingError());
  BOOST_CHECK_EQUAL(trf1.getComment(), "Bug in reco");
  BOOST_CHECK_EQUAL(trf1.getSource(), "qc/DET/QO/xyzCheck");

  auto& trf2 = *(++it);
  BOOST_CHECK_EQUAL(trf2.getStart(), 30);
  BOOST_CHECK_EQUAL(trf2.getEnd(), 50);
  BOOST_CHECK_EQUAL(trf2.getFlag(), FlagReasonFactory::ProcessingError());
  BOOST_CHECK_EQUAL(trf2.getComment(), "Bug in reco");
  BOOST_CHECK_EQUAL(trf2.getSource(), "qc/DET/QO/xyzCheck");

  auto& trf3 = *(++it);
  BOOST_CHECK_EQUAL(trf3.getStart(), 80);
  BOOST_CHECK_EQUAL(trf3.getEnd(), 100);
  BOOST_CHECK_EQUAL(trf3.getFlag(), FlagReasonFactory::ProcessingError());
  BOOST_CHECK_EQUAL(trf3.getComment(), "Bug in reco");
  BOOST_CHECK_EQUAL(trf3.getSource(), "qc/DET/QO/xyzCheck");
}