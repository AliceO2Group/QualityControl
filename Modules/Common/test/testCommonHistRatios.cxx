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
/// \file    testCommonReductors.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/Reductor.h"
#include "QualityControl/QualityObject.h"
#include "Common/TH1Ratio.h"
#include "Common/TH2Ratio.h"

#define BOOST_TEST_MODULE CommonHistRatios test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control_modules::common;

BOOST_AUTO_TEST_CASE(test_TH1FRatioUniform)
{
  auto histo1 = std::make_unique<TH1FRatio>("test1", "test1", 10, 0, 10.0, true);
  auto histo2 = std::make_unique<TH1FRatio>("test2", "test2", 10, 0, 10.0, true);
  auto histoMerged = std::make_unique<TH1FRatio>("testMerged", "testMerged", 10, 0, 10.0, true);

  for (int bin = 1; bin <= 10; bin++) {
    histo1->getNum()->SetBinContent(bin, bin * 4);
    histo2->getNum()->SetBinContent(bin, bin * 5);
  }

  histo1->getDen()->SetBinContent(1, 2);
  histo2->getDen()->SetBinContent(1, 3);

  histo1->update();
  histo2->update();

  histoMerged->merge(histo1.get());
  histoMerged->merge(histo2.get());

  for (int bin = 1; bin <= 10; bin++) {
    float value = 9.0 * bin / 5.0;
    BOOST_REQUIRE_EQUAL(histoMerged->GetBinContent(bin), value);
  }
}

BOOST_AUTO_TEST_CASE(test_TH1FRatio)
{
  auto histo1 = std::make_unique<TH1FRatio>("test1", "test1", 10, 0, 10.0, false);
  auto histo2 = std::make_unique<TH1FRatio>("test2", "test2", 10, 0, 10.0, false);
  auto histoMerged = std::make_unique<TH1FRatio>("testMerged", "testMerged", 10, 0, 10.0, false);

  for (int bin = 1; bin <= 10; bin++) {
    histo1->getNum()->SetBinContent(bin, bin * bin * 4);
    histo1->getDen()->SetBinContent(bin, bin * 3);

    histo2->getNum()->SetBinContent(bin, bin * bin * 5);
    histo2->getDen()->SetBinContent(bin, bin * 4);
  }

  histo1->update();
  histo2->update();

  histoMerged->merge(histo1.get());
  histoMerged->merge(histo2.get());

  for (int bin = 1; bin <= 10; bin++) {
    float value = 9.0 * bin / 7.0;
    BOOST_REQUIRE_EQUAL(histoMerged->GetBinContent(bin), value);
  }
}

BOOST_AUTO_TEST_CASE(test_TH2FRatioUniform)
{
  auto histo1 = std::make_unique<TH2FRatio>("test1", "test1", 10, 0, 10.0, 10, 0, 10.0, true);
  auto histo2 = std::make_unique<TH2FRatio>("test2", "test2", 10, 0, 10.0, 10, 0, 10.0, true);
  auto histoMerged = std::make_unique<TH2FRatio>("testMerged", "testMerged", 10, 0, 10.0, 10, 0, 10.0, true);

  for (int ybin = 1; ybin <= 10; ybin++) {
    for (int xbin = 1; xbin <= 10; xbin++) {
      histo1->getNum()->SetBinContent(xbin, ybin, xbin * ybin * 4);
      histo2->getNum()->SetBinContent(xbin, ybin, xbin * ybin * 5);
    }
  }

  histo1->getDen()->SetBinContent(1, 1, 2);
  histo2->getDen()->SetBinContent(1, 1, 3);

  histo1->update();
  histo2->update();

  histoMerged->merge(histo1.get());
  histoMerged->merge(histo2.get());

  for (int ybin = 1; ybin <= 10; ybin++) {
    for (int xbin = 1; xbin <= 10; xbin++) {
      float value = 9.0 * xbin * ybin / 5.0;
      BOOST_REQUIRE_EQUAL(histoMerged->GetBinContent(xbin, ybin), value);
    }
  }
}

BOOST_AUTO_TEST_CASE(test_TH2FRatio)
{
  auto histo1 = std::make_unique<TH2FRatio>("test1", "test1", 10, 0, 10.0, 10, 0, 10.0, false);
  auto histo2 = std::make_unique<TH2FRatio>("test2", "test2", 10, 0, 10.0, 10, 0, 10.0, false);
  auto histoMerged = std::make_unique<TH2FRatio>("testMerged", "testMerged", 10, 0, 10.0, 10, 0, 10.0, false);

  for (int ybin = 1; ybin <= 10; ybin++) {
    for (int xbin = 1; xbin <= 10; xbin++) {
      histo1->getNum()->SetBinContent(xbin, ybin, xbin * ybin * xbin * ybin * 4);
      histo1->getDen()->SetBinContent(xbin, ybin, xbin * ybin * 3);

      histo2->getNum()->SetBinContent(xbin, ybin, xbin * ybin * xbin * ybin * 5);
      histo2->getDen()->SetBinContent(xbin, ybin, xbin * ybin * 4);
    }
  }

  histo1->update();
  histo2->update();

  histoMerged->merge(histo1.get());
  histoMerged->merge(histo2.get());

  for (int ybin = 1; ybin <= 10; ybin++) {
    for (int xbin = 1; xbin <= 10; xbin++) {
      float value = 9.0 * xbin * ybin / 7.0;
      BOOST_REQUIRE_EQUAL(histoMerged->GetBinContent(xbin, ybin), value);
    }
  }
}
