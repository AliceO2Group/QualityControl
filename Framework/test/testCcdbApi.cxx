///
/// \file   testCcdbApi.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/CcdbApi.h"

#define BOOST_TEST_MODULE Quality test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <cassert>
#include <iostream>
#include "curl/curl.h"

#include <stdio.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <TH1F.h>

using namespace std;
using namespace o2::ccdb;

struct test_fixture
{
  test_fixture()
  {
    api.init("http://ccdb-test.cern.ch:8080");
  }
  ~test_fixture()
  {
  }
  CcdbApi api;
  map<string, string> metadata;
};

BOOST_AUTO_TEST_CASE(store_test)
{
  test_fixture f;

  auto h1 = new TH1F("object1", "object1", 100, 0, 99);
  f.api.store(h1, "Test/Detector", f.metadata);
}

BOOST_AUTO_TEST_CASE(retrieve_test)
{
  test_fixture f;

  auto h1 = f.api.retrieve("Test/Detector", f.metadata);
  BOOST_CHECK(h1 != nullptr);
  BOOST_CHECK_EQUAL(h1->GetName(), "object1");

  auto h2 = f.api.retrieve("asdf/asdf", f.metadata);
  BOOST_CHECK_EQUAL(h2, nullptr);
}

BOOST_AUTO_TEST_CASE(truncate_test)
{
  test_fixture f;

  auto h1 = f.api.retrieve("Test/Detector", f.metadata);
  BOOST_CHECK(h1 != nullptr);
  f.api.truncate("Test/Detector");
  h1 = f.api.retrieve("Test/Detector", f.metadata);
  BOOST_CHECK(h1 == nullptr);
}

BOOST_AUTO_TEST_CASE(delete_test)
{
  test_fixture f;

  auto h1 = new TH1F("object1", "object1", 100, 0, 99);
  f.api.store(h1, "Test/Detector", f.metadata);
  auto h2 = f.api.retrieve("Test/Detector", f.metadata);
  BOOST_CHECK(h2 != nullptr);
  f.api.deleteObject("Test/Detector");
  h2 = f.api.retrieve("Test/Detector", f.metadata);
  BOOST_CHECK(h2 == nullptr);
}

BOOST_AUTO_TEST_CASE(list_test)
{
  test_fixture f;

  string s = f.api.list(); // top dir
  cout << "s : " << s << endl;

  f.api.truncate("Test/Detector");

}
