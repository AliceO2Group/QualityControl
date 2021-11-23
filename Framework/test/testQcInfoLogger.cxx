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
/// \file   testQcInfoLogger.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/QcInfoLogger.h"

#define BOOST_TEST_MODULE InfoLogger test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <fairlogger/Logger.h>
#include <cstdlib>

using namespace std;
using namespace AliceO2::InfoLogger;

namespace o2::quality_control::core
{

BOOST_AUTO_TEST_CASE(qc_info_logger)
{
  AliceO2::InfoLogger::InfoLogger& qc1 = QcInfoLogger::GetInfoLogger();
  AliceO2::InfoLogger::InfoLogger& qc2 = QcInfoLogger::GetInfoLogger();
  BOOST_CHECK_EQUAL(&qc1, &qc2);
  qc1 << "test" << AliceO2::InfoLogger::InfoLogger::endm;
}

BOOST_AUTO_TEST_CASE(qc_info_logger_2)
{
  // Decreasing verbosity of the code
  QcInfoLogger::GetInfoLogger() << "1. info message" << AliceO2::InfoLogger::InfoLogger::endm;
  QcInfoLogger::GetInfoLogger() << "2. info message" << InfoLogger::endm;
  ILOG(Info, Support) << "3. info message for support" << InfoLogger::endm;
  ILOG(Info, Devel) << "4. info message for devel" << ENDM;
  ILOG(Info) << "4b. info MEssage for default level" << ENDM;

  // Complexification of the messages
  ILOG(Error) << "5. error message" << ENDM;
  ILOG(Error) << "6. error message" << LogInfoSupport << " - 7. info message" << ENDM;
  ILOG_INST << InfoLogger::InfoLoggerMessageOption{ InfoLogger::Fatal, 1, 1, "asdf", 3 }
            << "8. fatal message with extra fields" << ENDM;

  // Different syntax
  ILOGE << "9a. error message" << ENDM;
  ILOGF << "9b. fatal message" << ENDM;
  ILOGW << "9c. warning message" << ENDM;
  ILOGI << "9d. info message" << ENDM;

  // Using the normal functions
  ILOG_INST.logInfo("a. info message");
  ILOG_INST.logError("b. error message");
  ILOG_INST.log("c. info message");

  // Using fairlogger
  LOG(info) << "fair message in infologger";

  // using different levels
  ILOG(Debug, Devel) << "LogDebugDevel" << ENDM;
  ILOG(Warning, Ops) << "LogWarningOps" << ENDM;
  ILOG(Error, Support) << "LogErrorSupport" << ENDM;
  ILOG(Info, Trace) << "LogInfoTrace" << ENDM;
}

BOOST_AUTO_TEST_CASE(qc_info_logger_fields)
{
  ILOG(Info, Support) << "No fields set, facility=QC, system=QC, detector=<none>" << ENDM;
  QcInfoLogger::setDetector("ITS");
  ILOG(Info, Support) << "Detector ITS set, facility=QC, system=QC, detector=ITS" << ENDM;
  QcInfoLogger::setFacility("Test");
  ILOG(Info, Support) << "Facility Test set, facility=Test, system=QC, detector=ITS" << ENDM;
  QcInfoLogger::setRun(12345);
  ILOG(Info, Support) << "Run set to 12345, facility=Test, system=QC, detector=ITS" << ENDM;
  QcInfoLogger::setPartition("physics_1");
  ILOG(Info, Support) << "Partition set to physics_1, facility=Test, system=QC, detector=ITS" << ENDM;
}

BOOST_AUTO_TEST_CASE(qc_info_logger_dplil)
{
  AliceO2::InfoLogger::InfoLogger dplInfoLogger;
  auto dplContext = new AliceO2::InfoLogger::InfoLoggerContext();
  dplContext->setField(infoContext::FieldName::Facility, "dplfacility");
  dplContext->setField(infoContext::FieldName::System, "dplsystem");
  QcInfoLogger::init("facility", false, 21, &dplInfoLogger, dplContext);
}

} // namespace o2::quality_control::core
