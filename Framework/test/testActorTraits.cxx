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
/// \file    testActivity.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/ActorTraits.h"
#include <catch_amalgamated.hpp>
#include <type_traits>

using namespace o2::quality_control::core;

TEST_CASE("test_kebab_case")
{
  {
    STATIC_CHECK(isKebabCase("a"));
    STATIC_CHECK(isKebabCase("asdf-fdsa-321"));
    STATIC_CHECK_FALSE(isKebabCase("ASDF-fdsa-321"));
    STATIC_CHECK_FALSE(isKebabCase("ASDF--fdsa-321"));
    STATIC_CHECK_FALSE(isKebabCase("-asdf"));
    STATIC_CHECK_FALSE(isKebabCase("asdf-"));
    STATIC_CHECK_FALSE(isKebabCase(""));
  }
}