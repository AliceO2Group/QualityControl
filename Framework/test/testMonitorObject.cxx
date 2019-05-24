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
/// \file   testMonitorObject.cxx
/// \author Barthelemy von Haller
///

#include "../include/QualityControl/MonitorObject.h"

#define BOOST_TEST_MODULE MO test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <cassert>
#include <TH1F.h>
#include <TFile.h>

using namespace std;

namespace o2::quality_control::core
{

//BOOST_AUTO_TEST_CASE(mo)
//{
//  o2::quality_control::core::MonitorObject obj;
//  BOOST_CHECK_EQUAL(obj.getName(), "");
//
//  obj.addCheck("first", "class1", "lib1");
//  obj.addCheck("second", "class1", "lib1");
//  obj.addCheck("third", "class2", "lib1");
//  obj.addCheck("first", "class2", "lib1");
//  auto checkers1 = obj.getChecks();
//  BOOST_CHECK_EQUAL(checkers1["first"].name, "first");
//  BOOST_CHECK_EQUAL(checkers1["first"].className, "class2");
//  BOOST_CHECK_EQUAL(checkers1["first"].libraryName, "lib1");
//  BOOST_CHECK_EQUAL(obj.getCheck("second").name, "second");
//  BOOST_CHECK_EQUAL(obj.getCheck("second").className, "class1");
//  BOOST_CHECK_EQUAL(obj.getCheck("second").libraryName, "lib1");
//
//  BOOST_CHECK_EQUAL(obj.getQuality(), Quality::Null);
//  obj.setQualityForCheck("first", Quality::Good);
//  BOOST_CHECK_EQUAL(obj.getQuality(), Quality::Good);
//  obj.setQualityForCheck("second", Quality::Bad);
//  BOOST_CHECK_EQUAL(obj.getQuality(), Quality::Bad);
//  obj.setQualityForCheck("second", Quality::Medium);
//  BOOST_CHECK_EQUAL(obj.getQuality(), Quality::Medium);
//}
//BOOST_AUTO_TEST_CASE(mo_check)
//{
//  o2::quality_control::core::MonitorObject obj;
//  BOOST_CHECK_EQUAL(obj.getQuality(), Quality::Null);
//  CheckDefinition check;
//  check.name = "test";
//  check.libraryName = "test";
//  check.className = "test";
//  obj.addOrReplaceCheck("test", check);
//  BOOST_CHECK_EQUAL(obj.getQuality(), Quality::Null);
//}

BOOST_AUTO_TEST_CASE(mo_save)
{
  TH1F h("asdf", "asdf", 100, 0, 99);
  o2::quality_control::core::MonitorObject obj(&h, "task");
  cout << "getName : '" << obj.getName() << "'" <<endl;
  cout << "GetName : '" << obj.GetName() << "'" << endl;
  cout << "title : '" << obj.GetTitle() << "'" << endl;
  obj.setIsOwner(false);
  TFile file("/tmp/test.root", "RECREATE");
  obj.Write(obj.getName().data());
//  h.Write();
  file.Close();
}

BOOST_AUTO_TEST_CASE(mo_read)
{
  cout << "***" << endl;
  TFile file("/tmp/test.root");
//  file.Print();
//  file.Map();
//  file.ShowStreamerInfo();
//  TH1F* mo = dynamic_cast<TH1F*>(file.Get("asdf"));
  o2::quality_control::core::MonitorObject *mo = dynamic_cast<o2::quality_control::core::MonitorObject*>(file.Get("asdf"));
  cout << "mo : " << mo << endl;
  cout << "name : " << mo->GetName() << endl;
  cout << "name : " << mo->getName() << endl;

//  TH1F h("asdf", "asdf", 100, 0, 99);
//  o2::quality_control::core::MonitorObject obj(&h, "taskname");
//  obj.setIsOwner(false);
//  obj.SaveAs("/tmp/test.root");
}

} // namespace o2::quality_control::core
