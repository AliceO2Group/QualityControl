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

#include "QualityControl/MonitorObject.h"
#include "QualityControl/QcInfoLogger.h"

#define BOOST_TEST_MODULE MO test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <chrono>
#include <iostream>
#include <TH1F.h>
#include <TFile.h>
#include <TSystem.h>

using namespace std;

namespace o2::quality_control::core
{

BOOST_AUTO_TEST_CASE(mo)
{
  o2::quality_control::core::MonitorObject obj;
  BOOST_CHECK_EQUAL(obj.getName(), "");
  BOOST_CHECK_EQUAL(obj.GetName(), "");

  obj.addCheck("first", "class1", "lib1");
  obj.addCheck("second", "class1", "lib1");
  obj.addCheck("third", "class2", "lib1");
  obj.addCheck("first", "class2", "lib1");
  auto checkers1 = obj.getChecks();
  BOOST_CHECK_EQUAL(checkers1["first"].name, "first");
  BOOST_CHECK_EQUAL(checkers1["first"].className, "class2");
  BOOST_CHECK_EQUAL(checkers1["first"].libraryName, "lib1");
  BOOST_CHECK_EQUAL(obj.getCheck("second").name, "second");
  BOOST_CHECK_EQUAL(obj.getCheck("second").className, "class1");
  BOOST_CHECK_EQUAL(obj.getCheck("second").libraryName, "lib1");

  BOOST_CHECK_EQUAL(obj.getQuality(), Quality::Null);
  obj.setQualityForCheck("first", Quality::Good);
  BOOST_CHECK_EQUAL(obj.getQuality(), Quality::Good);
  obj.setQualityForCheck("second", Quality::Bad);
  BOOST_CHECK_EQUAL(obj.getQuality(), Quality::Bad);
  obj.setQualityForCheck("second", Quality::Medium);
  BOOST_CHECK_EQUAL(obj.getQuality(), Quality::Medium);
}

BOOST_AUTO_TEST_CASE(mo_check)
{
  o2::quality_control::core::MonitorObject obj;
  BOOST_CHECK_EQUAL(obj.getQuality(), Quality::Null);
  CheckDefinition check;
  check.name = "test";
  check.libraryName = "test";
  check.className = "test";
  obj.addOrReplaceCheck("test", check);
  BOOST_CHECK_EQUAL(obj.getQuality(), Quality::Null);
}

BOOST_AUTO_TEST_CASE(mo_save)
{
  string objectName = "asdf";
  TH1F h(objectName.data(), objectName.data(), 100, 0, 99);
  o2::quality_control::core::MonitorObject obj(&h, "task");
  ILOG(Info) << "getName : '" << obj.getName() << "'" << ENDM;
  ILOG(Info) << "GetName : '" << obj.GetName() << "'" << ENDM;
  ILOG(Info) << "title : '" << obj.GetTitle() << "'" << ENDM;
  BOOST_CHECK_EQUAL(obj.getName(), "asdf");
  BOOST_CHECK_EQUAL(obj.GetName(), "asdf");
  BOOST_CHECK_EQUAL(obj.GetTitle(), "");
  obj.setIsOwner(false);
  string libName = "libraryName";
  obj.addCheck("name", "className", libName);
  string libName2 = "libraryName2";
  obj.addCheck("name2", "className2", libName2);
  obj.setQualityForCheck("name", Quality::Good);
  BOOST_CHECK_EQUAL(obj.getQuality(), Quality::Good);
  ILOG(Info) << "quality : " << obj.getQuality() << ENDM;
  ILOG(Info) << "check numbers : " << obj.getChecks().size() << ENDM;
  CheckDefinition c = obj.getCheck("name2");
  ILOG(Info) << "check2 libraryName : " << c.libraryName << ENDM;
  std::chrono::nanoseconds ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch());
  std::string filename = string("/tmp/test") + std::to_string(ns.count()) + ".root";
  TFile file(filename.data(), "RECREATE");
  obj.Write(obj.getName().data());
  file.Close();

  ILOG(Info) << "***" << ENDM;
  TFile file2(filename.data());
  o2::quality_control::core::MonitorObject* mo = dynamic_cast<o2::quality_control::core::MonitorObject*>(file2.Get(objectName.data()));
  BOOST_CHECK_NE(mo, nullptr);
  ILOG(Info) << "mo : " << mo << ENDM;
  BOOST_CHECK_EQUAL(mo->GetName(), objectName);
  BOOST_CHECK_EQUAL(mo->getName(), objectName);
  ILOG(Info) << "name : " << mo->GetName() << ENDM;
  ILOG(Info) << "name : " << mo->getName() << ENDM;
  BOOST_CHECK_EQUAL(mo->getQuality(), Quality::Good);
  ILOG(Info) << "quality : " << mo->getQuality() << ENDM;
  BOOST_CHECK_EQUAL(mo->getChecks().size(), 2);
  ILOG(Info) << "check numbers : " << mo->getChecks().size() << ENDM;
  CheckDefinition c2 = mo->getCheck("name2");
  ILOG(Info) << "check2 libraryName : " << c2.libraryName << ENDM;
  BOOST_CHECK_EQUAL(c2.libraryName, libName2);
  gSystem->Unlink(filename.data());
}

} // namespace o2::quality_control::core
