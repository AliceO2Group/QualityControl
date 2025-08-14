#include "EMCAL/CellAmpCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/Quality.h"

// ROOT
#include <ROOT/TSeq.hxx>
#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TLatex.h>
#include <TList.h>
#include <TPaveText.h>
#include <TRobustEstimator.h>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

namespace o2::quality_control_modules::emcal
{
void CellAmpCheck::configure()
{
  // configure threshold-based checkers
  auto nChiSqMedThresh = mCustomParameters.find("ChiSqMedThresh");
  if (nChiSqMedThresh != mCustomParameters.end()) {
    try {
      mChiSqMedThresh = std::stod(nChiSqMedThresh->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << "Value " << nChiSqMedThresh->second.data()
                           << " not a double" << ENDM;
    }
  }

  auto nBadChiSqThresh = mCustomParameters.find("BadChiSqThresh");
  if (nBadChiSqThresh != mCustomParameters.end()) {
    try {
      mChiSqBadThresh = std::stod(nBadChiSqThresh->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << "Value " << nBadChiSqThresh->second.data()
                           << " not a double" << ENDM;
    }
  }
}

Quality CellAmpCheck::check(
  std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  if (!mCellAmpReference) {
    // Load reference histogram from the CCDB
    mCellAmpReference =
      this->retrieveConditionAny<TH1>("EMC/QCREF/CellAmpCalibCheck");
  }
  if (!mCellAmpReference) {
    // If histogram not found print out error message
    ILOG(Error, Support) << "Could not find the reference histogram!" << ENDM;
  }

  Quality result = Quality::Good;
  for (auto& [moName, mo] : *moMap) {
    if (mo->getName() == "cellAmplitudeCalib_PHYS") {

      auto* h = dynamic_cast<TH1*>(mo->getObject());
      h->Scale(1. / h->Integral());
      mCellAmpReference->Scale(1. / mCellAmpReference->Integral());

      Double_t chisq_val = h->Chi2Test(mCellAmpReference, "UU NORM CHI2/NDF");
      if (chisq_val > mChiSqMedThresh) {
        result = Quality::Medium;
      }
      if (mChiSqBadThresh < chisq_val) {
        result = Quality::Bad;
      }
    }
  }
  return result;
}



void CellAmpCheck::beautify(std::shared_ptr<MonitorObject> mo,
                            Quality checkResult)
{
  if (mo->getName() == "cellAmplitudeCalib_PHYS") {
    auto* h = dynamic_cast<TH1*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.5, 0.5, 0.9, 0.75, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));
    if (checkResult == Quality::Good) {
      //
      msg->Clear();
      msg->AddText("Cell amplitude within expectations.");
      msg->SetFillColor(kGreen);
      msg->Draw();
      //
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      ILOG(Debug, Devel) << "Quality::Bad, setting to red" << ENDM;
      msg->Clear();
      msg->AddText("Suspicious cell amplitude detected.");
      msg->AddText("If NOT a technical run,");
      msg->AddText("call EMCAL on-call.");
      msg->Draw();
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      ILOG(Debug, Devel) << "Quality::medium, setting to orange" << ENDM;
      msg->Clear();
      msg->AddText("WARNING: Deviation from expectations. Keep monitoring");
      msg->Draw();
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
}
} // namespace o2::quality_control_modules::emcal
