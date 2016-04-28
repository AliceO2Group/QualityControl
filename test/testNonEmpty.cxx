///
/// \file   testNonEmpty.cxx
/// \author flpprotodev
///

#include "../include/QualityControl/NonEmpty.h"

#define BOOST_TEST_MODULE Publisher test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <assert.h>

#include <TH1.h>

namespace AliceO2 {
namespace QualityControl {
namespace Checker {

BOOST_AUTO_TEST_CASE(checkable)
{
  TH1F histo("test", "test", 100, 0, 99);
  MonitorObject monitorObject("testObject", &histo);
  NonEmpty myCheck;

  BOOST_CHECK_EQUAL(myCheck.getAcceptedType(), "TH1");
  BOOST_CHECK_EQUAL(myCheck.isObjectCheckable(&monitorObject), true);
}

BOOST_AUTO_TEST_CASE(beautify)
{
  TH1F histo("test", "test", 100, 0, 99);
  MonitorObject monitorObject("testObject", &histo);
  NonEmpty myCheck;

  myCheck.beautify(&monitorObject, Quality::Null);
  BOOST_CHECK_EQUAL(histo.GetFillColor(), kWhite);

  myCheck.beautify(&monitorObject, Quality::Bad);
  BOOST_CHECK_EQUAL(histo.GetFillColor(), kRed);

  myCheck.beautify(&monitorObject, Quality::Good);
  BOOST_CHECK_EQUAL(histo.GetFillColor(), kGreen);

  myCheck.beautify(&monitorObject, Quality::Medium);
  BOOST_CHECK_EQUAL(histo.GetFillColor(), kOrange);

}

BOOST_AUTO_TEST_CASE(nonempty)
{
  TH1F histo("test", "test", 100, 0, 99);
  MonitorObject monitorObject("testObject", &histo);
  NonEmpty myCheck;

  Quality quality = myCheck.check(&monitorObject);
  BOOST_CHECK_EQUAL(quality, Quality::Bad);

  histo.Fill(1);
  quality = myCheck.check(&monitorObject);
  BOOST_CHECK_EQUAL(quality, Quality::Good);

  histo.Reset();
  quality = myCheck.check(&monitorObject);
  BOOST_CHECK_EQUAL(quality, Quality::Bad);
}

} // namespace Checker 
} // namespace QualityControl 
} // namespace AliceO2 
