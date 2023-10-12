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
/// \file   testPtreeUtils.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/ptreeUtils.h"

#define BOOST_TEST_MODULE Publisher test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace std;
using namespace o2::quality_control::core;
using boost::property_tree::ptree;

namespace o2::quality_control::core
{

ptree getTree(const std::string& str)
{
  ptree tree1;
  stringstream stream1;
  stream1 << str;
  read_json(stream1, tree1);
  return tree1;
}

BOOST_AUTO_TEST_CASE(test_simple_json_merge2)
{
    cout << "test_simple_json_merge" << endl;
    std::string file1 =
      "{\n"
      "  \"AAA\": {\n"
      "    \"name\" : \"barth\"\n"
      "  }\n"
      "}";
    string file2 =
      "{\n"
      "  \"BBB\": {\n"
      "    \"name\" : \"von Haller\"\n"
      "  }\n"
      "}";
    ptree tree1 = getTree(file1);
    ptree tree2 = getTree(file2);

    ptree merged;

    mergeInto(tree1, /*merged, */merged, "", 1);
    mergeInto(tree2,/* merged,*/ merged, "", 1);

    boost::property_tree::write_json(std::cout, merged);

    BOOST_CHECK_EQUAL(merged.get<string>("AAA.name"), "barth");
    BOOST_CHECK_EQUAL(merged.get<string>("BBB.name"), "von Haller");
}

BOOST_AUTO_TEST_CASE(test_json_merge_identical2)
{
    cout << "test_json_merge_identical" << endl;
    std::string file1 =
      "{\n"
      "  \"AAA\": {\n"
      "    \"name\" : \"Barth\"\n"
      "  }\n"
      "}";
    string file2 =
      "{\n"
      "  \"AAA\": {\n"
      "    \"name\" : \"von Haller\"\n"
      "  }\n"
      "}";
    ptree tree1 = getTree(file1);
    ptree tree2 = getTree(file2);

    ptree merged;

    mergeInto(tree1, /*merged, */merged, "", 1);
    mergeInto(tree2,/* merged,*/ merged, "", 1);

    boost::property_tree::write_json(std::cout, merged);

    ptree sub = merged.get_child("AAA");
    BOOST_CHECK_EQUAL(sub.size(), 2);
    int i = 0;
    for (auto it = sub.begin(); it != sub.end(); ++it, i++)
    {
      cout << it->first << endl;
      BOOST_CHECK_EQUAL(it->first, "name");
      if(i == 0) {
        BOOST_CHECK_EQUAL(it->second.get_value<string>(), "Barth");
      } else {
        BOOST_CHECK_EQUAL(it->second.get_value<string>(), "von Haller");
      }
    }
}

BOOST_AUTO_TEST_CASE(test_json_merge_2_tasks2)
{
    cout << "test_json_merge_2_tasks" << endl;
    std::string file1 ="{\n"
      " \"qc\": {\n"
      "    \"tasks\": {\n"
      "      \"QcTask\": {\n"
      "        \"active\": \"true\",\n"
      "        \"className\": \"o2::quality_control_modules::skeleton::SkeletonTask\",\n"
      "        \"moduleName\": \"QcSkeleton\",\n"
      "        \"detectorName\": \"TST\",\n"
      "        \"cycleDurationSeconds\": \"10\",     \"\": \"10 seconds minimum\",\n"
      "        \"maxNumberCycles\": \"-1\",\n"
      "        \"\": \"The other type of dataSource is \\\"direct\\\", see basic-no-sampling.json.\",\n"
      "        \"dataSource\": {\n"
      "          \"type\": \"dataSamplingPolicy\",\n"
      "          \"name\": \"tst-raw\"\n"
      "        },\n"
      "        \"taskParameters\": {\n"
      "          \"myOwnKey\": \"myOwnValue\"\n"
      "        },\n"
      "        \"location\": \"remote\",\n"
      "        \"saveObjectsToFile\": \"\",      \"\": \"For debugging, path to the file where to save. If empty or missing it won't save.\"\n"
      "      }\n"
      "    }"
      "}"
      "}";
    std::string file2 ="{\n"
      " \"qc\": {\n"
      "    \"tasks\": {\n"
      "      \"QcTask2\": {\n"
      "        \"active\": \"true\",\n"
      "        \"className\": \"o2::quality_control_modules::skeleton::SkeletonTask\",\n"
      "        \"moduleName\": \"QcSkeleton\",\n"
      "        \"detectorName\": \"TST\",\n"
      "        \"cycleDurationSeconds\": \"10\",     \"\": \"10 seconds minimum\",\n"
      "        \"maxNumberCycles\": \"-1\",\n"
      "        \"\": \"The other type of dataSource is \\\"direct\\\", see basic-no-sampling.json.\",\n"
      "        \"dataSource\": {\n"
      "          \"type\": \"dataSamplingPolicy\",\n"
      "          \"name\": \"tst-raw\"\n"
      "        },\n"
      "        \"taskParameters\": {\n"
      "          \"myOwnKey\": \"myOwnValue\"\n"
      "        },\n"
      "        \"location\": \"remote\",\n"
      "        \"saveObjectsToFile\": \"\",      \"\": \"For debugging, path to the file where to save. If empty or missing it won't save.\"\n"
      "      }\n"
      "    }"
      "}"
      "}";
    ptree tree1 = getTree(file1);
    ptree tree2 = getTree(file2);

    ptree merged;

    mergeInto(tree1, /*merged,*/ merged, "", 1);
    mergeInto(tree2, /*merged,*/ merged, "", 1);

    boost::property_tree::write_json(std::cout, merged);

    ptree tasks = merged.get_child("qc.tasks");
    BOOST_CHECK_EQUAL(tasks.size(), 2);
}

BOOST_AUTO_TEST_CASE(test_json_merge_simple_arrays2)
{
    cout << "test_json_merge_simple_arrays2" << endl;

    std::string file1 =
      "{\"MOs\": [\"example\"]}";
    string file2 =
      "{\"MOs\": [\"example2\"]}";
    ptree tree1 = getTree(file1);
    ptree tree2 = getTree(file2);

    std::cout << "tree1: " << std::endl;
    boost::property_tree::write_json(std::cout, tree1);
    std::cout << "tree2: " << std::endl;
    boost::property_tree::write_json(std::cout, tree2);

    ptree merged;

    mergeInto(tree1, /*merged, */merged, "", 1);
    cout << "merged after tree1: " << endl;
    boost::property_tree::write_json(std::cout, merged);

    mergeInto(tree2, /*merged,*/ merged, "", 1);
    cout << "merged after tree2: " << endl;
    boost::property_tree::write_json(std::cout, merged);

    ptree mos = merged.get_child("MOs");
    BOOST_CHECK_EQUAL(mos.size(), 2);
    auto it = mos.begin();
    BOOST_CHECK_EQUAL(it->second.get_value<string>(), "example");
    it++;
    BOOST_CHECK_EQUAL(it->second.get_value<string>(), "example2");
}

BOOST_AUTO_TEST_CASE(test_json_merge_arrays_objects2)
{
    cout << "test_json_merge_arrays_objects" << endl;

    std::string file1 =
      "{\n"
      "  \"dataSource\": [{\n"
      "    \"type\": \"Task\",\n"
      "    \"name\": \"Task\"\n"
      "  }]\n"
      "}";
    string file2 =
      "{\n"
      "  \"dataSource\": [{\n"
      "    \"type\": \"Task2\",\n"
      "    \"name\": \"Task2\"\n"
      "  }]\n"
      "}";
    ptree tree1 = getTree(file1);
    ptree tree2 = getTree(file2);

    ptree merged;

    mergeInto(tree1, /*merged, */merged, "", 1);
    mergeInto(tree2, /*merged, */merged, "", 1);

    boost::property_tree::write_json(std::cout, tree1);
    boost::property_tree::write_json(std::cout, tree2);
    boost::property_tree::write_json(std::cout, merged);

    ptree array = merged.get_child("dataSource");
    BOOST_CHECK_EQUAL(array.size(), 2);
}

BOOST_AUTO_TEST_CASE(test_json_merge_arrays_of_arrays2)
{
    cout << "test_json_merge_arrays_of_arrays" << endl;

    std::string file1 =
      "{\n"
      "  \"dataSamplingPolicies\": [\n"
      "    {\n"
      "      \"id\": \"tst-raw\",\n"
      "      \"samplingConditions\": [\n"
      "        {\n"
      "          \"condition\": \"random\",\n"
      "          \"fraction\": \"0.1\"\n"
      "        }\n"
      "      ],\n"
      "      \"blocking\": \"false\"\n"
      "    }\n"
      "  ]\n"
      "}";
    string file2 =
      "{\n"
      "  \"dataSamplingPolicies\": [\n"
      "    {\n"
      "      \"id\": \"tst-raw2\",\n"
      "      \"samplingConditions\": [\n"
      "        {\n"
      "          \"condition\": \"random\",\n"
      "          \"fraction\": \"0.1\"\n"
      "        }\n"
      "      ],\n"
      "      \"blocking\": \"false\"\n"
      "    }\n"
      "  ]\n"
      "}";
    ptree tree1 = getTree(file1);
    ptree tree2 = getTree(file2);

    ptree merged;

    mergeInto(tree1, /*merged, */merged, "", 1);
    mergeInto(tree2,/* merged, */merged, "", 1);

    boost::property_tree::write_json(std::cout, tree1);
    boost::property_tree::write_json(std::cout, tree2);
    cout << "" << endl;
    boost::property_tree::write_json(std::cout, merged);

    ptree array = merged.get_child("dataSamplingPolicies");
    BOOST_CHECK_EQUAL(array.size(), 2);
}

BOOST_AUTO_TEST_CASE(test_json_merge_comments)
{
    cout << "test_json_merge_simple_arrays2" << endl;

    std::string file1 =
      "{\"infologger\": {\n"
      "                \"\": \"Configuration of the Infologger (optional).\",\n"
      "                \"filterDiscardDebug\": \"false\",\n"
      "                \"\": [\n"
      "                    \"messages won't go there.\"\n"
      "                ]\n"
      "            }}";
    ptree tree1 = getTree(file1);

    std::cout << "tree1: " << std::endl;
    boost::property_tree::write_json(std::cout, tree1);

    ptree merged;

    mergeInto(tree1, /*merged, */ merged, "", 1);

    cout << "merged after tree1: " << endl;
    boost::property_tree::write_json(std::cout, merged);
}

} // namespace o2::quality_control::core
