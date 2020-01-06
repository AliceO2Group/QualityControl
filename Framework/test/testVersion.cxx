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

BOOST_AUTO_TEST_CASE(test_original_version)
{
  assert((Version("3.7.8.0") == Version("3.7.8.0")) == true);
  assert((Version("3.7.8.0") == Version("3.7.8")) == true);
  assert((Version("3.7.8.0") < Version("3.7.8")) == false);
  assert((Version("3.7.9") < Version("3.7.8")) == false);
  assert((Version("3") < Version("3.7.9")) == true);
  assert((Version("1.7.9") < Version("3.1")) == true);
  assert((Version("") == Version("0.0.0")) == true);
  assert((Version("0") == Version("0.0.0")) == true);
  assert((Version("") != Version("0.0.1")) == true);

  std::cout << "Printing version (3.7.8): " << Version("3.7.8.0") << std::endl;
}

BOOST_AUTO_TEST_CASE(test_version)
{
  Version v("2.0.0");
  BOOST_CHECK(v == Version("2.0.0"));
  Version qc = Version::GetQcVersion();
  BOOST_CHECK(qc.getMajor() != 0 || qc.getMinor() != 0 || qc.getPatch() != 0);
  cout << qc << endl;
}

BOOST_AUTO_TEST_CASE(test_output)
{
  Version v("1.2.3");
  boost::test_tools::output_test_stream output;
  output << v;

  BOOST_CHECK(output.is_equal("1.2.3"));
}

} // namespace o2::quality_control::core
