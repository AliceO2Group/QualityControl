///
/// \file   MockPublisherBackend.cxx
/// \author Barthelemy von Haller
///

#include <TCanvas.h>
#include "QualityControl/MockPublisherBackend.h"
#include <boost/algorithm/string.hpp>

namespace AliceO2 {
namespace QualityControl {
namespace Core {

MockPublisherBackend::MockPublisherBackend()
{
  // TODO Auto-generated constructor stub

}

MockPublisherBackend::~MockPublisherBackend()
{
  // TODO Auto-generated destructor stub
}

void MockPublisherBackend::publish(MonitorObject *mo)
{// just to see it
  TCanvas canvas;
  mo->getObject()->Draw();
  std::string title = "test";
  title += mo->getName() + ".jpg";
  boost::erase_all(title, " ");
  canvas.SaveAs(title.c_str());
}

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2
