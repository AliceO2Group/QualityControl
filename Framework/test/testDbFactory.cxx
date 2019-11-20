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
/// \file   testDbFactory.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/DatabaseFactory.h"

#ifdef _WITH_MYSQL

#include "QualityControl/MySqlDatabase.h"

#endif
#define BOOST_TEST_MODULE DbFactory test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
//#include <cassert>
//#include <iostream>

#include <QualityControl/DummyDatabase.h>
#include <QualityControl/CcdbDatabase.h>
#include <QualityControl/MonitorObject.h>
#include <TH1F.h>
//#include <fcntl.h>
//#include <stdio.h>
//#include <sys/stat.h>

using namespace std;
using namespace o2::quality_control::core;

namespace o2::quality_control::repository
{

bool do_nothing(AliceO2::Common::FatalException const&) { return true; }

BOOST_AUTO_TEST_CASE(db_factory_test)
{
#ifdef _WITH_MYSQL
  std::unique_ptr<DatabaseInterface> database = DatabaseFactory::create("MySql");
  BOOST_CHECK(database);
  BOOST_CHECK(dynamic_cast<MySqlDatabase*>(database.get()));
#endif

  std::unique_ptr<DatabaseInterface> database2 = nullptr;
  BOOST_CHECK_EXCEPTION(database2 = DatabaseFactory::create("asf"), AliceO2::Common::FatalException, do_nothing);
  BOOST_CHECK(!database2);

  std::unique_ptr<DatabaseInterface> database3 = DatabaseFactory::create("CCDB");
  BOOST_CHECK(database3);
  BOOST_CHECK(dynamic_cast<CcdbDatabase*>(database3.get()));

  std::unique_ptr<DatabaseInterface> database4 = DatabaseFactory::create("Dummy");
  BOOST_CHECK(database4);
  BOOST_CHECK(dynamic_cast<DummyDatabase*>(database4.get()));
}

BOOST_AUTO_TEST_CASE(db_ccdb_listing)
{
  std::unique_ptr<DatabaseInterface> database3 = DatabaseFactory::create("CCDB");
  BOOST_CHECK(database3);
  auto* ccdb = dynamic_cast<CcdbDatabase*>(database3.get());
  BOOST_CHECK(ccdb);

  ccdb->connect("ccdb-test.cern.ch:8080", "", "", "");

  // prepare stuff in the db
  string prefixPath = "qc/TST/";
  ccdb->truncate(prefixPath + "functional_test", "object1");
  ccdb->truncate(prefixPath + "functional_test", "object2");
  ccdb->truncate(prefixPath + "functional_test", "path/to/object3");
  auto* h1 = new TH1F("object1", "object1", 100, 0, 99);
  auto* h2 = new TH1F("object2", "object2", 100, 0, 99);
  auto* h3 = new TH1F("path/to/object3", "object3", 100, 0, 99);
  shared_ptr<MonitorObject> mo1 = make_shared<MonitorObject>(h1, "functional_test", "TST");
  shared_ptr<MonitorObject> mo2 = make_shared<MonitorObject>(h2, "functional_test", "TST");
  shared_ptr<MonitorObject> mo3 = make_shared<MonitorObject>(h3, "functional_test", "TST");
  ccdb->store(mo1);
  ccdb->store(mo2);
  ccdb->store(mo3);

  // test getting list of tasks
  std::vector<std::string> list = ccdb->getListing(prefixPath);
  //  for (const auto& item : list) {
  //    cout << "task : " << item << endl;
  //  }
  BOOST_CHECK(std::find(list.begin(), list.end(), prefixPath + "functional_test") != list.end());

  // test getting objects list from task
  auto objectNames = ccdb->getPublishedObjectNames(prefixPath + "functional_test");
  //  cout << "objects in task functional_test" << endl;
  //  for (auto name : objectNames) {
  //    cout << " - object : " << name << endl;
  //  }
  BOOST_CHECK(std::find(objectNames.begin(), objectNames.end(), "/object1") != objectNames.end());
  BOOST_CHECK(std::find(objectNames.begin(), objectNames.end(), "/object2") != objectNames.end());
  BOOST_CHECK(std::find(objectNames.begin(), objectNames.end(), "/path\\/to\\/object3") != objectNames.end());

  // store list of streamer infos
  //    ccdb->storeStreamerInfosToFile("streamerinfos.root");
}

} // namespace o2::quality_control::repository
