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
/// \file   testQuality.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/Version.h"

#define BOOST_TEST_MODULE Quality test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/test/output_test_stream.hpp>

using namespace std;

namespace o2::quality_control::core
{

BOOST_AUTO_TEST_CASE(test_int_repr)
{
  Version v1("0.19.2");
  Version v2("1.19.2");
  Version v3("2.0.0");
  BOOST_CHECK(v1.getIntegerRepresentation() == (19002));
  BOOST_CHECK(v2.getIntegerRepresentation() == 1019002);
  BOOST_CHECK(v3.getIntegerRepresentation() == 2000000);
}

BOOST_AUTO_TEST_CASE(test_version)
{
  BOOST_CHECK((Version("3.7.8.0") == Version("3.7.8.0")) == true);
  BOOST_CHECK((Version("3.7.8.0") == Version("3.7.8")) == true);
  BOOST_CHECK((Version("3.7.8.0") < Version("3.7.8")) == false);
  BOOST_CHECK((Version("3.7.9") < Version("3.7.8")) == false);
  BOOST_CHECK((Version("3") < Version("3.7.9")) == true);
  BOOST_CHECK((Version("1.7.9") < Version("3.1")) == true);
  BOOST_CHECK((Version("") == Version("0.0.0")) == true);
  BOOST_CHECK((Version("0") == Version("0.0.0")) == true);
  BOOST_CHECK((Version("") != Version("0.0.1")) == true);
  BOOST_CHECK((Version("2.0.0") < Version("1.19.0")) == false);

  Version v("2.0.0");
  BOOST_CHECK(v == Version("2.0.0"));
  Version qc = Version::GetQcVersion();
  BOOST_CHECK(qc.getMajor() != 0 || qc.getMinor() != 0 || qc.getPatch() != 0);
  cout << qc << endl;

  Version v2("3.2.1");
  BOOST_CHECK(v2.getMajor() == 3);
  BOOST_CHECK(v2.getMinor() == 2);
  BOOST_CHECK(v2.getPatch() == 1);

  BOOST_CHECK(v < Version("2.1.0"));
  BOOST_CHECK(v < Version("2.1"));
  BOOST_CHECK(v < Version("20"));
  BOOST_CHECK(v >= Version("1.19"));
  BOOST_CHECK(v >= Version("1"));
  BOOST_CHECK(v >= Version("1.8.1"));
  BOOST_CHECK(v >= Version("2.0.0"));
  BOOST_CHECK(v >= Version("2.0"));
  BOOST_CHECK(v > Version("1.19"));
  BOOST_CHECK(v > Version("1"));
  BOOST_CHECK(v > Version("1.8.1"));
  BOOST_CHECK(!(v > Version("2.0.0")));
}

BOOST_AUTO_TEST_CASE(test_output)
{
  Version v("1.2.3");
  boost::test_tools::output_test_stream output;
  output << v;

  BOOST_CHECK(output.is_equal("1.2.3"));

  BOOST_CHECK(v.getString() == "1.2.3");
}

} // namespace o2::quality_control::core
