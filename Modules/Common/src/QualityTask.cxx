// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file    QualityTask.h
/// \author  Andrea Ferrero
/// \brief   Post-processing of quality flags
///

#include "Common/QualityTask.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/ObjectMetadataKeys.h"
#include <TDatime.h>
#include <TPaveText.h>
#include <chrono>

using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control::core;
using namespace o2::quality_control;

namespace o2::quality_control_modules::common
{

QualityTask::QualityTrendGraph::QualityTrendGraph(std::string name, std::string title) : TCanvas(name.c_str(), title.c_str())
{
  SetGridy(1);
  mGraphHist = std::make_unique<TGraph>(0);

  mGraph = std::make_unique<TGraph>(0);
  mGraph->SetMarkerStyle(kCircle);
  mGraph->SetTitle(fmt::format("{};time;quality", title).c_str());

  mLabels[0] = std::make_unique<TText>(0.09, 0.1 + 0.8 / 8.0, "Null");
  mLabels[1] = std::make_unique<TText>(0.09, 0.1 + 3.0 * 0.8 / 8.0, "Bad");
  mLabels[2] = std::make_unique<TText>(0.09, 0.1 + 5.0 * 0.8 / 8.0, "Medium");
  mLabels[3] = std::make_unique<TText>(0.09, 0.1 + 7.0 * 0.8 / 8.0, "Good");
  for (auto& l : mLabels) {
    l->SetNDC();
    l->SetTextAlign(32);
    l->SetTextSize(0.08);
    l->SetTextFont(42);
  }
}

//_________________________________________________________________________________________

void QualityTask::QualityTrendGraph::update(uint64_t time, Quality q)
{
  float val = 0.5;
  if (q == Quality::Bad) {
    val = 1.5;
  }
  if (q == Quality::Medium) {
    val = 2.5;
  }
  if (q == Quality::Good) {
    val = 3.5;
  }
  // add new point both the the main graph and to the dummy one used for the axis
  mGraph->AddPoint(time, val);
  mGraphHist->AddPoint(time, 0);

  Clear();
  cd();

  // draw axis
  mGraphHist->Draw("A");

  // configure and beautify axis
  auto hAxis = mGraphHist->GetHistogram();
  hAxis->SetTitle(GetTitle());
  hAxis->GetXaxis()->SetTimeDisplay(1);
  hAxis->GetXaxis()->SetNdivisions(505);
  hAxis->GetXaxis()->SetTimeOffset(0.0);
  hAxis->GetXaxis()->SetTimeFormat("%Y-%m-%d %H:%M");
  hAxis->GetYaxis()->SetTitle("");
  hAxis->GetYaxis()->SetTickLength(0.0);
  hAxis->GetYaxis()->SetLabelSize(0.0);
  hAxis->GetYaxis()->SetNdivisions(3, kFALSE);
  hAxis->SetMinimum(0);
  hAxis->SetMaximum(4);
  hAxis->Draw("AXIS");

  // draw main graph
  mGraph->Draw("PL,SAME");

  // draw labels for vertical axis
  for (auto& l : mLabels) {
    l->Draw();
  }
}

//_________________________________________________________________________________________

void QualityTask::configure(const boost::property_tree::ptree& config)
{
  mConfig = QualityTaskConfig(getID(), config);
}

//_________________________________________________________________________________________

static void setQualityLabels(TH1F* h)
{
  TAxis* ax = h->GetXaxis();
  ax->SetBinLabel(1, "Null");
  ax->ChangeLabel(1, 45);
  ax->SetBinLabel(2, "Bad");
  ax->ChangeLabel(2, 45);
  ax->SetBinLabel(3, "Medium");
  ax->ChangeLabel(3, 45);
  ax->SetBinLabel(4, "Good");
  ax->ChangeLabel(4, 45);
}

//_________________________________________________________________________________________

void QualityTask::initialize(quality_control::postprocessing::Trigger t, framework::ServiceRegistryRef services)
{
  mColors[Quality::Null.getName()] = kViolet - 6;
  mColors[Quality::Bad.getName()] = kRed;
  mColors[Quality::Medium.getName()] = kOrange - 3;
  mColors[Quality::Good.getName()] = kGreen + 2;

  mQualityIDs[Quality::Null.getName()] = 0;
  mQualityIDs[Quality::Bad.getName()] = 1;
  mQualityIDs[Quality::Medium.getName()] = 2;
  mQualityIDs[Quality::Good.getName()] = 3;

  mCheckerMessages[Quality::Null.getName()] = mConfig.mMessageNull;
  mCheckerMessages[Quality::Bad.getName()] = mConfig.mMessageBad;
  mCheckerMessages[Quality::Medium.getName()] = mConfig.mMessageMedium;
  mCheckerMessages[Quality::Good.getName()] = mConfig.mMessageGood;

  // instantiate the histograms and trends, one for each of the quality objects in the data sources list
  for (auto source : mConfig.dataSources) {
    auto iter1 = mHistograms.emplace(std::make_pair(source.name, std::make_unique<TH1F>(source.name.c_str(), source.name.c_str(), 4, 0, 4)));
    setQualityLabels(iter1.first->second.get());
    getObjectsManager()->startPublishing(iter1.first->second.get());

    auto iter2 = mTrends.emplace(std::make_pair(source.name, std::make_unique<QualityTrendGraph>((std::string("Trends/") + source.name).c_str(), source.name.c_str())));
    getObjectsManager()->startPublishing(iter2.first->second.get());
    getObjectsManager()->setDisplayHint(iter2.first->second.get(), "gridy");
  }

  // canvas for the human-readable messages
  mQualityCanvas = std::make_unique<TCanvas>("QualitySummary", "Quality Summary", 800, 600);
  getObjectsManager()->startPublishing(mQualityCanvas.get());
}

//_________________________________________________________________________________________
// Helper function for retrieving a QualityObject from the QCDB, in the form of a std::pair<std::shared_ptr<QualityObject>, bool>
// A non-null QO is returned in the first element of the pair if the QO is found in the QCDB
// The second element of the pair is set to true if the QO has a time stamp more recent than the last retrieved one

static std::pair<std::shared_ptr<QualityObject>, bool> getQO(repository::DatabaseInterface& qcdb, Trigger t, QualityTaskConfig::DataSource& source)
{
  // retrieve MO from CCDB
  auto qo = qcdb.retrieveQO(source.path + "/" + source.name, t.timestamp, t.activity);
  if (!qo) {
    return { nullptr, false };
  }
  // get the MO creation time stamp
  long timeStamp{ 0 };
  auto iter = qo->getMetadataMap().find(repository::metadata_keys::created);
  if (iter != qo->getMetadataMap().end()) {
    timeStamp = std::stol(iter->second);
  }
  // check if the object is newer than the last visited one
  if (timeStamp <= source.timeStamp) {
    return { qo, false };
  }

  // update the time stamp of the last visited object
  source.timeStamp = timeStamp;

  return { qo, true };
}

//_________________________________________________________________________________________

void QualityTask::update(quality_control::postprocessing::Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();
  std::vector<std::string> messages;

  Quality aggregatedQuality = Quality::Null;

  for (auto source : mConfig.dataSources) {
    // retrieve QO from CCDB, in the form of a std::pair<std::shared_ptr<QualityObject>, bool>
    // a valid object is returned in the first element of the pair if the QO is found in the QCDB
    // the second element of the pair is set to true if the QO has a time stamp more recent than the last retrieved one
    auto qo = getQO(qcdb, t, source);
    if (!qo.first) {
      continue;
    }

    auto timeSeconds = source.timeStamp / 1000; // ROOT expects seconds since epoch

    std::string qoName = qo.first->getName();
    std::string qoValue = qo.first->getQuality().getName();
    int qoID = mQualityIDs[qoValue];
    std::string qoStr = fmt::format("#color[{}]", mColors[qoValue]) + "{" + qoValue + "}";

    // only update plots/trends when the objects is updated
    if (qo.second) {
      // fill quality histogram
      auto iter1 = mHistograms.find(source.name);
      if (iter1 != mHistograms.end()) {
        iter1->second->Fill(qoID + 0.5);
      }

      // add point in quality trending plot
      auto iter2 = mTrends.find(source.name);
      if (iter2 != mTrends.end()) {
        iter2->second->update(timeSeconds, qo.first->getQuality());
      }
    }

    // add line to be displayed in summary canvas
    if (qoName == mConfig.mAggregatedQualityName) {
      // the aggregated quality is treated separately
      aggregatedQuality = qo.first->getQuality();
    } else {
      messages.emplace_back(fmt::format("{} = {}", qoName, qoStr));
    }
  }

  // add mesages for aggregated quality
  messages.insert(messages.begin(), "");
  if (!mCheckerMessages[aggregatedQuality.getName()].empty()) {
    messages.insert(messages.begin(), fmt::format("#color[{}]", mColors[aggregatedQuality.getName()]) + "{" + mCheckerMessages[aggregatedQuality.getName()] + "}");
  }
  messages.insert(messages.begin(), fmt::format("Quality = #color[{}]", mColors[aggregatedQuality.getName()]) + "{" + aggregatedQuality.getName() + "}");

  // draw messages in canvas
  mQualityCanvas->Clear();
  mQualityCanvas->cd();

  TPaveText* msg = new TPaveText(0.1, 0.1, 0.9, 0.9, "NDC");
  for (auto s : messages) {
    msg->AddText(s.c_str());
  }
  msg->SetBorderSize(0);
  msg->SetFillColor(kWhite);
  msg->Draw();
}

//_________________________________________________________________________________________

void QualityTask::finalize(quality_control::postprocessing::Trigger t, framework::ServiceRegistryRef)
{
}

} // namespace o2::quality_control_modules::common
