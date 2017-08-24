///
/// \file   Publisher_test.cpp
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

namespace AliceO2 {
namespace QualityControl {
namespace Core {

BOOST_AUTO_TEST_CASE(qc_info_logger)
{
  QcInfoLogger &qc1 = QcInfoLogger::GetInstance();
  QcInfoLogger &qc2 = QcInfoLogger::GetInstance();
  BOOST_CHECK_EQUAL(&qc1, &qc2);
  qc1 << "test" << AliceO2::InfoLogger::InfoLogger::endm;
}

} /* namespace ObjectsManager */
} /* namespace QualityControl */
} /* namespace AliceO2 */
