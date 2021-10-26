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
/// \file   CheckOfTrendings.cxx
/// \author Laura Serksnyte
///

#include "TPC/CheckOfTrendings.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

#include <fairlogger/Logger.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TList.h>
#include <TPaveText.h>

#include <vector>
#include <numeric>

namespace o2::quality_control_modules::tpc
{

void CheckOfTrendings::configure(std::string) {}
Quality CheckOfTrendings::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;
  auto mo = moMap->begin()->second;

  auto* canv = (TCanvas*)mo->getObject();
  TGraph* g = (TGraph*)canv->GetListOfPrimitives()->FindObject("Graph");

  const int NBins = g->GetN();

  // If only one data point available, don't check quality
  if (NBins < 2) {
    return result;
  }

  double x_last = 0., y_last = 0.;
  g->GetPoint(NBins - 1, x_last, y_last);
  const double* yValues = g->GetY();
  const std::vector<double> v(yValues, yValues + NBins - 1);

  const double sum = std::accumulate(v.begin(), v.end(), 0.0);
  const double mean = sum / v.size();

  std::vector<double> diff(v.size());
  std::transform(v.begin(), v.end(), diff.begin(), [mean](double x) { return x - mean; });
  const double sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
  const double stdev = std::sqrt(sq_sum / v.size());

  if (fabs(y_last - mean) <= stdev * 3.) {
    result = Quality::Good;
  } else if (fabs(y_last - mean) > stdev * 6.) {
    result = Quality::Bad;
  } else {
    result = Quality::Medium;
  }
  return result;
}

std::string CheckOfTrendings::getAcceptedType() { return "TCanvas"; }

void CheckOfTrendings::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  auto* c1 = (TCanvas*)mo->getObject();
  TGraph* h = (TGraph*)c1->GetListOfPrimitives()->FindObject("Graph");
  TPaveText* msg = new TPaveText(0.7, 0.85, 0.9, 0.9, "NDC");
  h->GetListOfFunctions()->Add(msg);
  msg->SetName(Form("%s_msg", mo->GetName()));

  if (checkResult == Quality::Good) {
    h->SetFillColor(kGreen);
    msg->Clear();
    msg->AddText("Quality::Good");
    msg->SetFillColor(kGreen);
  } else if (checkResult == Quality::Bad) {
    LOG(INFO) << "Quality::Bad, setting to red";
    h->SetFillColor(kRed);
    msg->Clear();
    msg->AddText("Quality::Bad");
    msg->AddText("Outlier, more than 6sigma.");
    msg->SetFillColor(kRed);
  } else if (checkResult == Quality::Medium) {
    LOG(INFO) << "Quality::medium, setting to orange";
    h->SetFillColor(kOrange);
    msg->Clear();
    msg->AddText("Quality::Medium");
    msg->AddText("Outlier, more than 3sigma.");
    msg->SetFillColor(kOrange);
  } else if (checkResult == Quality::Null) {
    h->SetFillColor(0);
  }
  h->SetLineColor(kBlack);
}

} // namespace o2::quality_control_modules::tpc