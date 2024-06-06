#include "QualityControl/MonitorObject.h"
#include "EMCAL/CellTimeCalibCheck.h"
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
#include <TSpectrum.h>

using namespace std;

namespace o2::quality_control_modules::emcal
{
void CellTimeCalibCheck::configure()
{
  // configure threshold-based checkers
  auto nThreshTSpectrum = mCustomParameters.find("ThreshTSpectrum");
  if (nThreshTSpectrum != mCustomParameters.end()) {
    try {
      mThreshTSpectrum = std::stod(nThreshTSpectrum->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << "Value " << nThreshTSpectrum->second << " not a double" << ENDM;
    }
  }

  auto nSigmaTSpectrum = mCustomParameters.find("SigmaTSpectrum");
  if (nSigmaTSpectrum != mCustomParameters.end()) {
    try {
      mSigmaTSpectrum = std::stod(nSigmaTSpectrum->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << "Value " << nSigmaTSpectrum->second << " not a double" << ENDM;
    }
  }
}

Quality CellTimeCalibCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{

  Quality result = Quality::Good;
  for (auto& [moName, mo] : *moMap) {
    if (mo->getName() == "cellTimeCalib_PHYS") {

      auto* h = dynamic_cast<TH1*>(mo->getObject());
      h->GetXaxis()->SetRangeUser(-200, 200); // limit range to find peaks

      TSpectrum peakfinder;
      double threshold;
      Int_t nfound = peakfinder.Search(h, mSigmaTSpectrum, "nobackground", mThreshTSpectrum); // Search for peaks
      Double_t* xpeaks = peakfinder.GetPositionX();
      Double_t* ypeaks = peakfinder.GetPositionY();
      std::sort(ypeaks, ypeaks + nfound, std::greater<double>()); // sort peaks in descending order to easy pick the y value of max peak
      double max_peak = ypeaks[0];
      threshold = 0.4 * max_peak;

      for (Int_t n_peak = 0; n_peak < nfound; n_peak++) {
        Int_t bin = h->GetXaxis()->FindBin(xpeaks[n_peak]);
        Double_t yp = h->GetBinContent(bin);

        if (yp >= threshold && yp != max_peak) {
          result = Quality::Bad;
        }
      }
      h->GetXaxis()->SetRangeUser(-400, 800); // set range back to full range
    }
  }
  return result;
}

std::string CellTimeCalibCheck::getAcceptedType() { return "TH1"; }

void CellTimeCalibCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "cellTimeCalib_PHYS") {
    auto* h = dynamic_cast<TH1*>(mo->getObject());
    if ( h == nullptr) {
      ILOG(Error, Support) << "Could not cast 'cellTimeCalib_PHYS' to TH1*, skipping" << ENDM;
      return;
    }
    TPaveText* msg = new TPaveText(0.5, 0.5, 0.9, 0.75, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));
    if (checkResult == Quality::Good) {
      //
      msg->Clear();
      msg->AddText("Data: OK!!!");
      msg->SetFillColor(kGreen);
      //
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      ILOG(Debug, Devel) << "Quality::Bad, setting to red" << ENDM;
      msg->Clear();
      msg->AddText("Secondary peak amplitude is high");
      msg->AddText("If NOT a technical run,");
      msg->AddText("call EMCAL on-call.");
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      ILOG(Debug, Devel) << "Quality::medium, setting to orange" << ENDM;
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
}
} // namespace o2::quality_control_modules::emcal
