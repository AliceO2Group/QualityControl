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

#include "QualityControl/QcInfoLogger.h"

#define BOOST_TEST_MODULE InfoLogger test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace AliceO2::InfoLogger;

namespace o2::quality_control::core
{

BOOST_AUTO_TEST_CASE(qc_info_logger)
{
  QcInfoLogger& qc1 = QcInfoLogger::GetInstance();
  QcInfoLogger& qc2 = QcInfoLogger::GetInstance();
  BOOST_CHECK_EQUAL(&qc1, &qc2);
  qc1 << "test" << AliceO2::InfoLogger::InfoLogger::endm;
}

BOOST_AUTO_TEST_CASE(qc_info_logger_2)
{
  // Decreasing verbosity of the code
  QcInfoLogger::GetInstance() << "1. info message" << AliceO2::InfoLogger::InfoLogger::endm;
  QcInfoLogger::GetInstance() << "2. info message" << InfoLogger::endm;
  ILOG << "3. info message" << InfoLogger::endm;
  ILOG << "4. info message" << ENDM;

  // Complexification of the messages
  ILOG << InfoLogger::Error << "5. error message" << ENDM;
  ILOG << InfoLogger::Error << "6. error message" << InfoLogger::Info << " - 7. info message" << ENDM;
  ILOG << InfoLogger::InfoLoggerMessageOption{InfoLogger::Fatal, 1, 1, "asdf", 3}
       << "8. fatal message with extra fields" << ENDM;

  // Different syntax
  ILOGD(Warning) << "9. warning message" << InfoLogger::endm;

  // Check how the QC infologger is configured
  ILOG.logInfo("a. info message");
  ILOG.logError("b. error message");
  ILOG.log("c. message");
}

} // namespace o2::quality_control::core
