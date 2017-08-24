///
/// \file   Publisher_test.cpp
/// \author Barthelemy von Haller
///

//#define private public // hack to have access to everything
#include "../include/Common/MeanIsAbove.h"

#define BOOST_TEST_MODULE MO test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <cassert>
#include <TH1F.h>
#include <TList.h>

namespace AliceO2 {
namespace QualityControlModules {
namespace Common {

BOOST_AUTO_TEST_CASE(test_checks)
{
  AliceO2::QualityControl::Core::MonitorObject mo;
  mo.setName("test");
  mo.setQuality(Quality::Null);
  TH1F th1f("h1", "h1", 10, 0, 9);
  mo.setObject(&th1f);
  mo.setIsOwner(false);

  MeanIsAbove check;
  check.configure("mytest");
  Quality quality = check.check(&mo);
  BOOST_CHECK_EQUAL(quality, Quality::Bad);

  th1f.Fill(1); // the threshold is set to 1 -> bad
  quality = check.check(&mo);
  BOOST_CHECK_EQUAL(quality, Quality::Bad);

  th1f.Fill(2); // the threshold is set to 1 -> good
  quality = check.check(&mo);
  BOOST_CHECK_EQUAL(quality, Quality::Good);

  check.beautify(&mo); // add a line
  BOOST_CHECK_EQUAL(1, th1f.GetListOfFunctions()->GetEntries()); 

  check.beautify(&mo);
  // Should update the line, not add one --> TODO THIS FAILS
  //  BOOST_CHECK_EQUAL(numberFunctions, th1f.GetListOfFunctions()->GetEntries()); // no modifications to the plot

}

BOOST_AUTO_TEST_CASE(test_types)
{
  AliceO2::QualityControl::Core::MonitorObject mo;
  mo.setName("test");
  mo.setQuality(Quality::Null);
  TObject obj;
  mo.setObject(&obj);
  mo.setIsOwner(false);

  MeanIsAbove check;
  check.configure("mytest");
  BOOST_TEST(!check.isObjectCheckable(&mo));

  Quality quality = check.check(&mo);
  BOOST_CHECK_EQUAL(quality, Quality::Null);
}

} /* namespace Common */
} /* namespace QualityControlModules */
} /* namespace AliceO2 */
