///
/// \file   SkeletonCheck.cxx
/// \author Piotr Konopka
///

#include "Skeleton/SkeletonCheck.h"

// ROOT
#include <FairLogger.h>
#include <TH1.h>
#include <TPaveText.h>

using namespace std;

namespace o2::quality_control_modules::skeleton
{

SkeletonCheck::SkeletonCheck() {}

SkeletonCheck::~SkeletonCheck() {}

void SkeletonCheck::configure(std::string) {}

Quality SkeletonCheck::check(const MonitorObject* mo)
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

std::string SkeletonCheck::getAcceptedType() { return "TH1"; }

void SkeletonCheck::beautify(MonitorObject* mo, Quality checkResult)
{
  if (mo->getName() == "example") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());

    if (checkResult == Quality::Good) {
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      LOG(INFO) << "Quality::Bad, setting to red";
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      LOG(INFO) << "Quality::medium, setting to orange";
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
}

} // namespace daq::quality_control_modules::skeleton
