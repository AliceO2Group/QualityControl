#include "EMCAL/TrendGraphCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/Quality.h"

// ROOT
#include <ROOT/TSeq.hxx>
#include <TCanvas.h>
#include <TGraph.h>
#include <TH1.h>
#include <TH2.h>
#include <TLatex.h>
#include <TList.h>
#include <TPaveText.h>
#include <TRobustEstimator.h>
#include <iostream>
#include <string>
#include <vector>
#include <queue>

namespace o2::quality_control_modules::emcal
{
void TrendGraphCheck::configure()
{

  // configure threshold-based checkers
  auto nBadThresholdLow = mCustomParameters.find("BadThresholdLow");
  if (nBadThresholdLow != mCustomParameters.end()) {
    try {
      mBadThresholdLow = std::stod(nBadThresholdLow->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << "Value " << nBadThresholdLow->second.data()
                           << " not a double" << ENDM;
    }
  }

  auto nBadThresholdHigh = mCustomParameters.find("BadThresholdHigh");
  if (nBadThresholdHigh != mCustomParameters.end()) {
    try {
      mBadThresholdHigh = std::stod(nBadThresholdHigh->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << "Value " << nBadThresholdHigh->second.data()
                           << " not a double" << ENDM;
    }
  }

  auto nBadDiff = mCustomParameters.find("BadDiff");
  if (nBadDiff != mCustomParameters.end()) {
    try {
      mBadDiff = std::stod(nBadDiff->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << "Value " << nBadDiff->second.data() << " not a double" << ENDM;
    }
  }

  auto nPeriodMovAvg = mCustomParameters.find("PeriodMovAvg");
  if (nPeriodMovAvg != mCustomParameters.end()) {
    try {
      mPeriodMovAvg = std::stoi(nPeriodMovAvg->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << "Value " << nPeriodMovAvg->second.data()
                           << " not an int" << ENDM;
    }
  }
}

Quality TrendGraphCheck::check(
  std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Good;

  for (auto& [moName, mo] : *moMap) {
    auto* c = dynamic_cast<TCanvas*>(mo->getObject());
    TList* list_name = c->GetListOfPrimitives();
    double counts = -1;
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
        if (meanArray[i] < mBadThresholdLow ||
            meanArray[i] > mBadThresholdHigh) {
          result = Quality::Medium;
        }
      }

      for (decltype(meanArray.size()) i = 0; i < meanArray.size() - 1; ++i) {
        if (std::abs(meanArray[i] - meanArray[i + 1]) > mBadDiff) {
          result = Quality::Bad;
          break;
        }
      }

      meanArray.clear();
    }
  }
  return result;
}

std::string TrendGraphCheck::getAcceptedType() { return "TGraph"; }

void TrendGraphCheck::beautify(std::shared_ptr<MonitorObject> mo,
                               Quality checkResult)
{

  auto* c = dynamic_cast<TCanvas*>(mo->getObject());
  TList* list_name = c->GetListOfPrimitives();
  for (auto h : TRangeDynCast<TGraph>(list_name)) {
    if (!h) {
      continue;
    }
    TPaveText* msg = new TPaveText(0.5, 0.5, 0.9, 0.75, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));
    if (checkResult == Quality::Good) {
      //
      msg->Clear();
      msg->AddText("Trend is as expected: OK!!!");
      msg->SetFillColor(kGreen);
      msg->Draw();
      //
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      ILOG(Debug, Devel) << "Quality::Bad, setting to red" << ENDM;
      msg->Clear();
      msg->AddText("Consecutive trend rates very different: BAD!!!");
      msg->AddText("If NOT a technical run,");
      msg->AddText("call EMCAL on-call.");
      msg->Draw();
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      ILOG(Debug, Devel) << "Quality::medium, setting to orange" << ENDM;
      msg->Clear();
      msg->AddText("Trend rate outside expected range. Keep monitoring.");
      msg->Draw();
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
}
} // namespace o2::quality_control_modules::emcal
