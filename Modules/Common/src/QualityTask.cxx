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
/// \author  Piotr Konopka
/// \brief   Post-processing of quality flags
///

#include "Common/QualityTask.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/ObjectMetadataKeys.h"
#include <TDatime.h>
#include <TPaveText.h>
#include <TLine.h>
#include <chrono>
#include <variant>

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

std::string QualityTask::QualityTrendGraph::distributionName(const std::string& groupName, const std::string& qualityName)
{
  return "Distributions/" + groupName + "/" + qualityName;
}

std::string QualityTask::QualityTrendGraph::trendName(const std::string& groupName, const std::string& qualityName)
{
  return "Trends/" + groupName + "/" + qualityName;
}

static std::string fullQoPath(const std::string& path, const std::string& name)
{
  return path + "/" + name;
}

static std::string uniqueQoID(const std::string& group, const std::string& fullPath)
{
  return group + "/" + fullPath;
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

  // instantiate the histograms and trends, one for each of the quality objects in the data sources list
  for (const auto& qualityGroupConfig : mConfig.qualityGroups) {
    for (const auto& qualityConfig : qualityGroupConfig.inputObjects) {
      auto qoID = uniqueQoID(qualityGroupConfig.name, fullQoPath(qualityGroupConfig.path, qualityConfig.name));
      mLatestTimestamps[qoID] = 0;

      auto distributionName = QualityTrendGraph::distributionName(qualityGroupConfig.name, qualityConfig.name);
      auto distributionTitle = qualityConfig.title.empty() ? qualityConfig.name : qualityConfig.title;
      auto distrIter = mHistograms.emplace(std::make_pair(distributionName, std::make_unique<TH1F>(distributionName.c_str(), distributionTitle.c_str(), 4, 0, 4)));
      setQualityLabels(distrIter.first->second.get());
      getObjectsManager()->startPublishing(distrIter.first->second.get());

      auto trendName = QualityTrendGraph::trendName(qualityGroupConfig.name, qualityConfig.name);
      auto trendTitle = qualityConfig.title.empty() ? qualityConfig.name : qualityConfig.title;
      auto iter2 = mTrends.emplace(std::make_pair(trendName, std::make_unique<QualityTrendGraph>(trendName.c_str(), trendTitle)));
      getObjectsManager()->startPublishing(iter2.first->second.get());
      getObjectsManager()->setDisplayHint(iter2.first->second.get(), "gridy");
    }
  }

  // canvas for the human-readable messages
  mQualityCanvas = std::make_unique<TCanvas>("QualitySummary", "Quality Summary", 800, 600);
  getObjectsManager()->startPublishing(mQualityCanvas.get());
}

//_________________________________________________________________________________________
// Helper function for retrieving a QualityObject from the QCDB, in the form of a std::pair<std::shared_ptr<QualityObject>, bool>
// A non-null QO is returned in the first element of the pair if the QO is found in the QCDB
// The second element of the pair is set to true if the QO has a time stamp more recent than the last retrieved one

std::pair<std::shared_ptr<QualityObject>, bool> QualityTask::getQO(
  repository::DatabaseInterface& qcdb, const Trigger& t, const std::string& fullPath, const std::string& group)
{
  // retrieve QO from CCDB
  auto qo = qcdb.retrieveQO(fullPath, t.timestamp, t.activity);
  if (!qo) {
    return { nullptr, false };
  }
  // get the MO creation time stamp
  long thisTimestamp{ 0 };
  auto createdIter = qo->getMetadataMap().find(repository::metadata_keys::created);
  if (createdIter != qo->getMetadataMap().end()) {
    thisTimestamp = std::stol(createdIter->second);
  }

  // check if the object is newer than the last visited one
  auto qoID = uniqueQoID(group, fullPath);
  auto lastTimestamp = mLatestTimestamps[qoID];
  if (thisTimestamp <= lastTimestamp) {
    return { qo, false };
  }

  // update the time stamp of the last visited object
  mLatestTimestamps[qoID] = thisTimestamp;

  return { qo, true };
}

//_________________________________________________________________________________________

