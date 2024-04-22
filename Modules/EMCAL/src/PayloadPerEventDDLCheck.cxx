// R__ADD_INCLUDE_PATH(/Users/ar2545/alice/QualityControl/Framework/include)
////

#include "QualityControl/MonitorObject.h"
#include "EMCAL/PayloadPerEventDDLCheck.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/Quality.h"

// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TPaveText.h>
#include <TLatex.h>
#include <TList.h>
#include <TRobustEstimator.h>
#include <ROOT/TSeq.hxx>
#include <iostream>
#include <vector>
#include <TCanvas.h>
#include <string>

using namespace std;

namespace o2::quality_control_modules::emcal
{
Quality PayloadPerEventDDLCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  if (!mCalibReference) {
    // Load reference histogram from the CCDB
    mCalibReference = this->retrieveConditionAny<TH2>("/EMC/QCREF/PayloadSizeCheck");
  }

  auto mo = moMap->begin()->second;
  Quality result = Quality::Good;

  if (mo->getName() == "PayloadSizePerDDL") {

    auto* h = dynamic_cast<TH2*>(mo->getObject());

    for (int i = 1; i <= h->GetNbinsX(); i++) {
      TH1* h_y = (TH1*)h->ProjectionY(Form("h_y_%d", i), i, i);
      h_y->Scale(1. / h_y->Integral());
      TH1* h_y_ref = (TH1*)mCalibReference->ProjectionY(Form("h_y_ref_%d", i), i, i);
      h_y_ref->Scale(1. / h_y_ref->Integral());
      Double_t chisq_val = h_y->Chi2Test(h_y_ref, "UU NORM CHI2/NDF");

      if (chisq_val > 3) {
        result = Quality::Medium;
        continue;
      }
      if (5 < chisq_val && chisq_val < 7) {
        result = Quality::Bad;
        break;
      }
    }
  }
  return result;
}

std::string PayloadPerEventDDLCheck::getAcceptedType() { return "TH2"; }

void PayloadPerEventDDLCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  //    if (mo->getName() == “NPhysicsTriggers”)
  //    {
  //        auto* h = dynamic_cast<TH1*>(mo->getObject());
  //        TPaveText* msg = new TPaveText(0.5, 0.5, 0.9, 0.75, "NDC");
  //        h->GetListOfFunctions()->Add(msg);
  //        msg->SetName(Form("%s_msg", mo->GetName()));
  //        if (checkResult == Quality::Good) {
  //            //
  //            msg->Clear();
  //            msg->AddText("NPhysicsTriggs increases with time: OK!!!");
  //            msg->SetFillColor(kGreen);
  //            //
  //            h->SetFillColor(kGreen);
  //        } else if (checkResult == Quality::Bad) {
  //            ILOG(Debug, Devel) << "Quality::Bad, setting to red";
  //            msg->Clear();
  //            msg->AddText("NPhysicsTriggs EMPTY");
  //            msg->AddText("If NOT a technical run,");
  //            msg->AddText("call EMCAL on-call.");
  //            h->SetFillColor(kRed);
  //        } else if (checkResult == Quality::Medium) {
  //            ILOG(Debug, Devel) << "Quality::medium, setting to orange";
  //            h->SetFillColor(kOrange);
  //        }
  //        h->SetLineColor(kBlack);
  //    }
}
} // namespace o2::quality_control_modules::emcal
