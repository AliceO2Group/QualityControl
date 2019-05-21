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
/// \file   testQcInfoLogger.cxx
/// \author Barthelemy von Haller
///

#include "../include/QualityControl/QcInfoLogger.h"

#define BOOST_TEST_MODULE Quality test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <cassert>
#include <iostream>

using namespace std;

namespace o2::quality_control::core
{

BOOST_AUTO_TEST_CASE(qc_info_logger)
{
  QcInfoLogger& qc1 = QcInfoLogger::GetInstance();
  QcInfoLogger& qc2 = QcInfoLogger::GetInstance();
  BOOST_CHECK_EQUAL(&qc1, &qc2);
  qc1 << "test" << AliceO2::InfoLogger::InfoLogger::endm;
}

} // namespace o2::quality_control::core
