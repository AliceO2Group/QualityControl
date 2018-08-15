///
/// \file   SkeletonCheckDPL.cxx
/// \author Barthelemy von Haller
///

#include "SkeletonDPL/SkeletonCheckDPL.h"

// ROOT
#include <TH1.h>
#include <TPaveText.h>
#include <FairLogger.h>

using namespace std;

ClassImp(o2::quality_control_modules::skeleton_dpl::SkeletonCheckDPL)

namespace o2 {
namespace quality_control_modules {
namespace skeleton_dpl {

SkeletonCheckDPL::SkeletonCheckDPL()
{
}

SkeletonCheckDPL::~SkeletonCheckDPL()
{
}

void SkeletonCheckDPL::configure(std::string name)
{
}

Quality SkeletonCheckDPL::check(const MonitorObject *mo)
{
  Quality result = Quality::Null;

  if (mo->getName() == "example") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());

    result = Quality::Good;

    for (int i = 0; i < h->GetNbinsX(); i++) {
      if (i > 0 && i < 8 && h->GetBinContent(i) == 0) {
        result = Quality::Bad;
        break;
      } else if ((i == 0 || i > 7) && h->GetBinContent(i) > 0) {
        result = Quality::Medium;
      }
    }
  }
  return result;
}

std::string SkeletonCheckDPL::getAcceptedType()
{
  return "TH1";
}

void SkeletonCheckDPL::beautify(MonitorObject *mo, Quality checkResult)
{
  if (mo->getName() == "example") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());


//    auto* paveText = new TPaveText(0.3, 0.8, 0.7, 0.95, "NDC");
    if (checkResult == Quality::Good) {
      h->SetFillColor(kGreen);
//      paveText->SetFillColor(kGreen);
//      paveText->AddText("No anomalies");
    } else if (checkResult == Quality::Bad) {
      LOG(INFO) << "Quality::Bad, setting to red";
      h->SetFillColor(kRed);
//      paveText->SetFillColor(kRed);
//      paveText->AddText("Some missing values in between bin 1 and 7");
    } else if (checkResult == Quality::Medium) {
      LOG(INFO) << "Quality::medium, setting to orange";
      h->SetFillColor(kOrange);
//      paveText->SetFillColor(kOrange);
//      paveText->AddText("Some values in bin 0 or bins above 7");
    }
    h->SetLineColor(kBlack);
//  h->GetListOfFunctions()->AddLast(paveText);

  }
}

} // namespace skeleton_dpl
} // namespace quality_control
} // namespace o2

