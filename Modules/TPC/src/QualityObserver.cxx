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
/// \file     QualityObserver.cxx
/// \author   Marcel Lesch
///

#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QcInfoLogger.h"
#include <TPC/QualityObserver.h>
#include <boost/property_tree/ptree.hpp>
#include <TPaveText.h>
#include <TLine.h>
#include <TText.h>

using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control_modules::tpc;

void QualityObserver::configure(std::string name,
                                const boost::property_tree::ptree& config)
{
  mObserverName = config.get<std::string>("qc.postprocessing." + name + ".qualityObserverName");

  for (const auto& dataSourceConfig : config.get_child("qc.postprocessing." + name + ".qualityObserverConfig")) {
    Config dataConfig;

    dataConfig.GroupTitle = dataSourceConfig.second.get<std::string>("groupTitle");
    dataConfig.Path = dataSourceConfig.second.get<std::string>("path");

    std::vector<std::string> inputQO;
    std::vector<std::string> inputQOTitle;

    if (const auto& QOsources = dataSourceConfig.second.get_child_optional("inputObjects"); QOsources.has_value()) {
      for (const auto& QOsource : QOsources.value()) {
        inputQO.push_back(QOsource.second.data());
      }
    }
    if (const auto& QOTitlesources = dataSourceConfig.second.get_child_optional("inputObjectTitles"); QOTitlesources.has_value()) {
      for (const auto& QOTitlesource : QOTitlesources.value()) {
        inputQOTitle.push_back(QOTitlesource.second.data());
      }
    }
    if (inputQO.size() != inputQOTitle.size()) {
      ILOG(Error, Devel) << "in config of group" << dataConfig.GroupTitle << ": Number of QOs does not match number of QO titles!" << ENDM;
    }
    dataConfig.QO = inputQO;
    dataConfig.QOTitle = inputQOTitle;
    mConfig.push_back(dataConfig);

    inputQO.clear();
    inputQOTitle.clear();
  } // for (const auto& dataSourceConfig : config.get_child("qc.postprocessing." + name + ".ratioConfig"))
}

void QualityObserver::initialize(Trigger, framework::ServiceRegistry&)
{
  for (const auto config : mConfig) {
    Qualities[config.GroupTitle] = std::vector<std::string>();
  }
  mColors["bad"] = kRed;
  mColors["medium"] = kOrange - 3;
  mColors["good"] = kGreen + 2;
  mColors["no quality"] = kGray + 2;
}

void QualityObserver::update(Trigger t, framework::ServiceRegistry& services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();
  getQualities(t, qcdb);
  generatePanel();
}

void QualityObserver::finalize(Trigger t, framework::ServiceRegistry&)
{
  generatePanel();
  if (mCanvas) {
    delete mCanvas;
    mCanvas = nullptr;
  }
}

void QualityObserver::getQualities(const Trigger& t,
                                   repository::DatabaseInterface& qcdb)
{
  for (const auto& config : mConfig) {

    if (Qualities[config.GroupTitle].size() > 0) {
      Qualities[config.GroupTitle].clear();
    }
    for (const std::string qualityobject : config.QO) {
      auto qo = qcdb.retrieveQO(config.Path + "/" + qualityobject, t.timestamp, t.activity);

      if (qo) {
        const auto quality = qo->getQuality();
        uint qualitylevel = quality.getLevel();

        if (qualitylevel == 1) {
          Qualities[config.GroupTitle].push_back("good");
        } else if (qualitylevel == 2) {
          Qualities[config.GroupTitle].push_back("medium");
        } else if (qualitylevel == 3) {
          Qualities[config.GroupTitle].push_back("bad");
        } else {
          Qualities[config.GroupTitle].push_back("no quality");
        }
      } else {
        Qualities[config.GroupTitle].push_back("no quality");
      }
    }
  }
} // void QualityObserver::getQualities(const Trigger& t, repository::DatabaseInterface& qcdb)

void QualityObserver::generatePanel()
{
  // Delete the existing plots before regenerating them.
  if (mCanvas) {
    getObjectsManager()->stopPublishing(mObserverName);
    delete mCanvas;
    mCanvas = nullptr;
  }

  // Draw the ratio on a new canvas.
  TCanvas* c = new TCanvas();
  c->SetName(mObserverName.c_str());
  c->SetTitle(mObserverName.c_str());
  c->cd(1);

  TPaveText* pt = new TPaveText(0.05, 0.05, .95, .95);
  pt->SetLineColor(0);
  pt->SetFillColor(0);
  pt->SetBorderSize(1);

  for (const auto& config : mConfig) {

    pt->AddText(""); // Emtpy line needed. AddLine() places the line to the first entry of the for loop. Check late
    TText* GroupText = pt->AddText(config.GroupTitle.data());
    ((TText*)pt->GetListOfLines()->Last())->SetTextAlign(22);
    for (int i = 0; i < config.QOTitle.size(); i++) {
      pt->AddText(Form("%s = #color[%d]{%s}", config.QOTitle.at(i).data(), mColors[Qualities[config.GroupTitle].at(i).data()], Qualities[config.GroupTitle].at(i).data()));
      // To-Check: SetTextAlign does currently not work in QCG
      ((TText*)pt->GetListOfLines()->Last())->SetTextAlign(12);
    }

    // To-Check: AddLine broken for qcg. Does not simply append line
    pt->AddLine();
    ((TLine*)pt->GetListOfLines()->Last())->SetLineWidth(3);
    ((TLine*)pt->GetListOfLines()->Last())->SetLineStyle(9);
  }

  pt->Draw();
  mCanvas = c;
  getObjectsManager()->startPublishing(c);

} // void QualityObserver::generatePanel()