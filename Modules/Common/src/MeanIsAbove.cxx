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
/// \file   MeanIsAbove.cxx
/// \author Barthelemy von Haller
///

#include "Common/MeanIsAbove.h"

#include <iostream>
// ROOT
#include <TClass.h>
#include <TH1.h>
#include <TLine.h>
#include <TList.h>
// O2
#include "Configuration/ConfigurationFactory.h"

ClassImp(o2::quality_control_modules::common::MeanIsAbove)

  using namespace std;
using namespace o2::configuration;

namespace o2::quality_control_modules::common
{

MeanIsAbove::MeanIsAbove() : mThreshold(0.0) {}

void MeanIsAbove::configure(std::string /*name*/)
{
  // TODO use the configuration system to set the params
  //  try {
  //    auto configFile = ConfigurationFactory::getConfiguration("file:../example.ini"); // not ok...
  //  } catch (string &exception) {
  //    cout << "error getting config file in MeanIsAbove : " << exception << endl;
  //    mThreshold = 1.0f;
  //    return;
  //  }
  mThreshold = 1.0f; // std::stof(configFile.getValue<string>("Checks.checkMeanIsAbove/threshold"));
}

std::string MeanIsAbove::getAcceptedType() { return "TH1"; }

Quality MeanIsAbove::check(const MonitorObject* mo)
{
  auto* th1 = dynamic_cast<TH1*>(mo->getObject());
  if (!th1) {
    // TODO
    return Quality::Null;
  }
  if (th1->GetMean() > mThreshold) {
    return Quality::Good;
  }
  return Quality::Bad;
}

void MeanIsAbove::beautify(MonitorObject* mo, Quality checkResult)
{
  // A line is drawn at the level of the threshold.
  // Its colour depends on the quality.

  if (!this->isObjectCheckable(mo)) {
    cerr << "object not checkable" << endl;
    return;
  }

  auto* th1 = dynamic_cast<TH1*>(mo->getObject());

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
} // namespace o2::quality_control_modules::common
