///
/// \file   MeanIsAbove.cxx
/// \author Barthelemy von Haller
///

#include "Common/MeanIsAbove.h"

// ROOT
#include <TClass.h>
#include <TH1.h>
#include <TList.h>
#include <TLine.h>
// O2
#include "Configuration/Configuration.h"

ClassImp(AliceO2::QualityControlModules::Common::MeanIsAbove)

using namespace std;

namespace AliceO2 {
namespace QualityControlModules {
namespace Common {

MeanIsAbove::MeanIsAbove()
    : mThreshold(0.0)
{
}

void MeanIsAbove::configure(std::string name)
{
  // TODO use the configuration system to set the params
  AliceO2::Configuration::ConfigFile configFile;
  try {
    configFile.load("file:../example.ini"); // not ok...
  } catch (string &exception) {
    cout << "error getting config file in MeanIsAbove : " << exception << endl;
    mThreshold = 1.0f;
    return;
  }
  mThreshold = 1.0f;//std::stof(configFile.getValue<string>("Checks.checkMeanIsAbove/threshold"));
}

std::string MeanIsAbove::getAcceptedType()
{
  return "TH1";
}

Quality MeanIsAbove::check(const MonitorObject *mo)
{
  TH1 *th1 = dynamic_cast<TH1*>(mo->getObject());
  if (!th1) {
    // TODO
    return Quality::Null;
  }
  if (th1->GetMean() > mThreshold) {
    return Quality::Good;
  }
  return Quality::Bad;
}

void MeanIsAbove::beautify(MonitorObject *mo, Quality checkResult)
{
  // A line is drawn at the level of the threshold.
  // Its colour depends on the quality.

  if (!this->isObjectCheckable(mo)) {
    cerr << "object not checkable" << endl;
    return;
  }

  TH1* th1 = dynamic_cast<TH1*>(mo->getObject());

  Double_t xMin = th1->GetXaxis()->GetXmin();
  Double_t xMax = th1->GetXaxis()->GetXmax();
  auto* lineMin = new TLine(xMin, mThreshold, xMax, mThreshold);
  lineMin->SetLineWidth(2);
  th1->GetListOfFunctions()->Add(lineMin);

  // set the colour according to the quality
  if (checkResult == Quality::Good) {
    lineMin->SetLineColor(kGreen);
  } else if (checkResult == Quality::Medium) {
    lineMin->SetLineColor(kOrange);
  } else if (checkResult == Quality::Bad) {
    lineMin->SetLineColor(kRed);
  } else {
    lineMin->SetLineColor(kWhite);
  }
}
} /* namespace Common */
} /* namespace QualityControlModules */
} /* namespace AliceO2 */

