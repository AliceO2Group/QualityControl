#include "QualityControl/MonitorObject.h"
#include "EMCAL/RawErrorCheckAll.h"
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
#include <TGraph.h>
#include <queue>

namespace o2::quality_control_modules::emcal
{
void RawErrorCheckAll::configure()
{

  // configure threshold-based checkers
  auto nBadThreshold = mCustomParameters.find("BadThreshold");
  if (nBadThreshold != mCustomParameters.end()) {
    try {
      mBadThreshold = std::stod(nBadThreshold->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << "Value " << nBadThreshold->second << " not a double" << ENDM;
    }
  }

  auto nPeriodMovAvg = mCustomParameters.find("PeriodMovAvg");
  if (nPeriodMovAvg != mCustomParameters.end()) {
    try {
      mPeriodMovAvg = std::stoi(nPeriodMovAvg->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << "Value " << nPeriodMovAvg->second << " not an int" << ENDM;
    }
  }
}

Quality RawErrorCheckAll::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Good;

  for (auto& [moName, mo] : *moMap) {
    if (mo->getName() == "TrendRawDataError") {
      auto* canvas_obj = dynamic_cast<TCanvas*>(mo->getObject());
      auto* list_name = canvas_obj->GetListOfPrimitives();
      for (auto trendgraph : TRangeDynCast<TGraph>(list_name)) {
        if (!trendgraph) {
          continue;
        }

        // queue used to store list so that we get the average
        std::queue<double> Dataset;
        double sum = 0;
        double mean = 0;

        auto* yValues = trendgraph->GetY();
        auto numPoints = trendgraph->GetN();
        std::vector<double> meanArray(numPoints);
        for (int i = 0; i < trendgraph->GetN(); ++i) {
          double y = yValues[i];
          sum += y;
          Dataset.push(y);
          if (Dataset.size() > mPeriodMovAvg) {
            sum -= Dataset.front();
            Dataset.pop();
          }
          double mean = sum / mPeriodMovAvg;
          meanArray[i] = mean;
        }

        for (decltype(meanArray.size()) i = 0; i < meanArray.size() - 1; ++i) {
          if (meanArray[i] > mBadThreshold && meanArray[i + 1] > mBadThreshold) {
            result = Quality::Bad;
            break;
          }
        }
        meanArray.clear();
      }
    }
  }

  return result;
}



void RawErrorCheckAll::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "TrendRawDataError") {
    auto* h = dynamic_cast<TH1*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.5, 0.5, 0.9, 0.75, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));
    if (checkResult == Quality::Good) {
      //
      msg->Clear();
      msg->AddText("Average raw error rate within threshold: OK!!!");
      msg->SetFillColor(kGreen);
      //
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      ILOG(Debug, Devel) << "Quality::Bad, setting to red";
      msg->Clear();
      msg->AddText("Average raw error rate above threshold");
      msg->AddText("If NOT a technical run,");
      msg->AddText("call EMCAL on-call.");
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      ILOG(Debug, Devel) << "Quality::medium, setting to orange";
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
}
} // namespace o2::quality_control_modules::emcal
