///
/// \file   EverIncreasingGraph.cxx
/// \author Barthelemy von Haller
///

#include "Daq/EverIncreasingGraph.h"

// ROOT
#include <TGraph.h>
#include <TH1.h>
#include <TList.h>
#include <TPaveText.h>

using namespace std;

ClassImp(o2::quality_control_modules::daq::EverIncreasingGraph)

  namespace o2
{
  namespace quality_control_modules
  {
  namespace daq
  {

  EverIncreasingGraph::EverIncreasingGraph() {}

  EverIncreasingGraph::~EverIncreasingGraph() {}

  void EverIncreasingGraph::configure(std::string name) {}

  Quality EverIncreasingGraph::check(const MonitorObject* mo)
  {
    Quality result = Quality::Good;
    auto* g = dynamic_cast<TGraph*>(mo->getObject());

    // simplistic and inefficient way to check that points are always increasing
    int nbPoints = g->GetN();
    double lastY = -DBL_MAX;
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

  void EverIncreasingGraph::beautify(MonitorObject* mo, Quality checkResult)
  {
    cout << "Beautify" << endl;

    if (checkResult == Quality::Null || checkResult == Quality::Medium) {
      return;
    }

    auto* g = dynamic_cast<TGraph*>(mo->getObject());
    if (!g) {
      cerr << "MO should be a graph" << endl;
      return;
    }

    auto* paveText = new TPaveText(0.3, 0.8, 0.7, 0.95, "NDC");
    if (checkResult == Quality::Good) {
      paveText->SetFillColor(kGreen);
      paveText->AddText("No anomalies");
    } else if (checkResult == Quality::Bad) {
      paveText->SetFillColor(kRed);
      paveText->AddText("Block IDs are not always increasing");
    }
    g->GetListOfFunctions()->AddLast(paveText);
  }

  } // namespace daq
  } // namespace quality_control_modules
} // namespace o2
