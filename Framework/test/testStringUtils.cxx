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
/// \file    testStringUtils.cxx
/// \author  Barthelemy von Haller
///

#include "QualityControl/stringUtils.h"

#include <catch_amalgamated.hpp>

using namespace o2::quality_control::core;

TEST_CASE("isUnsignedInteger() accepts only unsigned integers")
{
  CHECK(isUnsignedInteger("1") == true);
  CHECK(isUnsignedInteger("-1") == false);
  CHECK(isUnsignedInteger("1000000") == true);
  CHECK(isUnsignedInteger("0.1") == false);
  CHECK(isUnsignedInteger(".2") == false);
  CHECK(isUnsignedInteger("x") == false);
  CHECK(isUnsignedInteger("1x") == false);
  CHECK(isUnsignedInteger("......") == false);
}

TEST_CASE("test_kebab_case")
{
  STATIC_CHECK(isKebabCase("a"));
  STATIC_CHECK(isKebabCase("asdf-fdsa-321"));
  STATIC_CHECK(isKebabCase("a-b-c"));
  STATIC_CHECK_FALSE(isKebabCase("ASDF-fdsa-321"));
  STATIC_CHECK_FALSE(isKebabCase("ASDF--fdsa-321"));
  STATIC_CHECK_FALSE(isKebabCase("-asdf"));
  STATIC_CHECK_FALSE(isKebabCase("asdf-"));
  STATIC_CHECK_FALSE(isKebabCase(""));
}

TEST_CASE("isUpperCamelCase() validates UpperCamelCase identifiers")
{
  STATIC_CHECK(isUpperCamelCase("TaskRunner"));
  STATIC_CHECK(isUpperCamelCase("URLParser"));
  STATIC_CHECK(isUpperCamelCase("A"));
  STATIC_CHECK(isUpperCamelCase("A1"));
  STATIC_CHECK(isUpperCamelCase("My2DPlot"));

  STATIC_CHECK(isUpperCamelCase("") == false);
  STATIC_CHECK(isUpperCamelCase("taskRunner") == false);
  STATIC_CHECK(isUpperCamelCase("1Task") == false);
  STATIC_CHECK(isUpperCamelCase("Task_Runner") == false);
  STATIC_CHECK(isUpperCamelCase("Task-Runner") == false);
  STATIC_CHECK(isUpperCamelCase("task-runner") == false);
  STATIC_CHECK(isUpperCamelCase("Task Runner") == false);
}