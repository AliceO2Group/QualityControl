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
  string libName2 = "libraryName2";
  std::chrono::nanoseconds ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch());
  std::string filename = string("/tmp/test") + std::to_string(ns.count()) + ".root";
  TFile file(filename.data(), "RECREATE");
  obj.Write(obj.getName().data());
  file.Close();

  ILOG(Info) << "***" << ENDM;
  TFile file2(filename.data());
  auto* mo = dynamic_cast<o2::quality_control::core::MonitorObject*>(file2.Get(objectName.data()));
  BOOST_CHECK_NE(mo, nullptr);
  ILOG(Info) << "mo : " << mo << ENDM;
  BOOST_CHECK_EQUAL(mo->GetName(), objectName);
  BOOST_CHECK_EQUAL(mo->getName(), objectName);
  ILOG(Info) << "name : " << mo->GetName() << ENDM;
  ILOG(Info) << "name : " << mo->getName() << ENDM;
  gSystem->Unlink(filename.data());
}

BOOST_AUTO_TEST_CASE(metadata)
{
  string objectName = "asdf";
  TH1F h(objectName.data(), objectName.data(), 100, 0, 99);

  // no metadata at creation
  o2::quality_control::core::MonitorObject obj(&h, "task");
  obj.setIsOwner(false);
  BOOST_CHECK_EQUAL(obj.getMetadataMap().size(), 0);

  // add metadata with key value, check it is there
  obj.addMetadata("key1", "value1");
  BOOST_CHECK_EQUAL(obj.getMetadataMap().size(), 1);
  BOOST_CHECK_EQUAL(obj.getMetadataMap().at("key1"), "value1");

  // add same key again -> ignore
  obj.addMetadata("key1", "value1");
  BOOST_CHECK_EQUAL(obj.getMetadataMap().size(), 1);
  auto test = obj.getMetadataMap();
  BOOST_CHECK_EQUAL(obj.getMetadataMap().at("key1"), "value1");

  // add map
  map<string, string> another = {{"key2", "value2"}, {"key3", "value3"}};
  obj.addMetadata(another);
  BOOST_CHECK_EQUAL(obj.getMetadataMap().size(), 3);
  BOOST_CHECK_EQUAL(obj.getMetadataMap().at("key1"), "value1");
  BOOST_CHECK_EQUAL(obj.getMetadataMap().at("key2"), "value2");
  BOOST_CHECK_EQUAL(obj.getMetadataMap().at("key3"), "value3");

  // add map sharing some keys -> those are ignored not the others
  map<string, string> another2 = {{"key2", "value2a"}, {"key4", "value4"}};
  obj.addMetadata(another2);
  BOOST_CHECK_EQUAL(obj.getMetadataMap().size(), 4);
  BOOST_CHECK_EQUAL(obj.getMetadataMap().at("key1"), "value1");
  BOOST_CHECK_EQUAL(obj.getMetadataMap().at("key2"), "value2");
  BOOST_CHECK_EQUAL(obj.getMetadataMap().at("key3"), "value3");
  BOOST_CHECK_EQUAL(obj.getMetadataMap().at("key4"), "value4");

  // update value of existing key
  obj.updateMetadata("key1", "value11");
  BOOST_CHECK_EQUAL(obj.getMetadataMap().size(), 4);
  BOOST_CHECK_EQUAL(obj.getMetadataMap().at("key1"), "value11");

  // update value of non-existing key -> ignore
  obj.updateMetadata("asdf", "asdf");
  BOOST_CHECK_EQUAL(obj.getMetadataMap().size(), 4);
}

} // namespace o2::quality_control::core
