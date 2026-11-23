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
/// \file   testNonEmpty.cxx
/// \author Barthelemy von Haller
///

#include "Common/NonEmpty.h"

#include "QualityControl/MonitorObject.h"
#include <TH1.h>
#include <catch_amalgamated.hpp>

namespace o2::quality_control_modules::common
{

TEST_CASE("checkable")
{
  TH1F histo("testObject", "test", 100, 0, 99);
  MonitorObject monitorObject(&histo, "task", "testClass", "TST");
  monitorObject.setIsOwner(false);
  NonEmpty myCheck;
}

TEST_CASE("beautify")
{
  auto* histo = new TH1F("testObject", "test", 100, 0, 99);
  std::shared_ptr<MonitorObject> monitorObject(new MonitorObject(histo, "task", "testClass", "TST")); // here we are the owner of the histo
  NonEmpty myCheck;

  myCheck.beautify(monitorObject, Quality::Null);
  CHECK(histo->GetFillColor() == kWhite);

  /*myCheck.beautify(&monitorObject, Quality::Bad);
  CHECK(histo->GetFillColor() == kRed);

  myCheck.beautify(&monitorObject, Quality::Good);
  CHECK(histo->GetFillColor() == kGreen);

  myCheck.beautify(&monitorObject, Quality::Medium);
  CHECK(histo->GetFillColor() == kOrange);*/
}

TEST_CASE("nonempty")
{
  TH1F histo("testObject", "test", 100, 0, 99);
  std::shared_ptr<MonitorObject> monitorObject(new MonitorObject(&histo, "task", "testClass", "TST")); // here we are the owner of the histo
  monitorObject->setIsOwner(false);
  NonEmpty myCheck;

  std::map<std::string, std::shared_ptr<MonitorObject>> moMap = { { "test", monitorObject } };

  Quality quality = myCheck.check(&moMap);
  CHECK(quality == Quality::Bad);

  histo.Fill(1);
  quality = myCheck.check(&moMap);
  CHECK(quality == Quality::Good);

  histo.Reset();
  quality = myCheck.check(&moMap);
  CHECK(quality == Quality::Bad);
}

} // namespace o2::quality_control_modules::common
