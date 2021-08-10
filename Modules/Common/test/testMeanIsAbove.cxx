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
/// \file   Publisher_test.cpp
/// \author Barthelemy von Haller
///

//#define private public // hack to have access to everything
#include "Common/MeanIsAbove.h"

#define BOOST_TEST_MODULE MO test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <TH1F.h>
#include <TList.h>
#include <boost/test/unit_test.hpp>

namespace o2::quality_control_modules::common
{

BOOST_AUTO_TEST_CASE(test_checks)
{
  std::shared_ptr<MonitorObject> mo(new MonitorObject()); // here we are the owner of the histo
  TH1F th1f("h1", "h1", 10, 0, 9);
  mo->setObject(&th1f);
  mo->setIsOwner(false);

  std::map<std::string, std::shared_ptr<MonitorObject>> moMap = { { "test", mo } };

  MeanIsAbove check;
  check.configure("mytest");
  Quality quality = check.check(&moMap);
  BOOST_CHECK_EQUAL(quality, Quality::Bad);

  th1f.Fill(1); // the threshold is set to 1 -> bad
  quality = check.check(&moMap);
  BOOST_CHECK_EQUAL(quality, Quality::Bad);

  th1f.Fill(2); // the threshold is set to 1 -> good
  quality = check.check(&moMap);
  BOOST_CHECK_EQUAL(quality, Quality::Good);

  check.beautify(mo, Quality::Null); // add a line
  BOOST_CHECK_EQUAL(1, th1f.GetListOfFunctions()->GetEntries());

  check.beautify(mo, Quality::Null);
  // Should update the line, not add one --> TODO THIS FAILS
  //  BOOST_CHECK_EQUAL(numberFunctions, th1f.GetListOfFunctions()->GetEntries()); // no modifications to the plot
}

BOOST_AUTO_TEST_CASE(test_types)
{
  std::shared_ptr<MonitorObject> mo(new MonitorObject()); // here we are the owner of the histo
  TObject obj;
  mo->setObject(&obj);
  mo->setIsOwner(false);

  std::map<std::string, std::shared_ptr<MonitorObject>> moMap = { { "test", mo } };

  MeanIsAbove check;
  check.configure("mytest");
  BOOST_TEST(!check.isObjectCheckable(mo));

  Quality quality = check.check(&moMap);
  BOOST_CHECK_EQUAL(quality, Quality::Null);
}

} // namespace o2::quality_control_modules::common
