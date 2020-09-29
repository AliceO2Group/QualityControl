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
/// \file   EverIncreasingGraph.cxx
/// \author Barthelemy von Haller
///

#include "Common/EverIncreasingGraph.h"

#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"

// ROOT
#include <TGraph.h>
#include <TH1.h>
#include <TList.h>
#include <TPaveText.h>

using namespace std;

namespace o2::quality_control_modules::common
{
void EverIncreasingGraph::configure(std::string /*name*/) {}

Quality EverIncreasingGraph::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  auto mo = moMap->begin()->second;
  Quality result = Quality::Good;
  auto* g = dynamic_cast<TGraph*>(mo->getObject());
  if(g== nullptr) {
    return Quality::Null;
  }

  // simplistic and inefficient way to check that points are always increasing
  int nbPoints = g->GetN();
  double lastY = std::numeric_limits<double>::lowest();
  for (int i = 1; i < nbPoints; i++) {
    double x, y;
    g->GetPoint(i, x, y);
    if (y < lastY) {
      result = Quality::Bad;
      break;
    }
    lastY = y;
  }

  return result;
}

std::string EverIncreasingGraph::getAcceptedType() { return "TGraph"; }

void EverIncreasingGraph::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  ILOG(Info, Support) << "EverIncreasingGraph::Beautify" << ENDM;

  if (checkResult == Quality::Null || checkResult == Quality::Medium) {
    return;
  }

  auto* g = dynamic_cast<TGraph*>(mo->getObject());
  if (!g) {
    ILOG(Error, Support) << "MO should be a graph" << ENDM;
    return;
  }

  auto* paveText = new TPaveText(0.3, 0.8, 0.7, 0.95, "NDC");
  if (checkResult == Quality::Good) {
    paveText->SetFillColor(kGreen);
    paveText->AddText("No anomalies");
  } else if (checkResult == Quality::Bad) {
    paveText->SetFillColor(kRed);
    paveText->AddText("Values are not always increasing");
  }
  g->GetListOfFunctions()->AddLast(paveText);
}

} // namespace o2::quality_control_modules::common
