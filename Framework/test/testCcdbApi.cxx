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
};

BOOST_AUTO_TEST_CASE(store_test)
{
  test_fixture f;

  auto h1 = new TH1F("object1", "object1", 100, 0, 99);
  map<string, string> metadata;
  f.api.store(h1, "Task/Detector", metadata);
}

BOOST_AUTO_TEST_CASE(retrieve_test)
{
  test_fixture f;

  map<string, string> metadata;
  auto h1 = f.api.retrieve("Task/Detector", metadata);
  BOOST_CHECK(h1 != nullptr);
  BOOST_CHECK_EQUAL(h1->GetName(), "object1");
}


