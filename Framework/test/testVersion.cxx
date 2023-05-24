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
/// \file   testQuality.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/Version.h"
#include <catch_amalgamated.hpp>

using namespace std;

namespace o2::quality_control::core
{

TEST_CASE("test_int_repr")
{
  Version v1("0.19.2");
  Version v2("1.19.2");
  Version v3("2.0.0");
  CHECK(v1.getIntegerRepresentation() == (19002));
  CHECK(v2.getIntegerRepresentation() == 1019002);
  CHECK(v3.getIntegerRepresentation() == 2000000);
}

TEST_CASE("test_version")
{
  CHECK((Version("3.7.8.0") == Version("3.7.8.0")) == true);
  CHECK((Version("3.7.8.0") == Version("3.7.8")) == true);
  CHECK((Version("3.7.8.0") < Version("3.7.8")) == false);
  CHECK((Version("3.7.9") < Version("3.7.8")) == false);
  CHECK((Version("3") < Version("3.7.9")) == true);
  CHECK((Version("1.7.9") < Version("3.1")) == true);
  CHECK((Version("") == Version("0.0.0")) == true);
  CHECK((Version("0") == Version("0.0.0")) == true);
  CHECK((Version("") != Version("0.0.1")) == true);
  CHECK((Version("2.0.0") < Version("1.19.0")) == false);

  Version v("2.0.0");
  CHECK(v == Version("2.0.0"));
  Version qc = Version::GetQcVersion();
  CHECK((qc.getMajor() != 0 || qc.getMinor() != 0 || qc.getPatch() != 0));
  cout << qc << endl;

  Version v2("3.2.1");
  CHECK(v2.getMajor() == 3);
  CHECK(v2.getMinor() == 2);
  CHECK(v2.getPatch() == 1);

  CHECK(v < Version("2.1.0"));
  CHECK(v < Version("2.1"));
  CHECK(v < Version("20"));
  CHECK(v >= Version("1.19"));
  CHECK(v >= Version("1"));
  CHECK(v >= Version("1.8.1"));
  CHECK(v >= Version("2.0.0"));
  CHECK(v >= Version("2.0"));
  CHECK(v > Version("1.19"));
  CHECK(v > Version("1"));
  CHECK(v > Version("1.8.1"));
  CHECK(!(v > Version("2.0.0")));
}

TEST_CASE("test_output")
{
  Version v("1.2.3");
  std::stringstream output;
  output << v;

  CHECK(output.str() == "1.2.3");

  CHECK(v.getString() == "1.2.3");
}

} // namespace o2::quality_control::core
