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
/// \file   testNonEmpty.cxx
/// \author Barthelemy von Haller
///

#include "Common/NonEmpty.h"

#define BOOST_TEST_MODULE Publisher test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <TH1.h>

namespace o2::quality_control_modules::common
{

BOOST_AUTO_TEST_CASE(checkable)
{
  TH1F histo("testObject", "test", 100, 0, 99);
  MonitorObject monitorObject(&histo, "task");
  monitorObject.setIsOwner(false);
  NonEmpty myCheck;
  myCheck.configure("test");

  BOOST_CHECK_EQUAL(myCheck.getAcceptedType(), "TH1");
  BOOST_CHECK_EQUAL(myCheck.isObjectCheckable(&monitorObject), true);

  TObject obj;
  monitorObject.setObject(&obj);
  BOOST_CHECK_EQUAL(myCheck.isObjectCheckable(&monitorObject), false);
}

BOOST_AUTO_TEST_CASE(beautify)
{
  auto* histo = new TH1F("testObject", "test", 100, 0, 99);
  std::shared_ptr<MonitorObject> monitorObject(new MonitorObject(histo, "task")); // here we are the owner of the histo
  NonEmpty myCheck;
  myCheck.configure("test");

  myCheck.beautify(monitorObject, Quality::Null);
  BOOST_CHECK_EQUAL(histo->GetFillColor(), kWhite);

  /*myCheck.beautify(&monitorObject, Quality::Bad);
  BOOST_CHECK_EQUAL(histo->GetFillColor(), kRed);

  myCheck.beautify(&monitorObject, Quality::Good);
  BOOST_CHECK_EQUAL(histo->GetFillColor(), kGreen);

  myCheck.beautify(&monitorObject, Quality::Medium);
  BOOST_CHECK_EQUAL(histo->GetFillColor(), kOrange);*/
}

BOOST_AUTO_TEST_CASE(nonempty)
{
  TH1F histo("testObject", "test", 100, 0, 99);
  std::shared_ptr<MonitorObject> monitorObject(new MonitorObject(&histo, "task")); // here we are the owner of the histo
  monitorObject->setIsOwner(false);
  NonEmpty myCheck;

  std::map<std::string, std::shared_ptr<MonitorObject>> moMap = {{"test", monitorObject}};

  Quality quality = myCheck.check(&moMap);
  BOOST_CHECK_EQUAL(quality, Quality::Bad);

  histo.Fill(1);
  quality = myCheck.check(&moMap);
  BOOST_CHECK_EQUAL(quality, Quality::Good);

  histo.Reset();
  quality = myCheck.check(&moMap);
  BOOST_CHECK_EQUAL(quality, Quality::Bad);
}

} // namespace o2::quality_control_modules::common