void QualityTask::update(quality_control::postprocessing::Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();

  struct Separator {
  };
  struct TextAlign {
    Short_t value;
  };
  struct Message {
    std::string text;
  };
  std::vector<std::variant<Separator, TextAlign, Message>> lines;

  for (const auto& qualityGroupConfig : mConfig.qualityGroups) {
    if (!qualityGroupConfig.title.empty()) {
      lines.emplace_back(Message{ qualityGroupConfig.title });
      lines.emplace_back(TextAlign{ 22 });
    }

    for (const auto& qualityConfig : qualityGroupConfig.inputObjects) {
      auto fullPath = fullQoPath(qualityGroupConfig.path, qualityConfig.name);
      auto& qualityTitle = qualityConfig.title.empty() ? qualityConfig.name : qualityConfig.title;
      // retrieve QO from CCDB, in the form of a std::pair<std::shared_ptr<QualityObject>, bool>
      // a valid object is returned in the first element of the pair if the QO is found in the QCDB
      // the second element of the pair is set to true if the QO has a time stamp more recent than the last retrieved one
      auto [qo, wasUpdated] = getQO(qcdb, t, fullPath, qualityGroupConfig.name);
      if (!qo) {
        lines.emplace_back(Message{ fmt::format("#color[{}]{{{} : quality missing!}}", mColors[Quality::Null.getName()], qualityTitle) });
        lines.emplace_back(TextAlign{ 12 });
        continue;
      }

      const auto& quality = qo->getQuality();
      std::string qoName = qo->getName();
      std::string qoValue = quality.getName();
      int qID = mQualityIDs[qoValue];
      lines.emplace_back(Message{ fmt::format("#color[{}]{{{} : {}}}", mColors[qoValue], qualityTitle, qoValue) });
      lines.emplace_back(TextAlign{ 12 });

      if (auto& message = qualityConfig.messages.at(qoValue); !message.empty()) {
        lines.emplace_back(Message{ fmt::format("#color[{}]{{{}}}", mColors[qoValue], message) });
        lines.emplace_back(TextAlign{ 12 });
      }

      if (std::find(qualityGroupConfig.ignoreQualitiesDetails.begin(), qualityGroupConfig.ignoreQualitiesDetails.end(), quality) == qualityGroupConfig.ignoreQualitiesDetails.end()) {
        for (const auto& [flag, comment] : qo->getReasons()) {
          if (comment.empty()) {
            lines.emplace_back(Message{ fmt::format("#color[{}]{{#rightarrow Flag: {}}}", kGray + 2, flag.getName()) });
          } else {
            lines.emplace_back(Message{ fmt::format("#color[{}]{{#rightarrow Flag: {}: {}}}", kGray + 2, flag.getName(), comment) });
          }
          lines.emplace_back(TextAlign{ 12 });
        }
      }

      // only update plots/trends when the objects is wasUpdated
      if (wasUpdated) {
        // fill quality histogram
        auto distributionName = QualityTrendGraph::distributionName(qualityGroupConfig.name, qualityConfig.name);
        auto distroIter = mHistograms.find(distributionName);
        if (distroIter != mHistograms.end()) {
          distroIter->second->Fill(qID + 0.5);
        }

        // add point in quality trending plot
        auto trendName = QualityTrendGraph::trendName(qualityGroupConfig.name, qualityConfig.name);
        auto trendIter = mTrends.find(trendName);
        if (trendIter != mTrends.end()) {
          auto qoID = uniqueQoID(qualityGroupConfig.name, fullPath);
          auto timestampSeconds = mLatestTimestamps[qoID] / 1000; // ROOT expects seconds since epoch
          trendIter->second->update(timestampSeconds, qo->getQuality());
        }
      }
    }
    lines.emplace_back(Separator{});
  }
  lines.pop_back(); // remove the last separator

  // draw lines in canvas
  mQualityCanvas->Clear();
  mQualityCanvas->cd(1);

  auto* msg = new TPaveText(0.05, 0.05, 0.95, 0.95, "NDC");
  msg->SetLineColor(kBlack);
  msg->SetFillColor(kWhite);
  msg->SetBorderSize(0);

  for (const auto& line : lines) {
    if (std::holds_alternative<Message>(line)) {
      msg->AddText(std::get<Message>(line).text.c_str());
    } else if (std::holds_alternative<Separator>(line)) {
      msg->AddLine();
      ((TLine*)msg->GetListOfLines()->Last())->SetLineWidth(3);
      ((TLine*)msg->GetListOfLines()->Last())->SetLineStyle(9);
      ((TLine*)msg->GetListOfLines()->Last())->SetLineColor(kBlack);
    } else if (std::holds_alternative<TextAlign>(line)) {
      ((TText*)msg->GetListOfLines()->Last())->SetTextAlign(std::get<TextAlign>(line).value);
    }
  }

  msg->Draw();
}

//_________________________________________________________________________________________

void QualityTask::finalize(quality_control::postprocessing::Trigger t, framework::ServiceRegistryRef)
{
}

} // namespace o2::quality_control_modules::common
